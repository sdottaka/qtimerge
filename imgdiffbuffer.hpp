#pragma once

#include "image.hpp"
#include <string>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>

template<class T> struct Point
{
	Point(T x, T y): x(x), y(y) {}
	T x, y;
};

template<class T> struct Size
{
	Size(T cx, T cy): cx(cx), cy(cy) {}
	T cx, cy;
};

template<class T> struct Rect
{
	Rect(T left, T top, T right, T bottom): left(left), top(top), right(right), bottom(bottom) {}
	T left, top, right, bottom;
};

template <class T> struct Array2D
{
	Array2D() : m_width(0), m_height(0), m_data(NULL)
	{
	}

	Array2D(size_t width, size_t height) : m_width(width), m_height(height), m_data(new T[width * height])
	{
		memset(m_data, 0, m_width * m_height * sizeof(T));
	}

	Array2D(const Array2D& other) : m_width(other.m_width), m_height(other.m_height), m_data(new T[other.m_width * other.m_height])
	{
		memcpy(m_data, other.m_data, m_width * m_height * sizeof(T));
	}

	Array2D& operator=(const Array2D& other)
	{
		if (this != &other)
		{
			delete[] m_data;
			m_width  = other.m_width;
			m_height = other.m_height;
			m_data = new T[other.m_width * other.m_height];
			memcpy(m_data, other.m_data, m_width * m_height * sizeof(T));
		}
		return *this;
	}

	~Array2D()
	{
		delete[] m_data;
	}

	void resize(size_t width, size_t height)
	{
		delete[] m_data;
		m_data = new T[width * height];
		m_width  = width;
		m_height = height;
		memset(m_data, 0, sizeof(T) * width * height);
	}

	T& operator()(int x, int y)
	{
		return m_data[y * m_width + x];
	}

	const T& operator()(int x, int y) const
	{
		return m_data[y * m_width + x];
	}

	void clear()
	{
		delete[] m_data;
		m_data = NULL;
		m_width = 0;
		m_height = 0;
	}

	size_t height() const
	{
		return m_height;
	}

	size_t width() const
	{
		return m_width;
	}

	size_t m_width, m_height;
	T* m_data;
};

struct DiffInfo
{
	enum OP_TYPE
	{
		OP_NONE = 0, OP_1STONLY, OP_2NDONLY, OP_3RDONLY, OP_DIFF, OP_TRIVIAL
	};
	DiffInfo(int op, int x, int y) : op(op), rc(x, y, x + 1, y + 1) {}
	int op;
	Rect<int> rc;
};

struct DiffStat { int d1, d2, d3, detc; };

namespace
{
	int GetColorDistance2(Image::Color c1, Image::Color c2)
	{
		int rdist = Image::valueR(c1) - Image::valueR(c2);
		int gdist = Image::valueG(c1) - Image::valueG(c2);
		int bdist = Image::valueB(c1) - Image::valueB(c2);
		int adist = Image::valueA(c1) - Image::valueA(c2);
		return rdist * rdist + gdist * gdist + bdist * bdist + adist * adist;
	}
}

class CImgDiffBuffer
{
	typedef Array2D<int> DiffBlocks;

public:
	enum OVERLAY_MODE {
		OVERLAY_NONE = 0, OVERLAY_XOR, OVERLAY_ALPHABLEND
	};

	CImgDiffBuffer() : 
		  m_nImages(0)
		, m_showDifferences(true)
		, m_overlayMode(OVERLAY_NONE)
		, m_overlayAlpha(0.3)
		, m_diffBlockSize(8)
		, m_selDiffColor(Image::Rgb(0xff, 0x40, 0x40))
		, m_diffColor(Image::Rgb(0xff, 0xff, 0x40))
		, m_diffColorAlpha(0.7)
		, m_colorDistanceThreshold(0.0)
		, m_currentDiffIndex(-1)
		, m_diffCount(0)
	{
		for (int i = 0; i < 3; ++i)
			m_currentPage[i] = 0;
	}

	virtual ~CImgDiffBuffer()
	{
		CloseImages();
	}

	const wchar_t *GetFileName(int pane)
	{
		if (pane < 0 || pane >= m_nImages)
			return NULL;
		return m_filename[pane].c_str();
	}

	int GetPaneCount() const
	{
		return m_nImages;
	}

	Image::Color GetPixelColor(int pane, int x, int y) const
	{
		return m_imgOrig32[pane].pixel(x, y);
	}

	double GetColorDistance(int pane1, int pane2, int x, int y) const
	{
		return std::sqrt(static_cast<double>(
			::GetColorDistance2(GetPixelColor(pane1, x, y), GetPixelColor(pane2, x, y)) ));
	}

	Image::Color GetDiffColor() const
	{
		return m_diffColor;
	}

	void SetDiffColor(Image::Color clrDiffColor)
	{
		m_diffColor = clrDiffColor;
		RefreshImages();
	}

	Image::Color GetSelDiffColor() const
	{
		return m_selDiffColor;
	}

	void SetSelDiffColor(Image::Color clrSelDiffColor)
	{
		m_selDiffColor = clrSelDiffColor;
		RefreshImages();
	}

	double GetDiffColorAlpha() const
	{
		return m_diffColorAlpha;
	}

	void SetDiffColorAlpha(double diffColorAlpha)
	{
		m_diffColorAlpha = diffColorAlpha;
		RefreshImages();
	}

	int  GetCurrentPage(int pane) const
	{
		if (pane < 0 || pane >= m_nImages)
			return -1;
		return m_currentPage[pane];
	}

	void SetCurrentPage(int pane, int page)
	{
		if (page >= 0 && page < GetPageCount(pane))
		{
			if (m_imgOrigMultiPage[pane].isValid())
			{
				m_currentPage[pane] = page;
				m_imgOrig[pane] = m_imgOrigMultiPage[pane].getImage(page);
				m_imgOrig32[pane] = m_imgOrig[pane];
				m_imgOrig32[pane].convertTo32Bits();
				CompareImages();
			}
		}
	}

	void SetCurrentPageAll(int page)
	{
		for (int i = 0; i < m_nImages; ++i)
			SetCurrentPage(i, page);
	}

	int  GetCurrentMaxPage() const
	{
		int maxpage = 0;
		for (int i = 0; i < m_nImages; ++i)
		{
			int page = GetCurrentPage(i);
			maxpage = maxpage < page ? page : maxpage;
		}
		return maxpage;
	}

	int  GetPageCount(int pane) const
	{
		if (pane < 0 || pane >= m_nImages)
			return -1;
		if (m_imgOrigMultiPage[pane].isValid())
			return m_imgOrigMultiPage[pane].getPageCount();
		else
			return 1;
	}

	int  GetMaxPageCount() const
	{
		int maxpage = 0;
		for (int i = 0; i < m_nImages; ++i)
		{
			int page = GetPageCount(i);
			maxpage = page > maxpage ? page : maxpage;
		}
		return maxpage;
	}

	double GetColorDistanceThreshold() const
	{
		return m_colorDistanceThreshold;
	}

	void SetColorDistanceThreshold(double threshold)
	{
		m_colorDistanceThreshold = threshold;
		CompareImages();
	}

	int  GetDiffBlockSize() const
	{
		return m_diffBlockSize;
	}
	
	void SetDiffBlockSize(int blockSize)
	{
		m_diffBlockSize = blockSize;
		CompareImages();
	}

	OVERLAY_MODE GetOverlayMode() const
	{
		return m_overlayMode;
	}

	void SetOverlayMode(OVERLAY_MODE overlayMode)
	{
		m_overlayMode = overlayMode;
		RefreshImages();
	}

	double GetOverlayAlpha() const
	{
		return m_overlayAlpha;
	}

	void SetOverlayAlpha(double overlayAlpha)
	{
		m_overlayAlpha = overlayAlpha;
		RefreshImages();
	}

	bool GetShowDifferences() const
	{
		return m_showDifferences;
	}

	void SetShowDifferences(bool visible)
	{
		m_showDifferences = visible;
		CompareImages();
	}

	const DiffInfo *GetDiffInfo(int diffIndex) const
	{
		if (diffIndex < 0 || diffIndex >= m_diffCount)
			return NULL;
		return &m_diffInfos[diffIndex];
	}

	int  GetDiffCount() const
	{
		return m_diffCount;
	}

	int  GetConflictCount() const
	{
		int conflictCount = 0;
		for (int i = 0; i < m_diffCount; ++i)
			if (m_diffInfos[i].op == DiffInfo::OP_DIFF)
				++conflictCount;
		return conflictCount;
	}

	int  GetCurrentDiffIndex() const
	{
		return m_currentDiffIndex;
	}

	bool FirstDiff()
	{
		int oldDiffIndex = m_currentDiffIndex;
		if (m_diffCount == 0)
			m_currentDiffIndex = -1;
		else
			m_currentDiffIndex = 0;
		if (oldDiffIndex == m_currentDiffIndex)
			return false;
		RefreshImages();
		return true;
	}

	bool LastDiff()
	{
		int oldDiffIndex = m_currentDiffIndex;
		m_currentDiffIndex = m_diffCount - 1;
		if (oldDiffIndex == m_currentDiffIndex)
			return false;
		RefreshImages();
		return true;
	}

	bool NextDiff()
	{
		int oldDiffIndex = m_currentDiffIndex;
		++m_currentDiffIndex;
		if (m_currentDiffIndex >= m_diffCount)
			m_currentDiffIndex = m_diffCount - 1;
		if (oldDiffIndex == m_currentDiffIndex)
			return false;
		RefreshImages();
		return true;
	}

	bool PrevDiff()
	{
		int oldDiffIndex = m_currentDiffIndex;
		if (m_diffCount == 0)
			m_currentDiffIndex = -1;
		else
		{
			--m_currentDiffIndex;
			if (m_currentDiffIndex < 0)
				m_currentDiffIndex = 0;
		}
		if (oldDiffIndex == m_currentDiffIndex)
			return false;
		RefreshImages();
		return true;
	}

	bool FirstConflict()
	{
		int oldDiffIndex = m_currentDiffIndex;
		for (size_t i = 0; i < m_diffInfos.size(); ++i)
			if (m_diffInfos[i].op == DiffInfo::OP_DIFF)
				m_currentDiffIndex = static_cast<int>(i);
		if (oldDiffIndex == m_currentDiffIndex)
			return false;
		RefreshImages();
		return true;
	}

	bool LastConflict()
	{
		int oldDiffIndex = m_currentDiffIndex;
		for (int i = static_cast<int>(m_diffInfos.size() - 1); i >= 0; --i)
		{
			if (m_diffInfos[i].op == DiffInfo::OP_DIFF)
			{
				m_currentDiffIndex = i;
				break;
			}
		}
		if (oldDiffIndex == m_currentDiffIndex)
			return false;
		RefreshImages();
		return true;
	}

	bool NextConflict()
	{
		int oldDiffIndex = m_currentDiffIndex;
		for (size_t i = m_currentDiffIndex + 1; i < m_diffInfos.size(); ++i)
		{
			if (m_diffInfos[i].op == DiffInfo::OP_DIFF)
			{
				m_currentDiffIndex = static_cast<int>(i);
				break;
			}
		}
		if (oldDiffIndex == m_currentDiffIndex)
			return false;
		RefreshImages();
		return true;
	}

	bool PrevConflict()
	{
		int oldDiffIndex = m_currentDiffIndex;
		for (int i = m_currentDiffIndex - 1; i >= 0; --i)
		{
			if (m_diffInfos[i].op == DiffInfo::OP_DIFF)
			{
				m_currentDiffIndex = i;
				break;
			}
		}
		if (oldDiffIndex == m_currentDiffIndex)
			return false;
		RefreshImages();
		return true;
	}

	bool SelectDiff(int diffIndex)
	{
		if (diffIndex == m_currentDiffIndex || diffIndex < -1 || diffIndex >= m_diffCount)
			return false;
		m_currentDiffIndex = diffIndex;
		RefreshImages();
		return true;
	}
	
	int  GetNextDiffIndex() const
	{
		if (m_diffCount == 0 || m_currentDiffIndex >= m_diffCount - 1)
			return -1;
		return m_currentDiffIndex + 1;
	}

	int  GetPrevDiffIndex() const
	{
		if (m_diffCount == 0 || m_currentDiffIndex <= 0)
			return -1;
		return m_currentDiffIndex - 1;
	}

	int  GetNextConflictIndex() const
	{
		for (size_t i = m_currentDiffIndex + 1; i < m_diffInfos.size(); ++i)
			if (m_diffInfos[i].op == DiffInfo::OP_DIFF)
				return static_cast<int>(i);
		return -1;
	}

	int  GetPrevConflictIndex() const
	{
		for (int i = static_cast<int>(m_currentDiffIndex - 1); i >= 0; --i)
			if (m_diffInfos[i].op == DiffInfo::OP_DIFF)
				return i;
		return -1;
	}

	void CompareImages()
	{
		if (m_nImages <= 1)
			return;
		InitializeDiff();
		if (m_nImages == 2)
		{
			CompareImages2(0, 1, m_diff);
			m_diffCount = MarkDiffIndex(m_diff);
		}
		else if (m_nImages == 3)
		{
			CompareImages2(0, 1, m_diff01);
			CompareImages2(2, 1, m_diff21);
			CompareImages2(0, 2, m_diff02);
			Make3WayDiff(m_diff01, m_diff21, m_diff);
			m_diffCount = MarkDiffIndex3way(m_diff01, m_diff21, m_diff02, m_diff);
		}
		if (m_currentDiffIndex >= m_diffCount)
			m_currentDiffIndex = m_diffCount - 1;
		RefreshImages();
	}

	void RefreshImages()
	{
		if (m_nImages <= 1)
			return;
		InitializeDiffImages();
		for (int i = 0; i < m_nImages; ++i)
			CopyOriginalImageToDiffImage(i);
		void (CImgDiffBuffer::*func)(int src, int dst) = NULL;
		if (m_overlayMode == OVERLAY_ALPHABLEND)
			func = &CImgDiffBuffer::AlphaBlendImages2;
		else if (m_overlayMode == OVERLAY_XOR)
			func = &CImgDiffBuffer::XorImages2;
		if (func)
		{
			if (m_nImages == 2)
			{
				(this->*func)(1, 0);
				(this->*func)(0, 1);
			}
			else if (m_nImages == 3)
			{
				(this->*func)(1, 0);
				(this->*func)(0, 1);
				(this->*func)(2, 1);
				(this->*func)(1, 2);
			}
		}
		if (m_showDifferences)
		{
			for (int i = 0; i < m_nImages; ++i)
				MarkDiff(i, m_diff);
		}
	}

	bool OpenImages(int nImages, const wchar_t * const filename[3])
	{
		CloseImages();
		m_nImages = nImages;
		for (int i = 0; i < nImages; ++i)
			m_filename[i] = filename[i];
		return LoadImages();
	}

	virtual bool CloseImages()
	{
		for (int i = 0; i < m_nImages; ++i)
		{
			m_imgOrig[i].clear();
			m_imgOrig32[i].clear();
		}
		m_nImages = 0;
		return true;
	}

	bool SaveDiffImageAs(int pane, const wchar_t *filename)
	{
		if (pane < 0 || pane >= m_nImages)
			return false;
		return !!m_imgDiff[pane].save(filename);
	}

	int  GetImageWidth(int pane) const
	{
		if (pane < 0 || pane >= m_nImages)
			return -1;
		return m_imgOrig32[pane].width();
	}

	int  GetImageHeight(int pane) const
	{
		if (pane < 0 || pane >= m_nImages)
			return -1;
		return m_imgOrig32[pane].height();
	}

	int  GetImageBitsPerPixel(int pane) const
	{
		if (pane < 0 || pane >= m_nImages)
			return -1;
		return m_imgOrig[pane].depth();
	}

	int GetDiffIndexFromPoint(int x, int y) const
	{
		if (x > 0 && y > 0 && 
			x < static_cast<int>(m_imgDiff[0].width()) &&
			y < static_cast<int>(m_imgDiff[0].height()))
		{
			return m_diff(x / m_diffBlockSize, y / m_diffBlockSize) - 1;
		}
		return -1;
	}

	Image *GetImage(int pane)
	{
		if (pane < 0 || pane >= m_nImages)
			return NULL;
		return &m_imgDiff[pane];
	}

protected:
	bool LoadImages()
	{
		bool bSucceeded = true;
		for (int i = 0; i < m_nImages; ++i)
		{
			m_currentPage[i] = 0;
			m_imgOrigMultiPage[i].load(m_filename[i]);
			if (m_imgOrigMultiPage[i].isValid() && m_imgOrigMultiPage[i].getPageCount() > 1)
			{
				m_imgOrig[i] = m_imgOrigMultiPage[i].getImage(0);
				m_imgOrig32[i] = m_imgOrig[i];
			}
			else
			{
				m_imgOrigMultiPage[i] = MultiPageImages();
				if (!m_imgOrig[i].load(m_filename[i]))
					bSucceeded = false;
				m_imgOrig32[i] = m_imgOrig[i];
			}
			m_imgOrig32[i].convertTo32Bits();
		}
		return bSucceeded;
	}

	Size<unsigned> GetMaxWidthHeight()
	{
		unsigned wmax = 0;
		unsigned hmax = 0;
		for (int i = 0; i < m_nImages; ++i)
		{
			wmax  = (std::max)(wmax, static_cast<unsigned>(m_imgOrig32[i].width()));
			hmax = (std::max)(hmax, static_cast<unsigned>(m_imgOrig32[i].height()));
		}
		return Size<unsigned>(wmax, hmax);
	}

	void InitializeDiff()
	{
		Size<unsigned> size = GetMaxWidthHeight();
		int nBlocksX = (size.cx + m_diffBlockSize - 1) / m_diffBlockSize;
		int nBlocksY = (size.cy + m_diffBlockSize - 1) / m_diffBlockSize;

		m_diff.clear();
		m_diff.resize(nBlocksX, nBlocksY);
		if (m_nImages == 3)
		{
			m_diff01.clear();
			m_diff01.resize(nBlocksX, nBlocksY);
			m_diff21.clear();
			m_diff21.resize(nBlocksX, nBlocksY);
			m_diff02.clear();
			m_diff02.resize(nBlocksX, nBlocksY);
		}
		m_diffInfos.clear();
	}

	void InitializeDiffImages()
	{
		Size<unsigned> size = GetMaxWidthHeight();
		for (int i = 0; i < m_nImages; ++i)
			m_imgDiff[i].setSize(size.cx, size.cy);
	}

	void CompareImages2(int pane1, int pane2, DiffBlocks& diff)
	{
		unsigned w1 = m_imgOrig32[pane1].width();
		unsigned h1 = m_imgOrig32[pane1].height();
		unsigned w2 = m_imgOrig32[pane2].width();
		unsigned h2 = m_imgOrig32[pane2].height();

		const unsigned wmax = (std::max)(w1, w2);
		const unsigned hmax = (std::max)(h1, h2);

		for (unsigned by = 0; by < diff.height(); ++by)
		{
			unsigned bsy = (hmax - by * m_diffBlockSize) >= m_diffBlockSize ? m_diffBlockSize : (hmax - by * m_diffBlockSize); 
			for (unsigned i = 0; i < bsy; ++i)
			{
				unsigned y = by * m_diffBlockSize + i;
				if (y >= h1 || y >= h2)
				{
					for (unsigned bx = 0; bx < diff.width(); ++bx)
						diff(bx, by) = -1;
				}
				else
				{
					const unsigned char *scanline1 = m_imgOrig32[pane1].scanLine(y);
					const unsigned char *scanline2 = m_imgOrig32[pane2].scanLine(y);
					if (w1 == w2 && m_colorDistanceThreshold == 0.0)
					{
						if (memcmp(scanline1, scanline2, w1 * 4) == 0)
							continue;
					}
					for (unsigned x = 0; x < wmax; ++x)
					{
						if (x >= w1 || x >= w2)
							diff(x / m_diffBlockSize, by) = -1;
						else
						{
							if (m_colorDistanceThreshold > 0.0)
							{
								int bdist = scanline1[x * 4 + 0] - scanline2[x * 4 + 0];
								int gdist = scanline1[x * 4 + 1] - scanline2[x * 4 + 1];
								int rdist = scanline1[x * 4 + 2] - scanline2[x * 4 + 2];
								int adist = scanline1[x * 4 + 3] - scanline2[x * 4 + 3];
								int colorDistance2 = rdist * rdist + gdist * gdist + bdist * bdist + adist * adist;
								if (colorDistance2 > m_colorDistanceThreshold * m_colorDistanceThreshold)
									diff(x / m_diffBlockSize, by) = -1;
							}
							else
							{
								if (scanline1[x * 4 + 0] != scanline2[x * 4 + 0] ||
									scanline1[x * 4 + 1] != scanline2[x * 4 + 1] ||
									scanline1[x * 4 + 2] != scanline2[x * 4 + 2] ||
									scanline1[x * 4 + 3] != scanline2[x * 4 + 3])
								{
									diff(x / m_diffBlockSize, by) = -1;
								}
							}
						}
					}
				}
			}
		}
	}
		
	void FloodFill8Directions(DiffBlocks& data, int x, int y, unsigned val)
	{
		std::vector<Point<int> > stack;
		stack.push_back(Point<int>(x, y));
		while (!stack.empty())
		{
			const Point<int>& pt = stack.back();
			const int x = pt.x;
			const int y = pt.y;
			stack.pop_back();
			if (data(x, y) != -1)
				continue;
			data(x, y) = val;
			if (x + 1 < static_cast<int>(data.width()))
			{
				stack.push_back(Point<int>(x + 1, y));
				if (y + 1 < static_cast<int>(data.height()))
					stack.push_back(Point<int>(x + 1, y + 1));
				if (y - 1 >= 0)
					stack.push_back(Point<int>(x + 1, y - 1));
			}
			if (x - 1 >= 0)
			{
				stack.push_back(Point<int>(x - 1, y));
				if (y + 1 < static_cast<int>(data.height()))
					stack.push_back(Point<int>(x - 1, y + 1));
				if (y - 1 >= 0)
					stack.push_back(Point<int>(x - 1, y - 1));
			}
			if (y + 1 < static_cast<int>(data.height()))
				stack.push_back(Point<int>(x, y + 1));
			if (y - 1 >= 0)
				stack.push_back(Point<int>(x, y - 1));
		}
	}

	int MarkDiffIndex(DiffBlocks& diff)
	{
		int diffCount = 0;
		for (unsigned by = 0; by < diff.height(); ++by)
		{
			for (unsigned bx = 0; bx < diff.width(); ++bx)
			{
				int idx = diff(bx, by);
				if (idx == -1)
				{
					m_diffInfos.push_back(DiffInfo(DiffInfo::OP_DIFF, bx, by));
					++diffCount;
					FloodFill8Directions(diff, bx, by, diffCount);
				}
				else if (idx != 0)
				{
					Rect<int>& rc = m_diffInfos[idx - 1].rc;
					if (static_cast<int>(bx) < rc.left)
						rc.left = bx;
					else if (static_cast<int>(bx + 1) > rc.right)
						rc.right = bx + 1;
					if (static_cast<int>(by) < rc.top)
						rc.top = by;
					else if (static_cast<int>(by + 1) > rc.bottom)
						rc.bottom = by + 1;
				}
			}
		}
		return diffCount;
	}

	int MarkDiffIndex3way(DiffBlocks& diff01, DiffBlocks& diff21, DiffBlocks& diff02, DiffBlocks& diff3)
	{
		int diffCount = MarkDiffIndex(diff3);
		std::vector<DiffStat> counter(m_diffInfos.size());
		for (unsigned by = 0; by < diff3.height(); ++by)
		{
			for (unsigned bx = 0; bx < diff3.width(); ++bx)
			{
				int diffIndex = diff3(bx, by);
				if (diffIndex == 0)
					continue;
				--diffIndex;
				if (diff21(bx, by) == 0)
					++counter[diffIndex].d1;
				else if (diff02(bx, by) == 0)
					++counter[diffIndex].d2;
				else if (diff01(bx, by) == 0)
					++counter[diffIndex].d3;
				else
					++counter[diffIndex].detc;
			}
		}
		
		for (size_t i = 0; i < m_diffInfos.size(); ++i)
		{
			int op;
			if (counter[i].d1 != 0 && counter[i].d2 == 0 && counter[i].d3 == 0 && counter[i].detc == 0)
				op = DiffInfo::OP_1STONLY;
			else if (counter[i].d1 == 0 && counter[i].d2 != 0 && counter[i].d3 == 0 && counter[i].detc == 0)
				op = DiffInfo::OP_2NDONLY;
			else if (counter[i].d1 == 0 && counter[i].d2 == 0 && counter[i].d3 != 0 && counter[i].detc == 0)
				op = DiffInfo::OP_3RDONLY;
			else
				op = DiffInfo::OP_DIFF;
			m_diffInfos[i].op = op;
		}
		return diffCount;
	}

	void Make3WayDiff(const DiffBlocks& diff01, const DiffBlocks& diff21, DiffBlocks& diff3)
	{
		diff3 = diff01;
		for (unsigned bx = 0; bx < diff3.width(); ++bx)
		{
			for (unsigned by = 0; by < diff3.height(); ++by)
			{
				if (diff21(bx, by) != 0)
					diff3(bx, by) = -1;
			}
		}
	}

	void MarkDiff(int pane, const DiffBlocks& diff)
	{
		const unsigned w = m_imgDiff[pane].width();
		const unsigned h = m_imgDiff[pane].height();

		for (unsigned by = 0; by < diff.height(); ++by)
		{
			for (unsigned bx = 0; bx < diff.width(); ++bx)
			{
				int diffIndex = diff(bx, by);
				if (diffIndex != 0 && (
					(pane == 0 && m_diffInfos[diffIndex - 1].op != DiffInfo::OP_3RDONLY) ||
					(pane == 1) ||
					(pane == 2 && m_diffInfos[diffIndex - 1].op != DiffInfo::OP_1STONLY)
					))
				{
					Image::Color color = (diffIndex - 1 == m_currentDiffIndex) ? m_selDiffColor : m_diffColor;
					unsigned bsy = (h - by * m_diffBlockSize < m_diffBlockSize) ? (h - by * m_diffBlockSize) : m_diffBlockSize;
					for (unsigned i = 0; i < bsy; ++i)
					{
						unsigned y = by * m_diffBlockSize + i;
						unsigned char *scanline = m_imgDiff[pane].scanLine(y);
						unsigned bsx = (w - bx * m_diffBlockSize < m_diffBlockSize) ? (w - bx * m_diffBlockSize) : m_diffBlockSize;
						for (unsigned j = 0; j < bsx; ++j)
						{
							unsigned x = bx * m_diffBlockSize + j;
							scanline[x * 4 + 0] = static_cast<unsigned char>(scanline[x * 4 + 0] * (1 - m_diffColorAlpha) + Image::valueB(color) * m_diffColorAlpha);
							scanline[x * 4 + 1] = static_cast<unsigned char>(scanline[x * 4 + 1] * (1 - m_diffColorAlpha) + Image::valueG(color) * m_diffColorAlpha);
							scanline[x * 4 + 2] = static_cast<unsigned char>(scanline[x * 4 + 2] * (1 - m_diffColorAlpha) + Image::valueR(color) * m_diffColorAlpha);
						}
					}
				}
			}
		}
	}

	void CopyOriginalImageToDiffImage(int dst)
	{
		unsigned w = m_imgOrig32[dst].width();
		unsigned h = m_imgOrig32[dst].height();
		for (unsigned y = 0; y < h; ++y)
		{
			const unsigned char *scanline_src = m_imgOrig32[dst].scanLine(y);
			unsigned char *scanline_dst = m_imgDiff[dst].scanLine(y);
			for (unsigned x = 0; x < w; ++x)
			{
				scanline_dst[x * 4 + 0] = scanline_src[x * 4 + 0];
				scanline_dst[x * 4 + 1] = scanline_src[x * 4 + 1];
				scanline_dst[x * 4 + 2] = scanline_src[x * 4 + 2];
				scanline_dst[x * 4 + 3] = scanline_src[x * 4 + 3];
			}
		}	
	}

	void XorImages2(int src, int dst)
	{
		unsigned w = m_imgOrig32[src].width();
		unsigned h = m_imgOrig32[src].height();
		for (unsigned y = 0; y < h; ++y)
		{
			const unsigned char *scanline_src = m_imgOrig32[src].scanLine(y);
			unsigned char *scanline_dst = m_imgDiff[dst].scanLine(y);
			for (unsigned x = 0; x < w; ++x)
			{
				scanline_dst[x * 4 + 0] ^= scanline_src[x * 4 + 0];
				scanline_dst[x * 4 + 1] ^= scanline_src[x * 4 + 1];
				scanline_dst[x * 4 + 2] ^= scanline_src[x * 4 + 2];
			}
		}	
	}

	void AlphaBlendImages2(int src, int dst)
	{
		unsigned w = m_imgOrig32[src].width();
		unsigned h = m_imgOrig32[src].height();
		for (unsigned y = 0; y < h; ++y)
		{
			const unsigned char *scanline_src = m_imgOrig32[src].scanLine(y);
			unsigned char *scanline_dst = m_imgDiff[dst].scanLine(y);
			for (unsigned x = 0; x < w; ++x)
			{
				scanline_dst[x * 4 + 0] = static_cast<unsigned char>(scanline_dst[x * 4 + 0] * (1 - m_overlayAlpha) + scanline_src[x * 4 + 0] * m_overlayAlpha);
				scanline_dst[x * 4 + 1] = static_cast<unsigned char>(scanline_dst[x * 4 + 1] * (1 - m_overlayAlpha) + scanline_src[x * 4 + 1] * m_overlayAlpha);
				scanline_dst[x * 4 + 2] = static_cast<unsigned char>(scanline_dst[x * 4 + 2] * (1 - m_overlayAlpha) + scanline_src[x * 4 + 2] * m_overlayAlpha);
				scanline_dst[x * 4 + 3] = static_cast<unsigned char>(scanline_dst[x * 4 + 3] * (1 - m_overlayAlpha) + scanline_src[x * 4 + 3] * m_overlayAlpha);
			}
		}	
	}

	int m_nImages;
	MultiPageImages m_imgOrigMultiPage[3];
	Image m_imgOrig[3];
	Image m_imgOrig32[3];
	Image m_imgDiff[3];
	std::wstring m_filename[3];
	bool m_showDifferences;
	OVERLAY_MODE m_overlayMode;
	double m_overlayAlpha;
	unsigned m_diffBlockSize;
	Image::Color m_selDiffColor;
	Image::Color m_diffColor;
	double m_diffColorAlpha;
	double m_colorDistanceThreshold;
	int m_currentPage[3];
	int m_currentDiffIndex;
	int m_diffCount;
	DiffBlocks m_diff, m_diff01, m_diff21, m_diff02;
	std::vector<DiffInfo> m_diffInfos;
};

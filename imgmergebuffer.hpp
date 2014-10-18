#pragma once

#include "imgdiffbuffer.hpp"

struct UndoRecord
{
	UndoRecord(int pane, Image *oldbitmap, Image *newbitmap, const int modcountnew[3]) : 
		pane(pane), oldbitmap(oldbitmap), newbitmap(newbitmap)
	{
		for (int i = 0; i < 3; ++i)
			modcount[i] = modcountnew[i];
	}
	int pane;
	int modcount[3];
	Image *oldbitmap, *newbitmap;
};

struct UndoRecords
{
	UndoRecords() : m_currentUndoBufIndex(-1)
	{
		clear();
	}

	~UndoRecords()
	{
		clear();
	}

	void push_back(int pane, Image *oldbitmap, Image *newbitmap)
	{
		++m_currentUndoBufIndex;
		while (m_currentUndoBufIndex < static_cast<int>(m_undoBuf.size()))
		{
			--m_modcount[m_undoBuf.back().pane];
			delete m_undoBuf.back().newbitmap;
			delete m_undoBuf.back().oldbitmap;
			m_undoBuf.pop_back();
		}
		++m_modcount[pane];
		m_undoBuf.push_back(UndoRecord(pane, oldbitmap, newbitmap, m_modcount));
	}

	const UndoRecord& undo()
	{
		if (m_currentUndoBufIndex < 0)
			throw "no undoable";
		const UndoRecord& rec = m_undoBuf[m_currentUndoBufIndex];
		--m_currentUndoBufIndex;
		return rec;
	}

	const UndoRecord& redo()
	{
		if (m_currentUndoBufIndex >= static_cast<int>(m_undoBuf.size()) - 1)
			throw "no redoable";
		++m_currentUndoBufIndex;
		const UndoRecord& rec = m_undoBuf[m_currentUndoBufIndex];
		return rec;
	}

	bool is_modified(int pane) const
	{
		if (m_currentUndoBufIndex < 0)
			return (m_modcountonsave[pane] != 0);
		else
			return (m_modcountonsave[pane] != m_undoBuf[m_currentUndoBufIndex].modcount[pane]);
	}

	void save(int pane)
	{
		if (m_currentUndoBufIndex < 0)
			m_modcountonsave[pane] = 0;
		else
			m_modcountonsave[pane] = m_undoBuf[m_currentUndoBufIndex].modcount[pane];
	}

	bool undoable() const
	{
		return (m_currentUndoBufIndex >= 0);
	}

	bool redoable() const
	{
		return (m_currentUndoBufIndex < static_cast<int>(m_undoBuf.size()) - 1);
	}

	void clear()
	{
		m_currentUndoBufIndex = -1;
		for (int i = 0; i < 3; ++i)
		{
			m_modcount[i] = 0;
			m_modcountonsave[i] = 0;
		}
		while (!m_undoBuf.empty())
		{
			delete m_undoBuf.back().newbitmap;
			delete m_undoBuf.back().oldbitmap;
			m_undoBuf.pop_back();
		}
	}

	std::vector<UndoRecord> m_undoBuf;
	int m_currentUndoBufIndex;
	int m_modcount[3];
	int m_modcountonsave[3];
};

class CImgMergeBuffer : public CImgDiffBuffer
{
public:
	CImgMergeBuffer()
	{
		for (int i = 0; i < 3; ++i)
			m_bRO[i] = false;
	}

	virtual ~CImgMergeBuffer()
	{
	}

	bool GetReadOnly(int pane) const
	{
		if (pane < 0 || pane >= m_nImages)
			return true;
		return m_bRO[pane];
	}

	void SetReadOnly(int pane, bool readOnly)
	{
		if (pane < 0 || pane >= m_nImages)
			return;
		m_bRO[pane] = readOnly;
	}

	void CopyDiffInternal(int diffIndex, int srcPane, int dstPane)
	{
		if (srcPane < 0 || srcPane >= m_nImages)
			return;
		if (dstPane < 0 || dstPane >= m_nImages)
			return;
		if (diffIndex < 0 || diffIndex >= m_diffCount)
			return;
		if (m_bRO[dstPane])
			return;

		const Rect<int>& rc = m_diffInfos[diffIndex].rc;
		unsigned wsrc = m_imgOrig32[srcPane].width();
		unsigned hsrc = m_imgOrig32[srcPane].height();
		unsigned wdst = m_imgOrig32[dstPane].width();
		unsigned hdst = m_imgOrig32[dstPane].height();
		if (rc.right * m_diffBlockSize > wdst)
		{
			if ((std::max)(wsrc, wdst) < rc.right * m_diffBlockSize)
				wdst = (std::max)(wsrc, wdst);
			else
				wdst = rc.right * m_diffBlockSize;
		}
		if (rc.bottom * m_diffBlockSize > hdst)
		{
			if ((std::max)(hsrc, hdst) < rc.bottom * m_diffBlockSize)
				hdst = (std::max)(hsrc, hdst);
			else
				hdst = rc.bottom * m_diffBlockSize;
		}
		if (wdst != m_imgOrig32[dstPane].width() || hdst != m_imgOrig32[dstPane].height())
		{
			Image imgTemp = m_imgOrig32[dstPane];
			m_imgOrig32[dstPane].setSize(wdst, hdst);
			m_imgOrig32[dstPane].pasteSubImage(imgTemp, 0, 0);
		}
		
		for (unsigned y = rc.top * m_diffBlockSize; y < rc.bottom * m_diffBlockSize; y += m_diffBlockSize)
		{
			for (unsigned x = rc.left * m_diffBlockSize; x < rc.right * m_diffBlockSize; x += m_diffBlockSize)
			{
				if (m_diff(x / m_diffBlockSize, y / m_diffBlockSize) == diffIndex + 1)
				{
					int sizex = ((x + m_diffBlockSize) < wsrc) ? m_diffBlockSize : (wsrc - x);
					int sizey = ((y + m_diffBlockSize) < hsrc) ? m_diffBlockSize : (hsrc - y);
					if (sizex > 0 && sizey > 0)
					{
						for (int i = 0; i < sizey; ++i)
						{
							const unsigned char *scanline_src = m_imgOrig32[srcPane].scanLine(y + i);
							unsigned char *scanline_dst = m_imgOrig32[dstPane].scanLine(y + i);
							memcpy(&scanline_dst[x * 4], &scanline_src[x * 4], sizex * 4);
						}
					}
				}
			}
		}
	}

	void CopyDiff(int diffIndex, int srcPane, int dstPane)
	{
		if (srcPane < 0 || srcPane >= m_nImages)
			return;
		if (dstPane < 0 || dstPane >= m_nImages)
			return;
		if (diffIndex < 0 || diffIndex >= m_diffCount)
			return;
		if (m_bRO[dstPane])
			return;
		if (srcPane == dstPane)
			return;

		Image *oldbitmap = new Image(m_imgOrig32[dstPane]);

		CopyDiffInternal(diffIndex, srcPane, dstPane);

		Image *newbitmap = new Image(m_imgOrig32[dstPane]);
		m_undoRecords.push_back(dstPane, oldbitmap, newbitmap);
		CompareImages();
	}

	void CopyDiffAll(int srcPane, int dstPane)
	{
		if (srcPane < 0 || srcPane >= m_nImages)
			return;
		if (dstPane < 0 || dstPane >= m_nImages)
			return;
		if (m_bRO[dstPane])
			return;
		if (srcPane == dstPane)
			return;

		Image *oldbitmap = new Image(m_imgOrig32[dstPane]);

		for (int diffIndex = 0; diffIndex < m_diffCount; ++diffIndex)
			CopyDiffInternal(diffIndex, srcPane, dstPane);

		Image *newbitmap = new Image(m_imgOrig32[dstPane]);
		m_undoRecords.push_back(dstPane, oldbitmap, newbitmap);
		CompareImages();
	}

	int CopyDiff3Way(int dstPane)
	{
		if (dstPane < 0 || dstPane >= m_nImages)
			return 0;
		if (m_bRO[dstPane])
			return 0;

		Image *oldbitmap = new Image(m_imgOrig32[dstPane]);

		int nMerged = 0;
		for (int diffIndex = 0; diffIndex < m_diffCount; ++diffIndex)
		{
			int srcPane;
			switch (m_diffInfos[diffIndex].op)
			{
			case DiffInfo::OP_1STONLY:
				if (dstPane == 1)
					srcPane = 0;
				else
					srcPane = -1;
				break;
			case DiffInfo::OP_2NDONLY:
				if (dstPane != 1)
					srcPane = 1;
				else
					srcPane = -1;
				break;
			case DiffInfo::OP_3RDONLY:
				if (dstPane == 1)
					srcPane = 2;
				else
					srcPane = -1;
				break;
			case DiffInfo::OP_DIFF:
				srcPane = -1;
				break;
			}

			if (srcPane >= 0)
			{
				CopyDiffInternal(diffIndex, srcPane, dstPane);
				++nMerged;
			}
		}

		Image *newbitmap = new Image(m_imgOrig32[dstPane]);
		m_undoRecords.push_back(dstPane, oldbitmap, newbitmap);
		CompareImages();

		return nMerged;
	}

	bool IsModified(int pane) const
	{
		return m_undoRecords.is_modified(pane);
	}

	bool IsUndoable() const
	{
		return m_undoRecords.undoable();
	}

	bool IsRedoable() const
	{
		return m_undoRecords.redoable();
	}

	bool Undo()
	{
		if (!m_undoRecords.undoable())
			return false;
		const UndoRecord& rec = m_undoRecords.undo();
		m_imgOrig32[rec.pane] = *rec.oldbitmap;
		CompareImages();
		return true;
	}

	bool Redo()
	{
		if (!m_undoRecords.redoable())
			return false;
		const UndoRecord& rec = m_undoRecords.redo();
		m_imgOrig32[rec.pane] = *rec.newbitmap;
		CompareImages();
		return true;
	}

	bool SaveImage(int pane)
	{
		if (pane < 0 || pane >= m_nImages)
			return false;
		if (m_bRO[pane])
			return false;
		if (!m_undoRecords.is_modified(pane))
			return true;
		bool result = SaveImageAs(pane, m_filename[pane].c_str());
		if (result)
			m_undoRecords.save(pane);
		return result;
	}

	bool SaveImages()
	{
		for (int i = 0; i < m_nImages; ++i)
			if (!SaveImage(i))
				return false;
		return true;
	}

	bool SaveImageAs(int pane, const wchar_t *filename)
	{
		if (pane < 0 || pane >= m_nImages)
			return false;
		m_imgOrig[pane].pullImageKeepingBPP(m_imgOrig32[pane]);
		if (m_imgOrigMultiPage[pane].isValid())
		{
			m_imgOrigMultiPage[pane].replacePage(m_currentPage[pane], m_imgOrig[pane]);
			return m_imgOrigMultiPage[pane].save(filename);
		}
		else
		{
			return m_imgOrig[pane].save(filename);
		}
	}

	virtual bool CloseImages()
	{
		for (int i = 0; i < m_nImages; ++i)
			m_undoRecords.clear();
		return CImgDiffBuffer::CloseImages();
	}

private:
	bool m_bRO[3];
	UndoRecords m_undoRecords;
};


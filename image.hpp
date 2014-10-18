#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QPainter>
#include <QList>
#include <string>

class Image
{
public:
    typedef QRgb Color;
    Image() {}
    Image(int w, int h) : m_image(w, h, QImage::Format_ARGB32) {}
    Image(const Image& other) { m_image = other.m_image; }
    Image(const QImage& image) { m_image = image; }
    unsigned char *scanLine(int y) { return m_image.scanLine(y); }
    const unsigned char *scanLine(int y) const { return m_image.scanLine(y); }
    bool convertTo32Bits()
    {
        m_image = m_image.convertToFormat(QImage::Format_ARGB32);
        return true;
    }
    bool load(const std::wstring& filename) { return m_image.load(QString::fromStdWString(filename)); }
    bool save(const std::wstring& filename) { return m_image.save(QString::fromStdWString(filename)); }
    int depth() const { return m_image.depth(); }
    unsigned width() const  { return m_image.width(); }
    unsigned height() const { return m_image.height(); }
    void clear() { m_image = QImage(); }
    void setSize(int w, int h) { m_image = QImage(w, h, QImage::Format_ARGB32); }
    QImage getQImage() const { return m_image; }
    Color pixel(int x, int y) const { return m_image.pixel(x, y); }
    bool pasteSubImage(const Image& srcImage, int x, int y)
    {
        QPainter painter(&m_image);
        painter.drawImage(QPoint(x, y), srcImage.m_image);
        painter.end();
        return true;
    }
    bool pullImageKeepingBPP(const Image& other)
    {
        QImage::Format format = m_image.format();
        QVector<QRgb> colorTable = m_image.colorTable();
        m_image = other.m_image;
        m_image.convertToFormat(format, colorTable);
        return true;
    }

    static int valueR(Color color) { return qRed(color); }
    static int valueG(Color color) { return qGreen(color); }
    static int valueB(Color color) { return qBlue(color); }
    static int valueA(Color color) { return qAlpha(color); }
    static Color Rgb(int r, int g, int b) { return qRgb(r, g, b); }
private:
    QImage m_image;
};

class MultiPageImages
{
public:
    bool isValid() const { return m_images.size() > 0; }
    int getPageCount() const { return m_images.size(); }
    bool load(const std::wstring& filename)
    {
        QImageReader reader(QString::fromStdWString(filename));
        if (!reader.canRead())
            return false;
        for (int i = 0; i < reader.imageCount(); ++i)
        {
            reader.jumpToNextImage();
            QImage image = reader.read();
            if (image.isNull())
            {
                m_images.clear();
                return false;
            }
            m_images.append(Image(image));
        }
        return true;
    }
    bool save(const std::wstring& filename)
    {
        Q_UNUSED(filename);
        return false;
    }

    bool replacePage(int page, const Image& image)
    {
        m_images[page] = image;
        return true;
    }

    Image getImage(int page) const { return m_images[page]; }

    QList<Image> m_images;
};

#endif // IMAGE_HPP

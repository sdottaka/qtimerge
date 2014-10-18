#include "imgmergeform.h"
#include "ui_imgmergeform.h"
#include <QScrollBar>
#include <QFileInfo>
#include <QGraphicsSceneEvent>
#include <QWheelEvent>
#include <QRgb>
#include <QLinearGradient>
#include <string>

const int MARGIN = 16;
const QColor BORDER_COLOR(206, 215, 230);

ImgMergeForm::ImgMergeForm(QWidget *parent) :
    QWidget(parent),
    m_scaleFactor(1.0),
    m_useBackgroundColor(true),
    m_backgroundColor(QColor(255, 255, 255)),
    ui(new Ui::ImgMergeForm)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);

    m_view[0] = ui->graphicsViewLeft;
    m_view[1] = ui->graphicsViewRight;

    QObject *pobj1[2] = {m_view[0]->horizontalScrollBar(), m_view[1]->horizontalScrollBar()};
    connect(pobj1[0], SIGNAL(valueChanged(int)), pobj1[1], SLOT(setValue(int)));
    connect(pobj1[1], SIGNAL(valueChanged(int)), pobj1[0], SLOT(setValue(int)));

    QObject *pobj2[2] = {m_view[0]->verticalScrollBar(), m_view[1]->verticalScrollBar()};
    connect(pobj2[0], SIGNAL(valueChanged(int)), pobj2[1], SLOT(setValue(int)));
    connect(pobj2[1], SIGNAL(valueChanged(int)), pobj2[0], SLOT(setValue(int)));

    for (int i = 0; i < 2; ++i)
    {
        connect(&m_scene[i], SIGNAL(mousePress(QGraphicsSceneMouseEvent*)), this, SLOT(onSceneMousePressEvent(QGraphicsSceneMouseEvent*)));
        connect(&m_scene[i], SIGNAL(mouseDoubleClick(QGraphicsSceneMouseEvent*)), this, SLOT(onSceneMouseDoubleClickEvent(QGraphicsSceneMouseEvent*)));
        connect(&m_scene[i], SIGNAL(mouseMove(QGraphicsSceneMouseEvent*)), this, SLOT(onSceneMouseMoveEvent(QGraphicsSceneMouseEvent*)));
        connect(&m_scene[i], SIGNAL(wheel(QGraphicsSceneWheelEvent*)), this, SLOT(onSceneWheelEvent(QGraphicsSceneWheelEvent*)));
    }

    for (int i = 0; i < 2; ++i)
    {
        m_scene[i].setBackgroundBrush(QBrush(BORDER_COLOR));
        m_view[i]->setMouseTracking(true);
    }
}

ImgMergeForm::~ImgMergeForm()
{
    delete ui;
}

bool ImgMergeForm::loadImages(const QStringList &paths)
{
    std::wstring buf[2] = {paths[0].toStdWString(), paths[1].toStdWString()};
    const wchar_t *pathsW[2] = {buf[0].c_str(), buf[1].c_str()};
    bool result = m_buffer.OpenImages(2, pathsW);
    if (!result)
        return result;
    m_buffer.CompareImages();
    refreshImages();
    updateTitle();
    updateStatusBar();
    updateDock();
    ui->spinBoxLeft->setRange(1, m_buffer.GetPageCount(0));
    ui->spinBoxRight->setRange(1, m_buffer.GetPageCount(1));
    ui->spinBoxPage->setRange(1, (std::max)(m_buffer.GetPageCount(0), m_buffer.GetPageCount(1)));
    emit contentsChanged();
    return true;
}

bool ImgMergeForm::saveImages()
{
    bool result = m_buffer.SaveImages();
    updateTitle();
    emit contentsChanged();
    return result;
}

bool ImgMergeForm::modified() const
{
    for (int i = 0; i < m_buffer.GetPaneCount(); ++i)
        if (m_buffer.IsModified(i))
            return true;
    return false;
}

bool ImgMergeForm::canUndo() const
{
    return m_buffer.IsUndoable();
}

bool ImgMergeForm::canRedo() const
{
    return m_buffer.IsRedoable();
}

void ImgMergeForm::undo()
{
    m_buffer.Undo();
    refreshImages();
    updateTitle();
    emit diffSelectionChanged();
    emit contentsChanged();
}

void ImgMergeForm::redo()
{
    m_buffer.Redo();
    refreshImages();
    updateTitle();
    emit diffSelectionChanged();
    emit contentsChanged();
}

int ImgMergeForm::getDiffCount() const
{
    return m_buffer.GetDiffCount();
}

int ImgMergeForm::getCurrentDiffIndex() const
{
    return m_buffer.GetCurrentDiffIndex();
}

void ImgMergeForm::nextDiff()
{
    if (!m_buffer.NextDiff())
        return;
    refreshImages();
    scrollToDiff(m_buffer.GetCurrentDiffIndex());
    emit diffSelectionChanged();
    emit imageAppearanceChanged();
}

void ImgMergeForm::prevDiff()
{
    if (!m_buffer.PrevDiff())
        return;
    refreshImages();
    scrollToDiff(m_buffer.GetCurrentDiffIndex());
    emit diffSelectionChanged();
    emit imageAppearanceChanged();
}

void ImgMergeForm::firstDiff()
{
    if (!m_buffer.FirstDiff())
        return;
    refreshImages();
    scrollToDiff(m_buffer.GetCurrentDiffIndex());
    emit diffSelectionChanged();
    emit imageAppearanceChanged();
}

void ImgMergeForm::lastDiff()
{
    if (!m_buffer.LastDiff())
        return;
    refreshImages();
    scrollToDiff(m_buffer.GetCurrentDiffIndex());
    emit diffSelectionChanged();
    emit imageAppearanceChanged();
}

void ImgMergeForm::copyToRight()
{
    m_buffer.CopyDiff(m_buffer.GetCurrentDiffIndex(), 0, 1);
    refreshImages();
    scrollToDiff(m_buffer.GetCurrentDiffIndex());
    updateTitle();
    emit diffSelectionChanged();
    emit imageAppearanceChanged();
    emit contentsChanged();
}

void ImgMergeForm::copyToLeft()
{
    m_buffer.CopyDiff(m_buffer.GetCurrentDiffIndex(), 1, 0);
    refreshImages();
    scrollToDiff(m_buffer.GetCurrentDiffIndex());
    updateTitle();
    emit diffSelectionChanged();
    emit imageAppearanceChanged();
    emit contentsChanged();
}

void ImgMergeForm::copyAllToRight()
{
    m_buffer.CopyDiffAll(0, 1);
    refreshImages();
    updateTitle();
    emit diffSelectionChanged();
    emit imageAppearanceChanged();
    emit contentsChanged();
}

void ImgMergeForm::copyAllToLeft()
{
    m_buffer.CopyDiffAll(1, 0);
    refreshImages();
    updateTitle();
    emit diffSelectionChanged();
    emit imageAppearanceChanged();
    emit contentsChanged();
}

bool ImgMergeForm::getShowDifferences() const
{
    return m_buffer.GetShowDifferences();
}

void ImgMergeForm::setShowDifferences(bool visible)
{
    m_buffer.SetShowDifferences(visible);
    refreshImages();
    updateDock();
    emit imageAppearanceChanged();
}

int ImgMergeForm::getDiffBlockSize() const
{
    return m_buffer.GetDiffBlockSize();
}

void ImgMergeForm::setDiffBlockSize(int blockSize)
{
    m_buffer.SetDiffBlockSize(blockSize);
    refreshImages();
    updateDock();
    emit imageAppearanceChanged();
}

double ImgMergeForm::getDiffColorAlpha() const
{
    return m_buffer.GetDiffColorAlpha();
}

void ImgMergeForm::setDiffColorAlpha(double alpha)
{
    m_buffer.SetDiffColorAlpha(alpha);
    refreshImages();
    updateDock();
    emit imageAppearanceChanged();
}

double ImgMergeForm::getColorDistanceThreshold() const
{
    return m_buffer.GetColorDistanceThreshold();
}

void ImgMergeForm::setColorDistanceThreshold(double threshold)
{
    m_buffer.SetColorDistanceThreshold(threshold);
    refreshImages();
    updateDock();
    emit imageAppearanceChanged();
}

int ImgMergeForm::getActivePane() const
{
    for (int i = 0; i < m_buffer.GetPaneCount(); ++i)
        if (m_view[i] == focusWidget())
            return i;
    return -1;
}

void ImgMergeForm::setActivePane(int pane)
{
    if (pane < 0 || pane >= m_buffer.GetPaneCount())
        return;
    m_view[pane]->setFocus();
}

int ImgMergeForm::getCurrentMaxPage() const
{
    return m_buffer.GetCurrentMaxPage();
}

void ImgMergeForm::setCurrentPageAll(int page)
{
    m_buffer.SetCurrentPageAll(page);
    refreshImages();
    updateStatusBar();
    emit diffSelectionChanged();
    emit imageAppearanceChanged();
}

int ImgMergeForm::getCurrentPage(int pane) const
{
    return m_buffer.GetCurrentPage(pane);
}

void ImgMergeForm::setCurrentPage(int pane, int page)
{
    m_buffer.SetCurrentPage(pane, page);
    refreshImages();
    updateStatusBar();
    updateDock();
    emit diffSelectionChanged();
    emit imageAppearanceChanged();
}

double ImgMergeForm::getZoom() const
{
    return m_scaleFactor;
}

QColor ImgMergeForm::getBackgroundColor() const
{
    return m_backgroundColor;
}

void ImgMergeForm::setBackgroundColor(QColor color)
{
    m_backgroundColor = color;
    refreshImages();
    emit imageAppearanceChanged();
}

bool ImgMergeForm::getUseBackgroundColor() const
{
    return m_useBackgroundColor;
}

void ImgMergeForm::setUseBackgroundColor(bool useBackgroundColor)
{
    m_useBackgroundColor = useBackgroundColor;
    refreshImages();
    emit imageAppearanceChanged();
}

void ImgMergeForm::setZoom(double factor)
{
    scaleImages(factor / m_scaleFactor);
    updateStatusBar();
    updateDock();
    emit imageAppearanceChanged();
}

CImgDiffBuffer::OVERLAY_MODE ImgMergeForm::getOverlayMode() const
{
    return m_buffer.GetOverlayMode();
}

void ImgMergeForm::setOverlayMode(CImgDiffBuffer::OVERLAY_MODE mode)
{
    m_buffer.SetOverlayMode(mode);
    refreshImages();
    updateDock();
    emit imageAppearanceChanged();
}

double ImgMergeForm::getOverlayAlpha() const
{
    return m_buffer.GetOverlayAlpha();
}

void ImgMergeForm::setOverlayAlpha(double alpha)
{
    m_buffer.SetOverlayAlpha(alpha);
    refreshImages();
    updateDock();
    emit imageAppearanceChanged();
}

void ImgMergeForm::updateTitle()
{
    QStringList title;
    for (int i = 0; i < m_buffer.GetPaneCount(); ++i)
    {
        QString filepath = QString::fromStdWString(m_buffer.GetFileName(i));
        title.append(QFileInfo(filepath).fileName());
        QLabel *widget = i == 0 ? ui->labelTitleLeft : ui->labelTitleRight;
        widget->setText((m_buffer.IsModified(i) ? "* " : "") + filepath);
        widget->setStyleSheet(
            (focusWidget() == m_view[i]) ?
                  "QLabel { background-color: rgba(170, 170, 255); }"
                : "QLabel { background-color: rgba(200, 200, 255); }");
    }
    setWindowTitle(title.join(" - "));
}

void ImgMergeForm::updateDock()
{
    ui->checkBoxShowDifferences->setChecked(getShowDifferences());
    ui->horizontalSliderDiffBlockSize->setValue(getDiffBlockSize());
    ui->horizontalSliderDiffColorAlpha->setValue(getDiffColorAlpha() * 100);
    ui->horizontalSliderThreshold->setValue(getColorDistanceThreshold());
    ui->comboBoxOverlayMode->setCurrentIndex(getOverlayMode());
    ui->horizontalSliderOverlayAlpha->setValue(getOverlayAlpha() * 100);
    ui->horizontalSliderZoom->setValue((getZoom() - 1.0) / 0.125);
    ui->spinBoxPage->setValue(getCurrentMaxPage() + 1);
}

void ImgMergeForm::updateStatusBar()
{
    QPoint pt(-1, -1);
    for (int i = 0; i < m_buffer.GetPaneCount(); ++i)
    {
        QPoint ptView = m_view[i]->mapFromGlobal(QCursor::pos());
        if (ptView.x() < 0 || ptView.x() >= m_view[i]->geometry().width() ||
            ptView.y() < 0 || ptView.y() >= m_view[i]->geometry().height())
            continue;
        QPointF ptTmp = m_view[i]->mapToScene(ptView);
        ptTmp -= QPointF(MARGIN, MARGIN);
        if (ptTmp.x() >= 0 && ptTmp.x() < m_buffer.GetImageWidth(0) &&
            ptTmp.y() >= 0 && ptTmp.y() < m_buffer.GetImageHeight(0))
        {
            pt = QPoint(ptTmp.x(), ptTmp.y());
            break;
        }
    }
    QLabel *label[2] = {ui->labelStatusLeft, ui->labelStatusRight};
    QSpinBox *spinBox[2] = {ui->spinBoxLeft, ui->spinBoxRight};
    for (int i = 0; i < m_buffer.GetPaneCount(); ++i)
    {
        QString text;
        if (pt.x() >= 0 && pt.y() >= 0)
        {
            Image::Color clr = m_buffer.GetPixelColor(i, pt.x(), pt.y());
            text += QString("Pt:(%1,%2) RGBA:(%3,%4,%5,%6) Dist:%7 ")
                    .arg(pt.x()).arg(pt.y())
                    .arg(qRed(clr)).arg(qGreen(clr)).arg(qBlue(clr)).arg(qAlpha(clr))
                    .arg(m_buffer.GetColorDistance(0, 1, pt.x(), pt.y()));
        }
        text += QString("Page:%1/%2 Zoom:%3% %4x%5 %6bpp")
                .arg(m_buffer.GetCurrentPage(i) + 1)
                .arg(m_buffer.GetPageCount(i))
                .arg(m_scaleFactor * 100)
                .arg(m_buffer.GetImageWidth(i))
                .arg(m_buffer.GetImageHeight(i))
                .arg(m_buffer.GetImageBitsPerPixel(i));
        label[i]->setText(text);
        spinBox[i]->setValue(m_buffer.GetCurrentPage(i) + 1);
    }
}

void ImgMergeForm::scaleImages(double factor)
{
    m_scaleFactor *= factor;
    m_view[0]->scale(factor, factor);
    m_view[1]->scale(factor, factor);
}

static void drawShadow(QPainter& painter, const QRect& rect, int shadowWidth, const QColor& color)
{
    QLinearGradient gradientLeft(rect.left() - shadowWidth, rect.top(), rect.left(), rect.top());
    gradientLeft.setColorAt(0, color);
    gradientLeft.setColorAt(1, color.darker(110));
    painter.fillRect(rect.left() - shadowWidth, rect.top() - shadowWidth, shadowWidth, rect.height() + shadowWidth * 2, QBrush(gradientLeft));

    QLinearGradient gradientRight(rect.right() + 1, rect.top(), rect.right() + 1 + shadowWidth, rect.top());
    gradientRight.setColorAt(0, color.darker(110));
    gradientRight.setColorAt(1, color);
    painter.fillRect(rect.right() + 1, rect.top() - shadowWidth, shadowWidth, rect.height() + shadowWidth * 2, QBrush(gradientRight));

    QLinearGradient gradientTop(rect.left(), rect.top() - shadowWidth, rect.left(), rect.top());
    gradientTop.setColorAt(0, color);
    gradientTop.setColorAt(1, color.darker(110));
    QPolygon polygonTop;
    polygonTop << QPoint(rect.left() - shadowWidth, rect.top() - shadowWidth)
               << QPoint(rect.left(), rect.top())
               << QPoint(rect.right(), rect.top())
               << QPoint(rect.right() + shadowWidth, rect.top() - shadowWidth);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(gradientTop));
    painter.drawPolygon(polygonTop);

    QLinearGradient gradientBottom(rect.left(), rect.bottom() + 1, rect.left(), rect.bottom() + 1 + shadowWidth);
    gradientBottom.setColorAt(0, color.darker(110));
    gradientBottom.setColorAt(1, color);
    QPolygon polygonBottom;
    polygonBottom << QPoint(rect.left() - shadowWidth, rect.bottom() + 1 + shadowWidth)
                  << QPoint(rect.left(), rect.bottom() + 1)
                  << QPoint(rect.right() + 1, rect.bottom() + 1)
                  << QPoint(rect.right() + 1 + shadowWidth, rect.bottom() + 1 + shadowWidth);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(gradientBottom));
    painter.drawPolygon(polygonBottom);
}

void ImgMergeForm::refreshImages()
{
    QPixmap checkered(":/images/images/checkered.png");
    for (int i = 0; i < 2; ++i)
    {
        QImage image = m_buffer.GetImage(i)->getQImage();
        const int w = image.width() + MARGIN * 2;
        const int h = image.height() + MARGIN * 2;
        QPixmap pixmap(w, h);
        QPainter painter(&pixmap);
        painter.fillRect(0, 0, w, h, QBrush(BORDER_COLOR));
        if (m_useBackgroundColor)
        {
            painter.fillRect(MARGIN, MARGIN, w - MARGIN * 2, h - MARGIN * 2, QBrush(m_backgroundColor));
        }
        else
        {
            painter.setClipRect(MARGIN, MARGIN, w - MARGIN * 2, h - MARGIN * 2);
            for (int y = MARGIN; y < h - MARGIN; y += 16)
                for (int x = MARGIN; x < w - MARGIN; x += 16)
                    painter.drawPixmap(x, y, checkered);
            painter.setClipping(false);
        }

        painter.drawPixmap(MARGIN, MARGIN, QPixmap::fromImage(image));
        drawShadow(painter, QRect(MARGIN, MARGIN, w - MARGIN * 2, h - MARGIN * 2), MARGIN, BORDER_COLOR);
        painter.end();

        m_scene[i].clear();
        m_scene[i].addPixmap(pixmap);
        m_view[i]->setScene(&m_scene[i]);
    }
}

void ImgMergeForm::scrollToDiff(int diffIndex)
{
    const DiffInfo *info = m_buffer.GetDiffInfo(diffIndex);
    if (info)
    {
        int left = info->rc.left * m_buffer.GetDiffBlockSize();
        int top  = info->rc.top * m_buffer.GetDiffBlockSize();
        m_view[0]->ensureVisible(left, top, 1, 1);
    }
}

void ImgMergeForm::onSceneMousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    updateTitle();
}

void ImgMergeForm::onSceneMouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF pt = event->scenePos();
    int diffIndex = m_buffer.GetDiffIndexFromPoint(pt.x() - MARGIN, pt.y() - MARGIN);
    if (!m_buffer.SelectDiff(diffIndex))
        return;
    refreshImages();
    emit diffSelectionChanged();
}

void ImgMergeForm::onSceneMouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    updateStatusBar();
}

void ImgMergeForm::onSceneWheelEvent(QGraphicsSceneWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier)
    {
        setZoom(getZoom() * (event->delta() > 0 ? 1.25 : 0.8));
        event->accept();
    }
    else if (event->modifiers() & Qt::ShiftModifier)
    {
        QWheelEvent newEvent(event->pos().toPoint(), event->delta(), event->buttons(), 0, Qt::Horizontal);
        m_view[0]->horizontalScrollBar()->event(&newEvent);
        event->accept();
    }
}

void ImgMergeForm::on_spinBoxLeft_valueChanged(int arg1)
{
    if (arg1 - 1 != m_buffer.GetCurrentPage(0))
        setCurrentPage(0, arg1 - 1);
}

void ImgMergeForm::on_spinBoxRight_valueChanged(int arg1)
{
    if (arg1 - 1 != m_buffer.GetCurrentPage(1))
        setCurrentPage(1, arg1 - 1);
}

void ImgMergeForm::on_horizontalSliderZoom_valueChanged(int value)
{
    if (getZoom() != 1.0 + value * 0.125)
        setZoom(1.0 + value * 0.125);
}

void ImgMergeForm::on_comboBoxOverlayMode_currentIndexChanged(int index)
{
    if (getOverlayMode() != static_cast<CImgDiffBuffer::OVERLAY_MODE>(index))
        setOverlayMode(static_cast<CImgDiffBuffer::OVERLAY_MODE>(index));
}

void ImgMergeForm::on_checkBoxShowDifferences_toggled(bool checked)
{
    if (getShowDifferences() != checked)
        setShowDifferences(checked);
}

void ImgMergeForm::on_horizontalSliderDiffBlockSize_valueChanged(int value)
{
    if (getDiffBlockSize() != value)
        setDiffBlockSize(value);
}

void ImgMergeForm::on_horizontalSliderDiffColorAlpha_valueChanged(int value)
{
    if (getDiffColorAlpha() != value / 100.0)
       setDiffColorAlpha(value / 100.0);
}

void ImgMergeForm::on_horizontalSliderThreshold_valueChanged(int value)
{
    if (getColorDistanceThreshold() != value)
        setColorDistanceThreshold(value);
}

void ImgMergeForm::on_horizontalSliderOverlayAlpha_valueChanged(int value)
{
    if (getOverlayAlpha() != value / 100.0)
        setOverlayAlpha(value / 100.0);
}

void ImgMergeForm::on_spinBoxPage_valueChanged(int arg1)
{
    if (getCurrentMaxPage() != arg1 - 1)
        setCurrentPageAll(arg1 - 1);
}

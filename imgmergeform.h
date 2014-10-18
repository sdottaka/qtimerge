#ifndef IMGMERGEFORM_H
#define IMGMERGEFORM_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsSceneWheelEvent>
#include <QStringList>
#include "imgmergebuffer.hpp"

namespace Ui {
class ImgMergeForm;
}

class MyGraphicsScene : public QGraphicsScene
{
    Q_OBJECT

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
    {
        QGraphicsScene::mousePressEvent(mouseEvent);
        emit mousePress(mouseEvent);
    }

    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent)
    {
        QGraphicsScene::mouseDoubleClickEvent(mouseEvent);
        emit mouseDoubleClick(mouseEvent);
    }

    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
    {
        QGraphicsScene::mouseMoveEvent(mouseEvent);
        emit mouseMove(mouseEvent);
    }

    virtual void wheelEvent(QGraphicsSceneWheelEvent * wheelEvent)
    {
        QGraphicsScene::wheelEvent(wheelEvent);
        emit wheel(wheelEvent);
    }

signals:
    void mousePress(QGraphicsSceneMouseEvent *event);
    void mouseDoubleClick(QGraphicsSceneMouseEvent *event);
    void mouseMove(QGraphicsSceneMouseEvent *event);
    void wheel(QGraphicsSceneWheelEvent *event);
};

class ImgMergeForm : public QWidget
{
    Q_OBJECT

public:
    explicit ImgMergeForm(QWidget *parent = 0);
    ~ImgMergeForm();
    bool loadImages(const QStringList& paths);
    bool saveImages();
    bool modified() const;
    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();
    int getDiffCount() const;
    int getCurrentDiffIndex() const;
    void nextDiff();
    void prevDiff();
    void firstDiff();
    void lastDiff();
    void copyToLeft();
    void copyToRight();
    void copyAllToLeft();
    void copyAllToRight();
    bool getShowDifferences() const;
    void setShowDifferences(bool visible);
    int  getDiffBlockSize() const;
    void setDiffBlockSize(int blockSize);
    double getDiffColorAlpha() const;
    void setDiffColorAlpha(double alpha);
    double getColorDistanceThreshold() const;
    void setColorDistanceThreshold(double threshold);
    int  getActivePane() const;
    void setActivePane(int pane);
    int  getCurrentMaxPage() const;
    void setCurrentPageAll(int page);
    int  getCurrentPage(int pane) const;
    void setCurrentPage(int pane, int page);
    void setZoom(double factor);
    double getZoom() const;
    QColor getBackgroundColor() const;
    void setBackgroundColor(QColor color);
    bool getUseBackgroundColor() const;
    void setUseBackgroundColor(bool useBackgroundColor);
    CImgDiffBuffer::OVERLAY_MODE getOverlayMode() const;
    void setOverlayMode(CImgDiffBuffer::OVERLAY_MODE mode);
    double getOverlayAlpha() const;
    void setOverlayAlpha(double alpha);

private slots:
    void onSceneMousePressEvent(QGraphicsSceneMouseEvent *event);
    void onSceneMouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
    void onSceneMouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void onSceneWheelEvent(QGraphicsSceneWheelEvent *event);
    void on_spinBoxLeft_valueChanged(int arg1);
    void on_spinBoxRight_valueChanged(int arg1);
    void on_horizontalSliderZoom_valueChanged(int value);
    void on_comboBoxOverlayMode_currentIndexChanged(int index);
    void on_checkBoxShowDifferences_toggled(bool checked);
    void on_horizontalSliderDiffBlockSize_valueChanged(int value);
    void on_horizontalSliderDiffColorAlpha_valueChanged(int value);
    void on_horizontalSliderThreshold_valueChanged(int value);
    void on_horizontalSliderOverlayAlpha_valueChanged(int value);
    void on_spinBoxPage_valueChanged(int arg1);

signals:
    void diffSelectionChanged();
    void imageAppearanceChanged();
    void contentsChanged();

private:
    void updateTitle();
    void updateDock();
    void updateStatusBar();
    void scaleImages(double factor);
    void refreshImages();
    void scrollToDiff(int diffIndex);

    Ui::ImgMergeForm *ui;
    QImage m_image[3];
    MyGraphicsScene m_scene[3];
    QGraphicsView *m_view[3];
    double m_scaleFactor;
    CImgMergeBuffer m_buffer;
    bool m_useBackgroundColor;
    QColor m_backgroundColor;
};

#endif // IMGMERGEFORM_H

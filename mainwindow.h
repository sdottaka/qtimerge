#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMdiSubWindow>
#include "imgdiffbuffer.hpp"

class ImgMergeForm;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    bool openImages(const QStringList& paths);

private slots:
    void onImgMergeFormDiffSelectionChanged();
    void onImgMergeFormImageAppearanceChanged();
    void onImgMergeFormContentsChanged();

    void on_mdiArea_subWindowActivated(QMdiSubWindow *arg1);
    void on_actionFileOpen_triggered();
    void on_actionFileSave_triggered();
    void on_actionFileClose_triggered();
    void on_actionEditUndo_triggered();
    void on_actionEditRedo_triggered();
    void on_actionViewZoomIn_triggered();
    void on_actionViewZoomOut_triggered();
    void on_actionMergeNextDifference_triggered();
    void on_actionMergePreviousDifferece_triggered();
    void on_actionMergeFirstDifference_triggered();
    void on_actionMergeLastDifference_triggered();
    void on_actionMergeCopyToRight_triggered();
    void on_actionMergeCopyToLeft_triggered();
    void on_actionMergeCopyAllToRight_triggered();
    void on_actionMergeCopyAllToLeft_triggered();
    void on_actionImageViewDifferences_triggered();
    void on_actionImagePreviousPage_triggered();
    void on_actionImageNextPage_triggered();
    void on_actionImageActivePanePreviousPage_triggered();
    void on_actionImageActivePaneNextPage_triggered();
    void on_actionImageOverlayNone_triggered();
    void on_actionImageOverlayXOR_triggered();
    void on_actionImageOverlayAlphaBlend_triggered();
    void on_actionImageThreshold0_triggered();
    void on_actionImageThreshold1_triggered();
    void on_actionImageThreshold2_triggered();
    void on_actionImageThreshold4_triggered();
    void on_actionImageThreshold8_triggered();
    void on_actionImageThreshold16_triggered();
    void on_actionImageThreshold32_triggered();
    void on_actionImageDiffBlockSize1_triggered();
    void on_actionImageDiffBlockSize2_triggered();
    void on_actionImageDiffBlockSize4_triggered();
    void on_actionImageDiffBlockSize8_triggered();
    void on_actionImageDiffBlockSize16_triggered();
    void on_actionImageDiffBlockSize32_triggered();
    void on_actionImageZoom25_triggered();
    void on_actionImageZoom50_triggered();
    void on_actionImageZoom100_triggered();
    void on_actionImageZoom200_triggered();
    void on_actionImageZoom400_triggered();
    void on_actionImageZoom800_triggered();
    void on_actionImageSetBackgroundColor_triggered();
    void on_actionHelpAbout_triggered();
    void on_actionHelpAboutQt_triggered();

private:
    void on_actionImageOverlay_triggered(CImgDiffBuffer::OVERLAY_MODE mode);
    void on_actionImageThreshold_triggered(int threshold);
    void on_actionImageDiffBlockSize_triggered(int blockSize);
    void on_actionImageZoom_triggered(double zoom);

    void loadSettings();
    void saveSettings() const;
    ImgMergeForm *activeMdiChild();
    void updateAllActionStates();
    void updateFileActionStates();
    void updateEditActionStates();
    void updateViewActionStates();
    void updateMergeActionStates();
    void updateImageActionStates();
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H

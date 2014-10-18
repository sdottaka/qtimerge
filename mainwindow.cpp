#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "imgmergeform.h"
#include "opendialog.h"
#include <QDialog>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QSettings>
#include <QLabel>
#include <QColorDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    loadSettings();

    updateAllActionStates();
}

MainWindow::~MainWindow()
{
    saveSettings();

    delete ui;
}

void MainWindow::loadSettings()
{
    QSettings settings;
    settings.beginGroup("mainwindow");
    if (settings.value("maximized", false).toBool())
        showMaximized();
    else
    {
        resize(settings.value("size", size()).toSize());
        move(settings.value("pos", pos()).toPoint());
    }
    settings.endGroup();
}

void MainWindow::saveSettings() const
{
    QSettings settings;
    settings.beginGroup("mainwindow");
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    settings.setValue("maximized", isMaximized());
    settings.endGroup();
}

bool MainWindow::openImages(const QStringList &paths)
{
    ImgMergeForm *imgMergeForm = new ImgMergeForm();
    if (!imgMergeForm->loadImages(paths))
    {
        delete imgMergeForm;
        return false;
    }

    connect(imgMergeForm, SIGNAL(diffSelectionChanged()), this, SLOT(onImgMergeFormDiffSelectionChanged()));
    connect(imgMergeForm, SIGNAL(imageAppearanceChanged()), this, SLOT(onImgMergeFormImageAppearanceChanged()));
    connect(imgMergeForm, SIGNAL(contentsChanged()), this, SLOT(onImgMergeFormContentsChanged()));

    QMdiSubWindow *subWindow1 = ui->mdiArea->addSubWindow(imgMergeForm);
    subWindow1->setAttribute(Qt::WA_DeleteOnClose);
    subWindow1->showMaximized();
    ui->mdiArea->activateWindow();
    ui->mdiArea->setActiveSubWindow(subWindow1);
    updateAllActionStates();

    imgMergeForm->firstDiff();

    return true;
}

ImgMergeForm *MainWindow::activeMdiChild()
{
    if (QMdiSubWindow *activeSubWindow = ui->mdiArea->activeSubWindow())
        return qobject_cast<ImgMergeForm *>(activeSubWindow->widget());
    return 0;
}

void MainWindow::updateAllActionStates()
{
    updateFileActionStates();
    updateEditActionStates();
    updateViewActionStates();
    updateMergeActionStates();
    updateImageActionStates();
}

void MainWindow::updateFileActionStates()
{
    ImgMergeForm *frm = activeMdiChild();
    ui->actionFileSave->setEnabled(frm && frm->modified());
}

void MainWindow::updateEditActionStates()
{
    ImgMergeForm *frm = activeMdiChild();
    ui->actionEditUndo->setEnabled(frm && frm->canUndo());
    ui->actionEditRedo->setEnabled(frm && frm->canRedo());
}

void MainWindow::updateViewActionStates()
{
    ImgMergeForm *frm = activeMdiChild();
    ui->actionViewZoomIn->setEnabled(frm && frm->getZoom() < 20.0);
    ui->actionViewZoomOut->setEnabled(frm && frm->getZoom() > 0.333);
}

void MainWindow::updateMergeActionStates()
{
    ImgMergeForm *frm = activeMdiChild();
    ui->actionMergeNextDifference->setEnabled(frm && frm->getDiffCount() > 0 && frm->getDiffCount() > 0 && frm->getCurrentDiffIndex() < frm->getDiffCount() - 1);
    ui->actionMergePreviousDifferece->setEnabled(frm && frm->getDiffCount() > 0 && frm->getCurrentDiffIndex() > 0);
    ui->actionMergeFirstDifference->setEnabled(frm && frm->getDiffCount() > 0 && frm->getCurrentDiffIndex() != 0);
    ui->actionMergeLastDifference->setEnabled(frm && frm->getDiffCount() > 0 && frm->getCurrentDiffIndex() != frm->getDiffCount() - 1);
    ui->actionMergeCopyToLeft->setEnabled(frm && frm->getCurrentDiffIndex() != -1);
    ui->actionMergeCopyToRight->setEnabled(frm && frm->getCurrentDiffIndex() != -1);
    ui->actionMergeCopyAllToLeft->setEnabled(frm && frm->getDiffCount() > 0);
    ui->actionMergeCopyAllToRight->setEnabled(frm && frm->getDiffCount() > 0);
}

void MainWindow::updateImageActionStates()
{
    ImgMergeForm *frm = activeMdiChild();
    ui->actionImageViewDifferences->setEnabled(frm ? true : false);
    ui->actionImageViewDifferences->setChecked(frm && frm->getShowDifferences());
    QAction *actions[] = {ui->actionImageDiffBlockSize1, ui->actionImageDiffBlockSize2, ui->actionImageDiffBlockSize4, ui->actionImageDiffBlockSize8, ui->actionImageDiffBlockSize16, ui->actionImageDiffBlockSize32};
    for (int i = 0; i < sizeof(actions)/sizeof(actions[0]); ++i)
    {
        actions[i]->setEnabled(frm ? true : false);
        actions[i]->setChecked(frm && frm->getDiffBlockSize() == (1 << i));
    }
    QAction *actions2[] = {ui->actionImageThreshold0, ui->actionImageThreshold1, ui->actionImageThreshold2, ui->actionImageThreshold4, ui->actionImageThreshold8, ui->actionImageThreshold16, ui->actionImageThreshold32};
    for (int i = 0; i < sizeof(actions2)/sizeof(actions2[0]); ++i)
    {
        int threshold = (i == 0) ? 0 : (1 << (i - 1));
        actions2[i]->setEnabled(frm ? true : false);
        actions2[i]->setChecked(frm && frm->getColorDistanceThreshold() == threshold);
    }
    QAction *actions3[] = {ui->actionImageOverlayNone, ui->actionImageOverlayXOR, ui->actionImageOverlayAlphaBlend};
    for (int i = 0; i < sizeof(actions3)/sizeof(actions3[0]); ++i)
    {
        actions3[i]->setEnabled(frm ? true : false);
        actions3[i]->setChecked(frm && static_cast<int>(frm->getOverlayMode()) == i);
    }
    QAction *actions4[] = {ui->actionImageZoom25, ui->actionImageZoom50, ui->actionImageZoom100, ui->actionImageZoom200, ui->actionImageZoom400, ui->actionImageZoom800};
    for (int i = 0; i < sizeof(actions4)/sizeof(actions4[0]); ++i)
    {
        actions4[i]->setEnabled(frm ? true : false);
        actions4[i]->setChecked(frm && static_cast<int>(frm->getZoom() * 100) == (25 * (1 << i)));
    }
    ui->actionImageSetBackgroundColor->setEnabled(frm);
    ui->actionImageSetBackgroundColor->setChecked(frm && frm->getUseBackgroundColor());
}

void MainWindow::on_mdiArea_subWindowActivated(QMdiSubWindow *arg1)
{
    Q_UNUSED(arg1);
    updateAllActionStates();
}

void MainWindow::onImgMergeFormDiffSelectionChanged()
{
    updateMergeActionStates();
}

void MainWindow::onImgMergeFormImageAppearanceChanged()
{
    updateImageActionStates();
}

void MainWindow::onImgMergeFormContentsChanged()
{
    updateFileActionStates();
    updateEditActionStates();
}

void MainWindow::on_actionFileOpen_triggered()
{
    OpenDialog dialog;
    for (;;)
    {
        if (dialog.exec() != QDialog::Accepted)
            return;

        if (openImages(dialog.getPaths()))
            break;
    }
}

void MainWindow::on_actionFileSave_triggered()
{
    if (activeMdiChild())
        activeMdiChild()->saveImages();
}

void MainWindow::on_actionFileClose_triggered()
{
    if (activeMdiChild())
        activeMdiChild()->parentWidget()->close();
    updateAllActionStates();
}

void MainWindow::on_actionEditUndo_triggered()
{
    if (activeMdiChild())
        activeMdiChild()->undo();
}

void MainWindow::on_actionEditRedo_triggered()
{
    if (activeMdiChild())
        activeMdiChild()->redo();
}

void MainWindow::on_actionViewZoomIn_triggered()
{
    if (activeMdiChild())
        activeMdiChild()->setZoom(activeMdiChild()->getZoom() * 1.25);
}

void MainWindow::on_actionViewZoomOut_triggered()
{
    if (activeMdiChild())
        activeMdiChild()->setZoom(activeMdiChild()->getZoom() * 0.8);
}

void MainWindow::on_actionMergeNextDifference_triggered()
{
    if (activeMdiChild())
        activeMdiChild()->nextDiff();
}

void MainWindow::on_actionMergePreviousDifferece_triggered()
{
    if (activeMdiChild())
        activeMdiChild()->prevDiff();
}

void MainWindow::on_actionMergeFirstDifference_triggered()
{
    if (activeMdiChild())
        activeMdiChild()->firstDiff();
}

void MainWindow::on_actionMergeLastDifference_triggered()
{
    if (activeMdiChild())
        activeMdiChild()->lastDiff();
}

void MainWindow::on_actionMergeCopyToLeft_triggered()
{
    if (activeMdiChild())
        activeMdiChild()->copyToLeft();
}

void MainWindow::on_actionMergeCopyToRight_triggered()
{
    if (activeMdiChild())
        activeMdiChild()->copyToRight();
}

void MainWindow::on_actionMergeCopyAllToRight_triggered()
{
    if (activeMdiChild())
        activeMdiChild()->copyAllToRight();
}

void MainWindow::on_actionMergeCopyAllToLeft_triggered()
{
    if (activeMdiChild())
        activeMdiChild()->copyAllToLeft();
}

void MainWindow::on_actionImageViewDifferences_triggered()
{
    if (activeMdiChild())
        activeMdiChild()->setShowDifferences(!activeMdiChild()->getShowDifferences());
}

void MainWindow::on_actionImagePreviousPage_triggered()
{
    if (activeMdiChild())
        activeMdiChild()->setCurrentPageAll(activeMdiChild()->getCurrentMaxPage() - 1);
}

void MainWindow::on_actionImageNextPage_triggered()
{
    if (activeMdiChild())
        activeMdiChild()->setCurrentPageAll(activeMdiChild()->getCurrentMaxPage() + 1);
}

void MainWindow::on_actionImageActivePanePreviousPage_triggered()
{
    if (activeMdiChild())
    {
        int pane = activeMdiChild()->getActivePane();
        activeMdiChild()->setCurrentPage(pane, activeMdiChild()->getCurrentPage(pane) - 1);
    }
}

void MainWindow::on_actionImageActivePaneNextPage_triggered()
{
    if (activeMdiChild())
    {
        int pane = activeMdiChild()->getActivePane();
        activeMdiChild()->setCurrentPage(pane, activeMdiChild()->getCurrentPage(pane) + 1);
    }
}

void MainWindow::on_actionImageOverlay_triggered(CImgDiffBuffer::OVERLAY_MODE mode)
{
    if (activeMdiChild())
        activeMdiChild()->setOverlayMode(mode);
}

void MainWindow::on_actionImageOverlayNone_triggered() { on_actionImageOverlay_triggered(CImgDiffBuffer::OVERLAY_NONE); }
void MainWindow::on_actionImageOverlayXOR_triggered() { on_actionImageOverlay_triggered(CImgDiffBuffer::OVERLAY_XOR); }
void MainWindow::on_actionImageOverlayAlphaBlend_triggered() { on_actionImageOverlay_triggered(CImgDiffBuffer::OVERLAY_ALPHABLEND); }

void MainWindow::on_actionImageThreshold_triggered(int threshold)
{
    if (activeMdiChild())
        activeMdiChild()->setColorDistanceThreshold(threshold);
}

void MainWindow::on_actionImageThreshold0_triggered() { on_actionImageThreshold_triggered(0); }
void MainWindow::on_actionImageThreshold1_triggered() { on_actionImageThreshold_triggered(1); }
void MainWindow::on_actionImageThreshold2_triggered() { on_actionImageThreshold_triggered(2); }
void MainWindow::on_actionImageThreshold4_triggered() { on_actionImageThreshold_triggered(4); }
void MainWindow::on_actionImageThreshold8_triggered() { on_actionImageThreshold_triggered(8); }
void MainWindow::on_actionImageThreshold16_triggered() { on_actionImageThreshold_triggered(16); }
void MainWindow::on_actionImageThreshold32_triggered() { on_actionImageThreshold_triggered(32); }

void MainWindow::on_actionImageDiffBlockSize_triggered(int blockSize)
{
    if (activeMdiChild())
        activeMdiChild()->setDiffBlockSize(blockSize);
}

void MainWindow::on_actionImageDiffBlockSize1_triggered() { on_actionImageDiffBlockSize_triggered(1); }
void MainWindow::on_actionImageDiffBlockSize2_triggered() { on_actionImageDiffBlockSize_triggered(2); }
void MainWindow::on_actionImageDiffBlockSize4_triggered() { on_actionImageDiffBlockSize_triggered(4); }
void MainWindow::on_actionImageDiffBlockSize8_triggered() { on_actionImageDiffBlockSize_triggered(8); }
void MainWindow::on_actionImageDiffBlockSize16_triggered() { on_actionImageDiffBlockSize_triggered(16); }
void MainWindow::on_actionImageDiffBlockSize32_triggered() { on_actionImageDiffBlockSize_triggered(32); }

void MainWindow::on_actionImageZoom_triggered(double zoom)
{
    if (activeMdiChild())
        activeMdiChild()->setZoom(zoom);
}

void MainWindow::on_actionImageZoom25_triggered() { on_actionImageZoom_triggered(0.25); }
void MainWindow::on_actionImageZoom50_triggered() { on_actionImageZoom_triggered(0.5); }
void MainWindow::on_actionImageZoom100_triggered() { on_actionImageZoom_triggered(1.0); }
void MainWindow::on_actionImageZoom200_triggered() { on_actionImageZoom_triggered(2.0); }
void MainWindow::on_actionImageZoom400_triggered() { on_actionImageZoom_triggered(4.0); }
void MainWindow::on_actionImageZoom800_triggered() { on_actionImageZoom_triggered(8.0); }

void MainWindow::on_actionImageSetBackgroundColor_triggered()
{
    if (activeMdiChild())
    {
        bool useBackgroundColor = !activeMdiChild()->getUseBackgroundColor();
        if (useBackgroundColor)
        {
            QColorDialog dialog;
            dialog.setCurrentColor(activeMdiChild()->getBackgroundColor());
            if (dialog.exec() == QDialog::Accepted)
            {
                activeMdiChild()->setBackgroundColor(dialog.currentColor());
                activeMdiChild()->setUseBackgroundColor(useBackgroundColor);
            }
        }
        else
        {
            activeMdiChild()->setUseBackgroundColor(useBackgroundColor);
        }
    }
}

void MainWindow::on_actionHelpAbout_triggered()
{
    QMessageBox::about(this, "About", "QtIMerge version x.x.x.x\n\nCopyright 2004 sdottaka@sourceforge.net, All rights reserved.");
}

void MainWindow::on_actionHelpAboutQt_triggered()
{
    QMessageBox::aboutQt(this);
}

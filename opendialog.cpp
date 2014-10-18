#include "opendialog.h"
#include "ui_opendialog.h"
#include <QComboBox>
#include <QFileDialog>
#include <QCompleter>
#include <QDirModel>
#include <QFile>
#include <QFileInfo>
#include <QSettings>

OpenDialog::OpenDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OpenDialog)
{
    ui->setupUi(this);

    QCompleter *completer = new QCompleter(this);
    completer->setModel(new QDirModel(completer));
    ui->comboBoxLeft->setCompleter(completer);
    completer = new QCompleter(this);
    completer->setModel(new QDirModel(completer));
    ui->comboBoxRight->setCompleter(completer);

    loadSettings();
}

OpenDialog::~OpenDialog()
{
    saveSettings();
    delete ui;
}

QStringList OpenDialog::getPaths() const
{
    QStringList ary;
    ary.append(ui->comboBoxLeft->currentText());
    ary.append(ui->comboBoxRight->currentText());
    return ary;
}

void OpenDialog::on_pushButtonLeft_clicked()
{
    openFileDialog(ui->comboBoxLeft);
}

void OpenDialog::on_pushButtonRight_clicked()
{
    openFileDialog(ui->comboBoxRight);
}

static void addRecentPath(QComboBox *widget)
{
    QString text = widget->currentText();
    int index = widget->findText(text);
    if (index >= 0)
        widget->removeItem(index);
    widget->insertItem(0, text);

    while (widget->count() >= 20)
        widget->removeItem(widget->count() - 1);

    widget->setEditText(text);
}

void OpenDialog::accept()
{
    if (QFile(ui->comboBoxLeft->currentText()).exists() &&
        QFile(ui->comboBoxRight->currentText()).exists())
    {
        addRecentPath(ui->comboBoxLeft);
        addRecentPath(ui->comboBoxRight);
        QDialog::accept();
    }
}

void OpenDialog::loadSettings()
{
    QSettings settings;
    settings.beginGroup("opendialog");
    resize(settings.value("size", size()).toSize());
    ui->comboBoxLeft->addItems(settings.value("left").toStringList());
    ui->comboBoxLeft->setCurrentIndex(0);
    ui->comboBoxRight->addItems(settings.value("right").toStringList());
    ui->comboBoxRight->setCurrentIndex(0);
    settings.endGroup();
}

void OpenDialog::saveSettings() const
{
    QStringList leftPaths, rightPaths;
    for (int i = 0; i < ui->comboBoxLeft->count(); ++i)
        leftPaths.append(ui->comboBoxLeft->itemText(i));
    for (int i = 0; i < ui->comboBoxRight->count(); ++i)
        rightPaths.append(ui->comboBoxRight->itemText(i));

    QSettings settings;
    settings.beginGroup("opendialog");
    settings.setValue("size", size());
    settings.setValue("left", leftPaths);
    settings.setValue("right", rightPaths);
    settings.endGroup();
}

void OpenDialog::openFileDialog(QComboBox *widget)
{
    QString dir = QFileInfo(widget->currentText()).absoluteDir().path();
    QFileDialog dialog;
    QString filename = dialog.getOpenFileName(this, tr("Open File"), dir);
    if (!filename.isNull())
        widget->setEditText(filename);
}

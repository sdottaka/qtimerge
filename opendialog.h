#ifndef OPENDIALOG_H
#define OPENDIALOG_H

#include <QDialog>
#include <QComboBox>

namespace Ui {
class OpenDialog;
}

class OpenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OpenDialog(QWidget *parent = 0);
    ~OpenDialog();
    QStringList getPaths() const;

private slots:
    void on_pushButtonLeft_clicked();
    void on_pushButtonRight_clicked();
    virtual void accept();

private:
    void loadSettings();
    void saveSettings() const;
    void openFileDialog(QComboBox *widget);
    Ui::OpenDialog *ui;
};

#endif // OPENDIALOG_H

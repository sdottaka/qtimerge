#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication::setApplicationName("qtimerge");
    QApplication::setOrganizationName("qtimerge");
    QApplication::setOrganizationDomain("qtimerge.net");
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    if (argc > 2)
    {
        QStringList paths;
        paths.append(argv[1]);
        paths.append(argv[2]);
        w.openImages(paths);
    }

    return a.exec();
}

#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setOrganizationName("Chizhong Jin");
    a.setApplicationName("csv-editor");

    MainWindow w;
    w.show();

    return a.exec();
}

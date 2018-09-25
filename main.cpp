#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setOrganizationName("Chizhong Jin");
    app.setApplicationName("csv-editor");

    QCommandLineParser parser;
    parser.addPositionalArgument("file", 
            QCoreApplication::translate("main", "File to open"));

    parser.process(app);

    MainWindow w;
    w.show();

    if (parser.positionalArguments().length() > 0) {
        w.openFile(parser.positionalArguments()[0]);
    }

    return app.exec();
}

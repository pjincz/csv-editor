#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    QString _getOpenFile();

public slots:
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionExit_triggered();

    void on_actionCopy_triggered();
    void on_actionPaste_triggered();
    void on_actionClear_triggered();
    void on_actionPasteAppend_triggered();
    void on_actionPastePrepend_triggered();

private:
    Ui::MainWindow *ui;

    QString m_filename;
};

#endif // MAINWINDOW_H

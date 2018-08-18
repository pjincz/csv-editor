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

public slots:
    void on_actionLoad_triggered();
    void on_actionSave_triggered();
    void on_actionExit_triggered();

private:
    Ui::MainWindow *ui;

    QString m_filename;
};

#endif // MAINWINDOW_H

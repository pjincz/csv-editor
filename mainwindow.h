#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class TableWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void openFile(QString fname);

protected:
    void closeEvent(QCloseEvent *event);

private:
    QString _getOpenFile();
    QString _guessCrlf(const QString & cont);
    void updateTitle();

public slots:
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionExit_triggered();

    void on_actionCopy_triggered();
    void on_actionCut_triggered();
    void on_actionPaste_triggered();
    void on_actionClear_triggered();

    void on_actionUndo_triggered();
    void on_actionRedo_triggered();

    void on_actionAbout_triggered();

    void onChanged();

private:
    Ui::MainWindow *ui;
    TableWidget * m_tw;

    QString m_filename;
    QString m_crlf = "\n";
    bool m_dirt;
};

#endif // MAINWINDOW_H

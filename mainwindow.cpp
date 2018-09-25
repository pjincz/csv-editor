#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "csv.h"
#include "tablewidget.h"
#include "dialogaddcolumn.h"

#include <QDebug>
#include <QMessageBox>
#include <QCloseEvent>
#include <QSettings>
#include <QFileDialog>
#include <QClipboard>
#include <QMimeData>
#include <QDesktopServices>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_dirt(false)
{
    ui->setupUi(this);
    m_tw = ui->centralWidget;

    connect(m_tw, SIGNAL(changed()), this, SLOT(onChanged()));
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_dirt) {
        int ir = QMessageBox::information(this, QString(), "Do you want to save the changes you made?",
                                 QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (ir == QMessageBox::Yes) {
            on_actionSave_triggered();
            event->accept();
        } else if (ir == QMessageBox::No) {
            event->accept();
        } else if (ir == QMessageBox::Cancel) {
            event->ignore();
        }
    }
}

QString MainWindow::_getOpenFile()
{
    QSettings s;
    QString lastOpenDir = s.value("lastOpenDir").toString();

    QString fname = QFileDialog::getOpenFileName(this, "Choose csv...", lastOpenDir, "CSV Files (*.csv);;All Files(*)");
    if (!fname.isEmpty()) {
        lastOpenDir = QFileInfo(fname).absolutePath();
        s.setValue("lastOpenDir", lastOpenDir);

        return fname;
    }

    return QString();
}

void MainWindow::openFile(QString fname)
{
    QString strCont;
    QFile file(fname);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Error", "Cannot open " + fname);
        return;
    }
    QTextStream stm(&file);
    stm.setCodec("UTF-8");
    strCont = stm.readAll();
    file.close();

    QList<QStringList> cont = CSV::parseFromString(strCont);

    m_tw->reset();
    m_dirt = false;

    QStringList header = cont[0];
    cont.pop_front();
    int columns = header.length();
    for (int i = 0; i < columns; ++i) {
        m_tw->addColumn(header[i]);
    }

    for (int i = 0; i < cont.length(); ++i) {
        QStringList line = cont[i];
        m_tw->addRow(line);
    }

    m_tw->resizeColumnsToContents();

    m_filename = fname;
    m_crlf = _guessCrlf(strCont);
    updateTitle();
}

void MainWindow::updateTitle()
{
    QString title = "CSV Editor - " + m_filename + (m_dirt ? " *" : "");
    setWindowTitle(title);
}

void MainWindow::on_actionOpen_triggered()
{
    QString fname = _getOpenFile();
    if (fname.isEmpty())
        return;

    openFile(fname);
}

void MainWindow::on_actionSave_triggered()
{
    QList<QStringList> data;

    QStringList header;
    for (int i = 0; i < m_tw->columnCount(); ++i) {
        header.append(m_tw->header(i));
    }
    data.append(header);

    for (int i = 0; i < m_tw->rowCount(); ++i) {
        QStringList row;
        for (int j = 0; j < m_tw->columnCount(); ++j) {
            row.append(m_tw->text(i, j));
        }
        data.append(row);
    }

    CSV::write(data, m_filename, "UTF-8", m_crlf);

    m_dirt = false;
    updateTitle();
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::on_actionCopy_triggered()
{
    TableWidgetTransaction ts(m_tw, "Copy");

    QClipboard * clip = QGuiApplication::clipboard();
    QMimeData * data = new QMimeData();

    TableWidgetSelection sel = m_tw->selection();
    data->setText(m_tw->text(sel.row, sel.col));

    QList<QStringList> csvData;
    for (int row = sel.top; row <= sel.bottom; ++row) {
        QStringList csvRow;
        for (int col = sel.left; col <= sel.right; ++col) {
            csvRow << m_tw->text(row, col);
        }
        csvData << csvRow;
    }
    QString csv = CSV::toString(csvData);
    data->setData("text/csv", csv.toUtf8());

    clip->setMimeData(data);
}

void MainWindow::on_actionCut_triggered()
{
    TableWidgetTransaction ts(m_tw, "Cut");

    on_actionCopy_triggered();
    on_actionClear_triggered();
}

void MainWindow::on_actionPaste_triggered()
{
    TableWidgetTransaction ts(m_tw, "Paste");

    QClipboard * clip = QGuiApplication::clipboard();
    const QMimeData * mime = clip->mimeData();

    QList<QStringList> grid;
    if (mime->hasFormat("text/csv")) {
        QString csv = QString::fromUtf8(mime->data("text/csv"));
        grid = CSV::parseFromString(csv);
    } else {
        QStringList sl;
        sl << mime->text();
        grid << sl;
    }

    TableWidgetSelection sel = m_tw->selection();
    for (int row = sel.top; row <= sel.bottom; ++row) {
        for (int col = sel.left; col <= sel.right; ++col) {
            QStringList x = grid[(row - sel.top) % grid.length()];
            QString text = x[(col - sel.left) % x.length()];
            m_tw->setText(row, col, text);
        }
    }
}

void MainWindow::on_actionClear_triggered()
{
    TableWidgetTransaction ts(m_tw, "Clear");

    TableWidgetSelection sel = m_tw->selection();
    for (int row = sel.top; row <= sel.bottom; ++row) {
        for (int col = sel.left; col <= sel.right; ++col) {
            m_tw->setText(row, col, "");
        }
    }
}


void MainWindow::on_actionUndo_triggered()
{
    m_tw->undo();
}

void MainWindow::on_actionRedo_triggered()
{
    m_tw->redo();
}

void MainWindow::on_actionAbout_triggered()
{
    QDesktopServices::openUrl(QUrl("https://github.com/jinchizhong/csv-editor"));
}

void MainWindow::onChanged()
{
    m_dirt = true;
    updateTitle();
}

QString MainWindow::_guessCrlf(const QString & cont)
{
    int crlf = 0, lf = 0, cr = 0;

    QRegularExpression re("(\\r\\n|\\n|\\r)");
    QRegularExpressionMatchIterator i = re.globalMatch(cont);

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString x = match.captured(1);
        if (x == "\r\n")
            crlf += 1;
        else if (x == "\n")
            lf += 1;
        else if (x == "\r")
            cr += 1;
    }

    if (lf >= crlf && lf >= cr)
        return "\n";
    if (crlf >= cr)
        return "\r\n";
    return "\r";
}

void MainWindow::on_actionAddColumn_triggered()
{
    DialogAddColumn dlg(this, m_tw);
    dlg.exec();
}

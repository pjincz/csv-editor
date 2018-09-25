#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QDebug"
#include "csv.h"
#include <QFileDialog>
#include <QSettings>
#include <QClipboard>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDesktopServices>

class Command {
public:
    virtual void redo(QTableWidget * tw) = 0;
    virtual void undo(QTableWidget * tw) = 0;
    virtual ~Command() {};
};

class SetDataCommand : public Command {
public:
    SetDataCommand(int i, int j, int role, QVariant oldData, QVariant newData)
        : m_i(i), m_j(j), m_role(role), m_oldData(oldData), m_newData(newData)
    {

    }

    ~SetDataCommand() {

    }

    void redo(QTableWidget *tw) {
        QTableWidgetItem* item = tw->item(m_i, m_j);
        item->QTableWidgetItem::setData(m_role, m_newData);
    }

    void undo(QTableWidget *tw) {
        QTableWidgetItem* item = tw->item(m_i, m_j);
        item->QTableWidgetItem::setData(m_role, m_oldData);
    }

private:
    int m_i;
    int m_j;
    int m_role;
    QVariant m_oldData;
    QVariant m_newData;
};

struct CommandGroup : public QList<QSharedPointer<Command>>
{
    QList<QTableWidgetSelectionRange> prevSelection;
    QList<QTableWidgetSelectionRange> postSelection;
};

class CommandCenter : public QObject {
    Q_OBJECT

public:
    CommandCenter(QObject * parent, QTableWidget * tw)
        : QObject(parent), m_tw(tw), m_curStatus(0), m_inTransaction(false)
    {
    }

    void begin() {
        while (m_history.length() > m_curStatus) {
            m_history.pop_back();
        }

        m_history.push_back(CommandGroup());
        m_history.last().prevSelection = m_tw->selectedRanges();

        m_inTransaction = true;
    }

    void addCommand(Command * cmd) {
        if (m_inTransaction) {
            m_history.last().append(QSharedPointer<Command>(cmd));
            cmd->redo(m_tw);
        } else {
            begin();
            addCommand(cmd);
            commit();
        }
    }

    void commit() {
        if (!m_inTransaction)
            return;

        if (m_history.last().length() > 0) {
            m_curStatus += 1;
            m_history.last().postSelection = m_tw->selectedRanges();
        }
        else
            m_history.pop_back();

        m_inTransaction = false;

        emit commited();
    }

    void undo() {
        if (m_curStatus == 0)
            return;

        CommandGroup & g = m_history[m_curStatus - 1];
        for (int i = g.length() - 1; i >= 0; --i) {
            g[i]->undo(m_tw);
        }

        m_tw->clearSelection();
        for (auto & s : g.prevSelection)
            m_tw->setRangeSelected(s, true);

        m_curStatus -= 1;

        emit undone();
    }

    void redo() {
        if (m_curStatus >= m_history.length())
            return;

        CommandGroup & g = m_history[m_curStatus];
        for (int i = 0; i < g.length(); ++i) {
            g[i]->redo(m_tw);
        }

        m_tw->clearSelection();
        for (auto & s : g.postSelection)
            m_tw->setRangeSelected(s, true);

        m_curStatus += 1;

        emit redone();
    }

    void clear() {
        m_history.clear();
        m_curStatus = 0;
        m_inTransaction = false;
    }

    int length() {
        return m_history.length();
    }

signals:
    void commited();
    void undone();
    void redone();

private:
    QTableWidget * m_tw;
    QList<CommandGroup> m_history;
    int m_curStatus;
    bool m_inTransaction;
};

class MyTableWidgetItem : public QTableWidgetItem {
public:
    MyTableWidgetItem(CommandCenter * cc, QString text)
        : QTableWidgetItem(text)
        , m_cc(cc)
    {
    }

    virtual void setData(int role, const QVariant &value) {
        Command * cmd = new SetDataCommand(this->row(), this->column(), role, this->data(role), value);
        m_cc->addCommand(cmd);
    }

private:
    CommandCenter * m_cc;
};

class TransactionLocker
{
public:
    TransactionLocker(CommandCenter * cc, QString)
        : m_cc(cc)
    {
        m_cc->begin();
    }
    ~TransactionLocker() {
        m_cc->commit();
    }

private:
    CommandCenter * m_cc;
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_dirt(false)
{
    ui->setupUi(this);
    m_cc = new CommandCenter(this, ui->tableWidget);

    connect(m_cc, SIGNAL(commited()), this, SLOT(onChanged()));
    connect(m_cc, SIGNAL(undone()), this, SLOT(onChanged()));
    connect(m_cc, SIGNAL(redone()), this, SLOT(onChanged()));
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
    QList<QStringList> cont = CSV::parseFromFile(fname, "UTF-8");

    ui->tableWidget->clear();
    m_cc->clear();
    m_dirt = false;

    QStringList header = cont[0];
    cont.pop_front();
    int columns = header.length();
    ui->tableWidget->setColumnCount(columns);
    for (int i = 0; i < header.length(); ++i) {
        ui->tableWidget->setHorizontalHeaderItem(i, new MyTableWidgetItem(m_cc, header[i]));
    }

    ui->tableWidget->setRowCount(cont.length());
    for (int i = 0; i < cont.length(); ++i) {
        QStringList line = cont[i];
        for (int j = 0; j < columns; ++j) {
            QString text = j >= line.length() ? "" : line[j];
            ui->tableWidget->setItem(i, j, new MyTableWidgetItem(m_cc, text));
        }
    }

    ui->tableWidget->resizeColumnsToContents();

    m_filename = fname;
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
    for (int i = 0; i < ui->tableWidget->columnCount(); ++i) {
        header.append(ui->tableWidget->horizontalHeaderItem(i)->text());
    }
    data.append(header);

    for (int i = 0; i < ui->tableWidget->rowCount(); ++i) {
        QStringList row;
        for (int j = 0; j < ui->tableWidget->columnCount(); ++j) {
            row.append(ui->tableWidget->item(i, j)->text());
        }
        data.append(row);
    }

    CSV::write(data, m_filename, "UTF-8");

    m_dirt = false;
    updateTitle();
}

void MainWindow::on_actionExit_triggered()
{
    qApp->exit();
}

void MainWindow::on_actionCopy_triggered()
{
    TransactionLocker ts(m_cc, "Copy");

    QTableWidget * tw = ui->tableWidget;

    QClipboard * clip = QGuiApplication::clipboard();
    clip->setText(tw->currentItem()->text());
}

void MainWindow::on_actionPaste_triggered()
{
    TransactionLocker ts(m_cc, "Paste");

    QTableWidget * tw = ui->tableWidget;

    QClipboard * clip = QGuiApplication::clipboard();
    QString text = clip->text();

    QList<QTableWidgetItem*> si = tw->selectedItems();
    for (QTableWidgetItem* item : si) {
        item->setText(text);
    }
}

void MainWindow::on_actionClear_triggered()
{
    TransactionLocker ts(m_cc, "Clear");

    QTableWidget * tw = ui->tableWidget;

    QList<QTableWidgetItem*> si = tw->selectedItems();
    for (QTableWidgetItem* item : si) {
        item->setText("");
    }
}

void MainWindow::on_actionPasteAppend_triggered()
{
    TransactionLocker ts(m_cc, "PasteAppend");

    QTableWidget * tw = ui->tableWidget;

    QClipboard * clip = QGuiApplication::clipboard();
    QString text = clip->text();

    QList<QTableWidgetItem*> si = tw->selectedItems();
    for (QTableWidgetItem* item : si) {
        item->setText(item->text() + text);
    }
}

void MainWindow::on_actionPastePrepend_triggered()
{
    TransactionLocker ts(m_cc, "PastePrepend");

    QTableWidget * tw = ui->tableWidget;

    QClipboard * clip = QGuiApplication::clipboard();
    QString text = clip->text();

    QList<QTableWidgetItem*> si = tw->selectedItems();
    for (QTableWidgetItem* item : si) {
        item->setText(text + item->text());
    }
}

void MainWindow::on_actionUndo_triggered()
{
    m_cc->undo();
}

void MainWindow::on_actionRedo_triggered()
{
    m_cc->redo();
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


#include "mainwindow.moc"

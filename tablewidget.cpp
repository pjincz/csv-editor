#include "tablewidget.h"
#include <QStack>

class CommandCenter;

class Command {
public:
    virtual void redo(QTableWidget * tw, CommandCenter * cc) = 0;
    virtual void undo(QTableWidget * tw, CommandCenter * cc) = 0;
    virtual ~Command() {}
};

class MyTableWidgetItem : public QTableWidgetItem {
public:
    MyTableWidgetItem(CommandCenter * cc, QString text);
    virtual void setData(int role, const QVariant &value);

private:
    CommandCenter * m_cc;
};


class SetDataCommand : public Command {
public:
    SetDataCommand(int i, int j, int role, QVariant oldData, QVariant newData)
        : m_i(i), m_j(j), m_role(role), m_oldData(oldData), m_newData(newData)
    {

    }

    ~SetDataCommand() {

    }

    void redo(QTableWidget *tw, CommandCenter *) {
        QTableWidgetItem* item = tw->item(m_i, m_j);
        item->QTableWidgetItem::setData(m_role, m_newData);
    }

    void undo(QTableWidget *tw, CommandCenter *) {
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

class AddColumnCommand : public Command {
public:
    AddColumnCommand(const QString &title)
        : m_title(title)
    {
    }

    ~AddColumnCommand() {
    }

    void redo(QTableWidget *tw, CommandCenter * cc) {
        int col = tw->columnCount();
        tw->setColumnCount(col + 1);
        tw->setHorizontalHeaderItem(col, new MyTableWidgetItem(cc, m_title));

        for (int i = 0; i < tw->rowCount(); ++i) {
            tw->setItem(i, col, new MyTableWidgetItem(cc, ""));
        }
    }

    void undo(QTableWidget *tw, CommandCenter *) {
        int col = tw->columnCount();
        tw->setColumnCount(col - 1);
    }
private:
    QString m_title;
};

struct CommandGroup : public QList<QSharedPointer<Command>>
{
    QString name;

    QList<QTableWidgetSelectionRange> prevSelection;
    QList<QTableWidgetSelectionRange> postSelection;
};

class CommandCenter : public QObject {
    Q_OBJECT

public:
    CommandCenter(QObject * parent, QTableWidget * tw)
        : QObject(parent), m_tw(tw), m_curStatus(0)
    {
    }

    void begin(const QString &name) {
        if (m_transStack.empty()) {
            while (m_history.length() > m_curStatus) {
                m_history.pop_back();
            }

            m_history.push_back(CommandGroup());
            m_history.last().name = name;
            m_history.last().prevSelection = m_tw->selectedRanges();
        }

        m_transStack.push(name);
    }

    void addCommand(Command * cmd) {
        if (!m_transStack.empty()) {
            m_history.last().append(QSharedPointer<Command>(cmd));
            cmd->redo(m_tw, this);
        } else {
            begin(QString());
            addCommand(cmd);
            commit();
        }
    }

    void commit() {
        if (m_transStack.empty())
            return;

        m_transStack.pop();

        if (m_transStack.empty()) {
            if (m_history.last().length() > 0) {
                m_curStatus += 1;
                m_history.last().postSelection = m_tw->selectedRanges();
            }
            else
                m_history.pop_back();

            emit commited();
        }
    }

    void undo() {
        if (m_curStatus == 0)
            return;

        CommandGroup & g = m_history[m_curStatus - 1];
        for (int i = g.length() - 1; i >= 0; --i) {
            g[i]->undo(m_tw, this);
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
            g[i]->redo(m_tw, this);
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
        m_transStack.clear();
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
    QStack<QString> m_transStack;
};



MyTableWidgetItem::MyTableWidgetItem(CommandCenter * cc, QString text)
    : QTableWidgetItem(text)
    , m_cc(cc)
{
}

void MyTableWidgetItem::setData(int role, const QVariant &value) {
    Command * cmd = new SetDataCommand(this->row(), this->column(), role, this->data(role), value);
    m_cc->addCommand(cmd);
}

////////////////////////////////////////////////////////////////////////////////
/// TableWidget

TableWidget::TableWidget(QWidget *parent)
    : QWidget(parent)
{
    m_tw = new QTableWidget(this);
    m_layout = new QBoxLayout(QBoxLayout::LeftToRight, this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->addWidget(m_tw);

    m_cc = new CommandCenter(this, m_tw);

    connect(m_cc, SIGNAL(commited()), this, SIGNAL(changed()));
    connect(m_cc, SIGNAL(undone()), this, SIGNAL(changed()));
    connect(m_cc, SIGNAL(redone()), this, SIGNAL(changed()));
}

void TableWidget::reset()
{
    m_tw->setRowCount(0);
    m_tw->setColumnCount(0);
    m_cc->clear();
}

int TableWidget::columnCount()
{
    return m_tw->columnCount();
}

int TableWidget::rowCount()
{
    return m_tw->rowCount();
}

QString TableWidget::text(int r, int c)
{
    return m_tw->item(r, c)->text();
}

void TableWidget::setText(int r, int c, const QString &text)
{
    m_tw->item(r, c)->setText(text);
}

QString TableWidget::header(int c)
{
    return m_tw->horizontalHeaderItem(c)->text();
}

int TableWidget::addColumn(QString title)
{
    m_cc->addCommand(new AddColumnCommand(title));
    return m_tw->columnCount() - 1;
}

void TableWidget::addRow(QStringList cont)
{
    // TODO undo support
    int row = m_tw->rowCount();
    m_tw->setRowCount(row + 1);

    for (int i = 0; i < m_tw->columnCount(); ++i) {
        QString text = i >= cont.length() ? "" : cont[i];
        m_tw->setItem(row, i, new MyTableWidgetItem(m_cc, text));
    }
}

TableWidgetSelection TableWidget::selection()
{
    TableWidgetSelection sel;
    sel.row = m_tw->currentItem()->row();
    sel.col = m_tw->currentItem()->column();

    QTableWidgetSelectionRange rg = m_tw->selectedRanges()[0];
    sel.left = rg.leftColumn();
    sel.top = rg.topRow();
    sel.right = rg.rightColumn();
    sel.bottom = rg.bottomRow();

    return sel;
}

void TableWidget::resizeColumnsToContents()
{
    m_tw->resizeColumnsToContents();
}

void TableWidget::resizeColumnToContents(int col)
{
    m_tw->resizeColumnToContents(col);
}

void TableWidget::undo()
{
    m_cc->undo();
}

void TableWidget::redo()
{
    m_cc->redo();
}

void TableWidget::_beginTransaction(const QString &name)
{
    m_cc->begin(name);
}

void TableWidget::_commitTransaction()
{
    m_cc->commit();
}


#include "tablewidget.moc"

#ifndef TABLEWIDGET_H
#define TABLEWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QBoxLayout>

class CommandCenter;
struct TableWidgetSelection;

class TableWidget : public QWidget
{
    friend class TableWidgetTransaction;

    Q_OBJECT
public:
    explicit TableWidget(QWidget *parent = nullptr);

    void reset();

    int columnCount();
    int rowCount();
    QString text(int r, int c);
    void setText(int r, int c, const QString &text);
    QString header(int c);

    void addColumn(QString title);
    void addRow(QStringList row);

    TableWidgetSelection selection();
    void resizeColumnsToContents();

    void undo();
    void redo();

private:
    void _beginTransaction(const QString &name);
    void _commitTransaction();

signals:
    void changed();

private:
    QBoxLayout * m_layout;
    QTableWidget * m_tw;
    CommandCenter * m_cc;
};


class TableWidgetTransaction
{
public:
    TableWidgetTransaction(TableWidget * tw, QString name)
        : m_tw(tw)
    {
        m_tw->_beginTransaction(name);
    }
    ~TableWidgetTransaction() {
        m_tw->_commitTransaction();
    }

private:
    TableWidget * m_tw;
};

struct TableWidgetSelection
{
    int row;
    int col;

    int top;
    int bottom;
    int left;
    int right;
};


#endif // TABLEWIDGET_H

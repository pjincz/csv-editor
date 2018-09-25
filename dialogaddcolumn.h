#ifndef DIALOGADDCOLUMN_H
#define DIALOGADDCOLUMN_H

#include <QDialog>
#include "tablewidget.h"

namespace Ui {
class DialogAddColumn;
}

class DialogAddColumn : public QDialog
{
    Q_OBJECT

public:
    explicit DialogAddColumn(QWidget *parent, TableWidget * tw);
    ~DialogAddColumn();

    void accept();

private slots:
    void on_inputInit_currentIndexChanged(int index);

private:
    Ui::DialogAddColumn *ui;
    TableWidget * m_tw;
};

#endif // DIALOGADDCOLUMN_H

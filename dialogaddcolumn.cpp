#include "dialogaddcolumn.h"
#include "ui_dialogaddcolumn.h"

enum {
    INIT_EMPTY = 0,
    INIT_DUPLICATE = 1,
};

DialogAddColumn::DialogAddColumn(QWidget *parent, TableWidget * tw) :
    QDialog(parent),
    ui(new Ui::DialogAddColumn),
    m_tw(tw)
{
    ui->setupUi(this);

    ui->labelFrom->setVisible(false);
    ui->inputFrom->setVisible(false);

    for (int i = 0; i < m_tw->columnCount(); ++i) {
        ui->inputFrom->addItem(m_tw->header(i));
    }
}

DialogAddColumn::~DialogAddColumn()
{
    delete ui;
}

void DialogAddColumn::accept()
{
    TableWidgetTransaction ts(m_tw, "Add Column");

    int col = m_tw->addColumn(ui->inputHeader->text());

    if (ui->inputInit->currentIndex() == INIT_DUPLICATE) {
        int src = ui->inputFrom->currentIndex();

        for (int i = 0; i < m_tw->rowCount(); ++i) {
            m_tw->setText(i, col, m_tw->text(i, src));
        }
    }

    m_tw->resizeColumnToContents(col);

    QDialog::accept();
}

void DialogAddColumn::on_inputInit_currentIndexChanged(int index)
{
    bool showFrom = index == INIT_DUPLICATE;
    ui->labelFrom->setVisible(showFrom);
    ui->inputFrom->setVisible(showFrom);
}

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QDebug"
#include "csv.h"
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
    QString fname = QFileDialog::getOpenFileName(this, "Choose csv...", QString(), "CSV Files (*.csv);;All Files(*)");
    m_filename = fname;

    QList<QStringList> cont = CSV::parseFromFile(fname, "UTF-8");

    ui->tableWidget->clear();

    QStringList header = cont[0];
    cont.pop_front();
    ui->tableWidget->setColumnCount(header.length());
    for (int i = 0; i < header.length(); ++i) {
        ui->tableWidget->setHorizontalHeaderItem(i, new QTableWidgetItem(header[i]));
    }

    ui->tableWidget->setRowCount(cont.length());
    for (int i = 0; i < cont.length(); ++i) {
        QStringList line = cont[i];
        for (int j = 0; j < line.length(); ++j) {
            ui->tableWidget->setItem(i, j, new QTableWidgetItem(line[j]));
        }
    }

    ui->tableWidget->resizeColumnsToContents();
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
}

void MainWindow::on_actionExit_triggered()
{
    qApp->exit();
}

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    s7.loadDBInfo2("db2.txt");
}

void MainWindow::on_pushButton_2_clicked()
{
    s7.setIp(tr("192.168.0.10"));
    s7.slot_connect();
}

void MainWindow::on_pushButton_3_clicked()
{
    s7.slot_disconnect();
}

void MainWindow::on_pushButton_4_clicked()
{
    s7.readCpuInfo();
}

void MainWindow::on_pushButton_5_clicked()
{
    //输出当前线程ID
    qDebug() << "thread is " << QThread::currentThreadId();
    s7.slot_startRead();
}

void MainWindow::on_pushButton_6_clicked()
{
    s7.slot_stopRead();
}

void MainWindow::on_pushButton_7_clicked()
{
}

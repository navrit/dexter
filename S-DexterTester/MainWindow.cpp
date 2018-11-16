#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->welcomeLabel->setText(_welcomeText);
    _connectGuiSignalsToSlots();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::_connectGuiSignalsToSlots()
{
    connect(ui->connectPushButton,SIGNAL(released()),this,SLOT(on_connect_pushButton_released()));
    connect(ui->disconnectPushButton,SIGNAL(released()),this,SLOT(on_disconnect_pushButton_released()));
    connect(ui->setThPushButton,SIGNAL(released()),this,SLOT(on_setTh_pushButton_released()));
    connect(ui->getThPushButton,SIGNAL(released()),this,SLOT(on_getTh_pushButton_released()));
}

void MainWindow::on_connect_pushButton_released()
{

}

void MainWindow::on_disconnect_pushButton_released()
{

}

void MainWindow::on_setTh_pushButton_released()
{

}

void MainWindow::on_getTh_pushButton_released()
{

}

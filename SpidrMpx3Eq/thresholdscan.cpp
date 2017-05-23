#include "thresholdscan.h"
#include "ui_thresholdscan.h"

thresholdScan::thresholdScan(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::thresholdScan)
{
    ui->setupUi(this);
}

thresholdScan::~thresholdScan()
{
    delete ui;
}

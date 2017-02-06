#include "imagecalculator.h"
#include "ui_imagecalculator.h"
#include "mpx3gui.h"

ImageCalculator::ImageCalculator(Mpx3GUI * mg, QWidget *parent) :
    _mpx3gui(mg),
    QDialog(parent),
    ui(new Ui::ImageCalculator)
{
    ui->setupUi(this);
}

ImageCalculator::~ImageCalculator()
{
    delete ui;
}

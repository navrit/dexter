#include "testpulseequalisation.h"
#include "ui_testpulseequalisation.h"

testPulseEqualisation::testPulseEqualisation(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::testPulseEqualisation)
{
    ui->setupUi(this);
}

testPulseEqualisation::~testPulseEqualisation()
{
    delete ui;
}

#include "imagecalculator.h"
#include "ui_imagecalculator.h"
#include "mpx3gui.h"

ImageCalculator::ImageCalculator(Mpx3GUI * mg, QWidget *parent) :
    _mpx3gui(mg),
    QDialog(parent),
    ui(new Ui::ImageCalculator)
{
    ui->setupUi(this);
    QStringList stringList;
    foreach (int item, _mpx3gui->getDataset()->getThresholds()){
        stringList << QString::number( item );
    }

    //! Initialise UI elements
    ui->comboBox_img1->addItems(stringList);
    ui->comboBox_img2->addItems(stringList);
    ui->comboBox_removeLayer->addItems(stringList);

    //! Connect comboBox remove signal to slot to remove that item
    //!  Apparently this happens automagically somewhere, somehow
    //connect(ui->comboBox_removeLayer, SIGNAL(activated(int)), this, SLOT(on_comboBox_removeLayer_currentIndexChanged(int)));
}

ImageCalculator::~ImageCalculator()
{
    delete ui;
}

void ImageCalculator::on_okCancelBox_accepted()
{
    QString imgOperator = ui->comboBox_operator->currentText();
    int index1 = ui->comboBox_img1->currentIndex();
    int index2 = ui->comboBox_img2->currentIndex();
    _mpx3gui->getDataset()->runImageCalculator(imgOperator, index1, index2);
    //_mpx3gui->visSetThreshold(4);
    //_mpx3gui->addLayer( _mpx3gui->getDataset()->getLayer(2));
    //_mpx3gui->getDataset()->addLayer( _mpx3gui->getDataset()->getLayer(0), _mpx3gui->getDataset()->getLayerCount()+1);

}

void ImageCalculator::on_okCancelBox_rejected()
{
    return;
}

void ImageCalculator::on_comboBox_removeLayer_currentIndexChanged(int index)
{
    qDebug() << "[INFO] ImageCalculator::on_comboBox_removeLayer_currentIndexChanged :" << index;
}

void ImageCalculator::on_pushButton_removeLayer_clicked()
{
    int index = ui->comboBox_removeLayer->currentIndex();
    qDebug() << "[INFO] ImageCalculator::on_pushButton_removeLayer_clicked() :" << "Removing index" << index;
}

void ImageCalculator::on_pushButton_calcAllEnergyBins_clicked()
{
    _mpx3gui->getDataset()->calcAllEnergyBins();
}

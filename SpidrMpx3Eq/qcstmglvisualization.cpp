#include "qcstmglvisualization.h"
#include "ui_qcstmglvisualization.h"
#include "mpx3equalization.h"
#include <stdio.h>
QCstmGLVisualization::QCstmGLVisualization(QWidget *parent) :  QWidget(parent),  ui(new Ui::QCstmGLVisualization)
{
  ui->setupUi(this);
}

QCstmGLVisualization::~QCstmGLVisualization()
{
  delete ui;
}

void QCstmGLVisualization::setGradient(int index){
  ui->gradientDisplay->setGradient(_mpx3gui->getGradient(index));
  ui->glPlot->setGradient(_mpx3gui->getGradient(index));
}

void QCstmGLVisualization::SetMpx3GUI(Mpx3GUI *p){
  _mpx3gui = p;
  setGradient(0);
  connect(_mpx3gui, SIGNAL(ConnectionStatusChanged(bool)), ui->startButton, SLOT(setEnabled(bool))); //enable the button on connection
  connect(ui->startButton, SIGNAL(clicked(bool)), _mpx3gui, SLOT(establish_connection()));
  connect(ui->summingCheckbox, SIGNAL(clicked(bool)), _mpx3gui, SLOT(set_summing(bool)));
  connect(_mpx3gui, SIGNAL(summing_set(bool)), ui->summingCheckbox, SLOT(setChecked(bool)));
  connect(ui->gradientSelector, SIGNAL(activated(int)), this, SLOT(setGradient(int)));
  connect(ui->generateDataButton, SIGNAL(clicked()), _mpx3gui, SLOT(generateFrame()));
  connect(_mpx3gui, SIGNAL(frame_added()), this, SLOT(on_frame_added()));
  connect(_mpx3gui, SIGNAL(frames_reload()),this, SLOT(on_frame_updated()));
  connect(_mpx3gui, SIGNAL(availible_gradients_changed(QStringList)), this, SLOT(on_availible_gradients_changed(QStringList)));
  connect(ui->histPlot, SIGNAL(rangeChanged(QCPRange)),ui->glPlot, SLOT(setRange(QCPRange)));
  connect(ui->histPlot, SIGNAL(rangeChanged(QCPRange)), ui->gradientDisplay, SLOT(set_range(QCPRange)));
  connect(ui->layerSpinner, SIGNAL(valueChanged(int)), ui->glPlot, SLOT(setActive(int)));
  connect(ui->layerSpinner, SIGNAL(valueChanged(int)), ui->histPlot, SLOT(setActive(int)));
  connect(ui->binWidthSpinner, SIGNAL(valueChanged(int)), ui->histPlot, SLOT(changeBinSize(int)));
  connect(ui->glPlot, SIGNAL(hovered_pixel_changed(QPoint)),this, SLOT(on_hover_changed(QPoint)));
  connect(ui->glPlot, SIGNAL(pixel_selected(QPoint,QPoint)), this, SLOT(on_pixel_selected(QPoint,QPoint)));
  connect(this, SIGNAL(change_hover_text(QString)), ui->mouseOverLabel, SLOT(setText(QString)));
}

void QCstmGLVisualization::on_availible_gradients_changed(QStringList gradients){
  ui->gradientSelector->clear();
  ui->gradientSelector->addItems(gradients);
}

void QCstmGLVisualization::on_frame_updated(){
  ui->glPlot->setData(_mpx3gui->getDataset()->getFrames());
  ui->histPlot->changeBinSize(ui->binWidthSpinner->value());
}

void QCstmGLVisualization::on_hover_changed(QPoint pixel){
  emit(change_hover_text(QString("%1 @ (%2, %3)").arg(_mpx3gui->getPixelAt(pixel.x(), pixel.y(),ui->layerSpinner->value())).arg(pixel.x()).arg(pixel.y())));
}

void QCstmGLVisualization::on_frame_added(){
  ui->histPlot->addHistogram(_mpx3gui->getHist(-1), ui->binWidthSpinner->value());
  ui->layerSpinner->setMaximum(_mpx3gui->getFrameCount()-1);
  ui->layerSpinner->setValue(_mpx3gui->getFrameCount()-1);
}

void QCstmGLVisualization::on_pixel_selected(QPoint pixel, QPoint position){
  QMenu contextMenu;
  QAction mask(QString("Mask pixel @ %1, %2").arg(pixel.x()).arg(pixel.y()), &contextMenu), unmask(QString("Unmask pixel @ %1, %2").arg(pixel.x()).arg(pixel.y()), &contextMenu);
  contextMenu.addAction(&mask);
  contextMenu.addAction(&unmask);
  QAction* selectedItem = contextMenu.exec(position);
  if(selectedItem == &mask)
      //_mpx3gui->getDataset()->addMask(pixel);
     _mpx3gui->getEqualization()->GetEqualizationResults()->maskPixel(pixel.y()*_mpx3gui->getX()+pixel.x());
  else if(selectedItem == &unmask)
      //_mpx3gui->getDataset()->removeMask(pixel);
      _mpx3gui->getEqualization()->GetEqualizationResults()->unmaskPixel(pixel.y()*_mpx3gui->getX()+pixel.x());
  //_mpx3gui->getDataset()->loadAdjustments();
  _mpx3gui->getEqualization()->SetAllAdjustmentBits( );//TODO: integrate into dataset
}

#include "qcstmglvisualization.h"
#include "ui_qcstmglvisualization.h"
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
  connect(ui->gradientSelector, SIGNAL(activated(int)), this, SLOT(setGradient(int)));
  connect(ui->generateDataButton, SIGNAL(clicked()), _mpx3gui, SLOT(generateFrame()));
  connect(_mpx3gui, SIGNAL(frame_added()), this, SLOT(on_frame_added()));
  connect(_mpx3gui, SIGNAL(frames_reload(QVector<int *>)),ui->glPlot, SLOT(setData(QVector<int*>)));
  connect(_mpx3gui, SIGNAL(availible_gradients_changed(QStringList)), this, SLOT(on_availible_gradients_changed(QStringList)));
  connect(ui->histPlot, SIGNAL(rangeChanged(QCPRange)),ui->glPlot, SLOT(setRange(QCPRange)));
  connect(ui->histPlot, SIGNAL(rangeChanged(QCPRange)), ui->gradientDisplay, SLOT(set_range(QCPRange)));
  connect(ui->layerSpinner, SIGNAL(valueChanged(int)), ui->glPlot, SLOT(setActive(int)));
  connect(ui->layerSpinner, SIGNAL(valueChanged(int)), ui->histPlot, SLOT(setActive(int)));
  connect(ui->binWidthSpinner, SIGNAL(valueChanged(int)), ui->histPlot, SLOT(changeBinSize(int)));
  connect(ui->glPlot, SIGNAL(hovered_pixel_changed(QPoint)),this, SLOT(on_hover_changed(QPoint)));
  connect(this, SIGNAL(change_hover_text(QString)), ui->mouseOverLabel, SLOT(setText(QString)));
}

void QCstmGLVisualization::on_availible_gradients_changed(QStringList gradients){
  ui->gradientSelector->clear();
  ui->gradientSelector->addItems(gradients);
}

void QCstmGLVisualization::on_hover_changed(QPoint pixel){
  emit(change_hover_text(QString("%1 @ (%2, %3)").arg(_mpx3gui->getPixelAt(pixel.x(), pixel.y(),ui->layerSpinner->value())).arg(pixel.x()).arg(pixel.y())));
}

void QCstmGLVisualization::on_frame_added(){
  printf("GLvis: frame added!\n");
  ui->histPlot->addHistogram(_mpx3gui->getHist(-1), ui->binWidthSpinner->value());
  ui->layerSpinner->setMaximum(_mpx3gui->getFrameCount()-1);
  ui->layerSpinner->setValue(_mpx3gui->getFrameCount()-1);
}

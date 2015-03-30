#include "qcstmglvisualization.h"
#include "ui_qcstmglvisualization.h"
#include <stdio.h>
QCstmGLVisualization::QCstmGLVisualization(QWidget *parent) :  QWidget(parent),  ui(new Ui::QCstmGLVisualization)
{
  ui->setupUi(this);
  ui->gradientDisplay->setGradient(ui->glPlot->getGradient());
}

QCstmGLVisualization::~QCstmGLVisualization()
{
  delete ui;
}

void QCstmGLVisualization::SetMpx3GUI(Mpx3GUI *p){
  _mpx3gui = p;
  connect(ui->generateDataButton, SIGNAL(clicked()), _mpx3gui, SLOT(generateFrame()));
  connect(_mpx3gui, SIGNAL(frame_added()), this, SLOT(on_frame_added()));
  connect(_mpx3gui, SIGNAL(frames_reload(QVector<int *>)),ui->glPlot, SLOT(setData(QVector<int*>)));
  connect(ui->histPlot, SIGNAL(rangeChanged(QCPRange)),ui->glPlot, SLOT(setRange(QCPRange)));
}

void QCstmGLVisualization::on_frame_added(){
  printf("GLvis: frame added!\n");
  ui->histPlot->addHistogram(_mpx3gui->getHist(-1), ui->binWidthSpinner->value());
  ui->layerSpinner->setMaximum(_mpx3gui->getFrameCount()-1);
  ui->layerSpinner->setValue(_mpx3gui->getFrameCount()-1);
}

#include "heatmapdisplay.h"
#include "ui_heatmapdisplay.h"

HeatmapDisplay::HeatmapDisplay(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::HeatmapDisplay)
{
  ui->setupUi(this);
  ui->WRuler->setMinimumSize(1,1);
  ui->NRuler->setMinimumSize(1,1);
  ui->gradient->setMinimumSize(1,1);
  setupSignalsAndSlots();
  //ui->heatmap->setSize(512, 512);
  ui->NRuler->setOrientation(QCstmRuler::orientationTop);
  ui->NRuler->setMargin(25);
}

HeatmapDisplay::~HeatmapDisplay()
{
  delete ui;
}

void HeatmapDisplay::setupSignalsAndSlots(){
  connect(ui->heatmap, SIGNAL(size_changed(QPoint)), ui->WRuler, SLOT(set_max(QPoint)));
  connect(ui->heatmap, SIGNAL(size_changed(QPoint)), ui->NRuler, SLOT(set_max(QPoint)));
  //connect(ui->heatmap, SIGNAL(offset_changed(QPointF)), ui->WRuler, SLOT(set_offset(QPointF)));
  //connect(ui->heatmap, SIGNAL(zoom_changed(float)), ui->WRuler, SLOT(set_zoom(float)));
  connect(ui->heatmap, SIGNAL(bounds_changed(QRectF)), ui->WRuler, SLOT(on_bounds_changed(QRectF)));
  connect(ui->heatmap, SIGNAL(bounds_changed(QRectF)), ui->NRuler, SLOT(on_bounds_changed(QRectF)));
  connect(ui->heatmap, SIGNAL(size_changed(QPoint)), ui->NRuler, SLOT(set_cutoff(QPoint)));
  connect(ui->heatmap, SIGNAL(size_changed(QPoint)), ui->WRuler, SLOT(set_cutoff(QPoint)));
}

void HeatmapDisplay::setGradient(Gradient *gradient){
  ui->gradient->setGradient(gradient);
  ui->heatmap->setGradient(gradient);
}

void HeatmapDisplay::setSize(QPoint size){
	setSize(size.x(), size.y());
}
void HeatmapDisplay::setSize(int x, int y){
  //ui->heatmap->setSize(x, y);
  ui->NRuler->set_cutoff(x,y);
  ui->WRuler->set_cutoff(x,y);
}

void HeatmapDisplay::set_range(QCPRange range){
  ui->heatmap->setRange(range);
  ui->gradient->set_range(range);
}

QCstmGLPlot* HeatmapDisplay::getPlot(){
  return ui->heatmap;
}

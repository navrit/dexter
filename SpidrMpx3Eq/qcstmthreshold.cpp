#include "qcstmthreshold.h"
#include "ui_qcstmthreshold.h"
#include  "qcstmdacs.h"

QCstmThreshold::QCstmThreshold(QWidget *parent) :  QWidget(parent),  ui(new Ui::QCstmThreshold)
{
  ui->setupUi(this);
  QList<int> defaultSizesMain; //The ratio of the splitters. Defaults to the golden ratio because "oh! fancy".
  defaultSizesMain.append(2971215);
  defaultSizesMain.append(1836312);
  for(int i = 0; i < ui->splitter->count();i++){
      QWidget *child = ui->splitter->widget(i);
      child->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      child->setMinimumSize(1,1);
    }
  ui->splitter->setSizes(defaultSizesMain);

  // Signals & Slots
  SetupSignalsAndSlots();

  // GUI defaults
  GUIDefaults();

  ui->framePlot->axisRect()->setupFullAxesBox(true);
  QCPColorScale *colorScale = new QCPColorScale(ui->framePlot);
  ui->framePlot->plotLayout()->addElement(0, 1, colorScale); // add it to the right of the main axis rect
  colorScale->setType(QCPAxis::atRight); // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
  for(int i = 0; i < MPX3RX_DAC_COUNT;i++){
      ui->scanTargetComboBox->addItem(MPX3RX_DAC_TABLE[i].name);
    }
}

QCstmThreshold::~QCstmThreshold()
{
  delete ui;
}

int QCstmThreshold::getActiveTargetCode(){
  return MPX3RX_DAC_TABLE[ui->scanTargetComboBox->currentIndex()-1].code;
}

void QCstmThreshold::StartCalibration() {

  SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
  SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();



}

void QCstmThreshold::SetupSignalsAndSlots() {

  std::cout << "[QCstmThreshold] Connecting signals and slots" << std::endl;
  connect( ui->thlCalibStart, SIGNAL(clicked()), this, SLOT( StartCalibration() ) );

}

void QCstmThreshold::GUIDefaults() {

  //ui->thlCalibStart->
}

void QCstmThreshold::addFrame(QPoint offset, int layer, int* data){
  while(layer >= ui->framePlot->plottableCount())
    ui->framePlot->addPlottable(new QCPColorMap(ui->framePlot->xAxis, ui->framePlot->yAxis));
  int nx = _mpx3gui->getDataset()->x(),  ny =_mpx3gui->getDataset()->y(); //TODO: grab from config.
  for(int i = 0; i < ny; i++)
    for(int j = 0; j < nx; j++)
      ((QCPColorMap*)ui->framePlot->plottable(layer))->data()->setCell(j+offset.x()*nx,ny-1-i+offset.y()*ny, data[i*nx+j]);
  ui->framePlot->rescaleAxes();
}


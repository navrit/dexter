#include "qcstmthreshold.h"
#include "ui_qcstmthreshold.h"

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

}

QCstmThreshold::~QCstmThreshold()
{
  delete ui;
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


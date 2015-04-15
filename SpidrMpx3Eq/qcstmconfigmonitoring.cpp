#include "qcstmconfigmonitoring.h"
#include "ui_qcstmconfigmonitoring.h"
#include "mpx3config.h"

QCstmConfigMonitoring::QCstmConfigMonitoring(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::QCstmConfigMonitoring)
{
  ui->setupUi(this);
}

QCstmConfigMonitoring::~QCstmConfigMonitoring()
{
  delete ui;
}

void QCstmConfigMonitoring::SetMpx3GUI(Mpx3GUI *p){
  _mpx3gui = p;
  Mpx3Config *config = _mpx3gui->getConfig();
  ui->ipLabel->setText(config->getIpAddress());



  connect(ui->ColourModeCheckBox, SIGNAL(clicked(bool)), config, SLOT(setColourMode(bool)));
  connect(config, SIGNAL(colourModeChanged(bool)), ui->ColourModeCheckBox, SLOT(setChecked(bool)));

  connect(ui->csmSpmSpinner, SIGNAL(valueChanged(int)), config, SLOT(setCsmSpm(int)));
  connect(config, SIGNAL(csmSpmChanged(int)), ui->csmSpmSpinner, SLOT(setValue(int)));

  connect(ui->decodeFramesCheckbox, SIGNAL(clicked(bool)), config, SLOT(setDecodeFrames(bool)));
  connect(config, SIGNAL(decodeFramesChanged(bool)), ui->decodeFramesCheckbox, SLOT(setChecked(bool)));

  connect(ui->gainModeSpinner, SIGNAL(valueChanged(int)), config, SLOT(setGainMode(int)));
  connect(config, SIGNAL(gainModeChanged(int)), ui->gainModeSpinner, SLOT(setValue(int)));

  connect(ui->maxPacketSizeSpinner, SIGNAL(valueChanged(int)), config, SLOT(setMaxPacketSize(int)));
  connect(config, SIGNAL(MaxPacketSizeChanged(int)), ui->maxPacketSizeSpinner, SLOT(setValue(int)));

  connect(ui->nTriggersSpinner, SIGNAL(valueChanged(int)), config, SLOT(setNTriggers(int)));
  connect(config, SIGNAL(nTriggersChanged(int)), ui->nTriggersSpinner, SLOT(setValue(int)));

  connect(ui->operationModeSpinner, SIGNAL(valueChanged(int)), config, SLOT(setOperationMode(int)));
  connect(config, SIGNAL(operationModeChanged(int)), ui->operationModeSpinner, SLOT(setValue(int)));

  connect(ui->pixelDepthSpinner, SIGNAL(valueChanged(int)), config, SLOT(setPixelDepth(int)));
  connect(config, SIGNAL(pixelDepthChanged(int)), ui->pixelDepthSpinner, SLOT(setValue(int)));

  connect(ui->triggerLengthSpinner, SIGNAL(valueChanged(int)), config, SLOT(setTriggerLength(int)));
  connect(config, SIGNAL(TriggerLengthChanged(int)), ui->triggerLengthSpinner, SLOT(setValue(int)));

  connect(ui->triggerModeSpinner, SIGNAL(valueChanged(int)), config, SLOT(setTriggerMode(int)));
  connect(config, SIGNAL(TriggerModeChanged(int)), ui->triggerModeSpinner, SLOT(setValue(int)));

  connect(config, SIGNAL(IpAdressChanged(QString)), ui->ipLabel, SLOT(setText(QString)));
}

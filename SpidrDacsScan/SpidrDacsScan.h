#include "ui_SpidrDacsScan.h"
#include <QDialog>

class SpidrController;
class QIntValidator;
class QCustomPlot;
class QCPPlotTitle;
class QCPGraph;

class SpidrDacsScan : public QDialog, Ui_SpidrDacsScanDialog
{
  Q_OBJECT

 public:
  SpidrDacsScan();
  ~SpidrDacsScan();

 public slots:
  void connectOrDisconnect();
  void changeDeviceIndex( int index );
  void changeDeviceType( int index );
  void startOrStopScan();
  void scan();

 private:
  void inError();

 private:
  SpidrController *_spidrController;
  int _deviceIndex;
  int _deviceType;

  QIntValidator   *_ipAddrValidator;
  QIntValidator   *_ipPortValidator;

  // Scan parameters
  bool _scanInProgress;
  int  _dacCount;
  int  _dacIndex;
  int  _dacCode;
  int  _dacMax;
  int  _dacVal;
  int  _dacStep;
  int  _samples;
  const struct dac_s *_dacTable;

  // ADC parameters
  double _adcFullScale;
  double _adcRange;

  // The plot
  QCustomPlot  *_plot;
  QCPPlotTitle *_title;

  // Currently active graph
  QCPGraph *_graph;
};

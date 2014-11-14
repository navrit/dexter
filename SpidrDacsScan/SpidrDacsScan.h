#include "ui_SpidrDacsScan.h"
#include <QDialog>

class SpidrController;
class QIntValidator;
class QCustomPlot;
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
  void startOrStopScan();
  void scan();

 private:
  void inError();

 private:
  SpidrController *_spidrController;
  int _deviceIndex;

  QIntValidator   *_ipAddrValidator;
  QIntValidator   *_ipPortValidator;

  // Scan parameters
  bool _scanInProgress;
  int  _dacIndex;
  int  _dacCode;
  int  _dacMax;
  int  _dacVal;
  int  _dacStep;
  int  _samples;

  // The plot
  QCustomPlot *_plot;

  // Currently active graph
  QCPGraph *_graph;
};

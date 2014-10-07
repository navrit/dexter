#include "ui_SpidrMon.h"
#include <QDialog>

class SpidrController;
class QIntValidator;

class SpidrMon : public QDialog, Ui_SpidrMonDialog
{
  Q_OBJECT

 public:
  SpidrMon();
  ~SpidrMon();

 private slots:
  void connectOrDisconnect();
  void updateLedOff();
  void doubleSpidrModeChanged();
  void myResize();

 private:
  SpidrController *_spidrController;

  QIntValidator   *_ipAddrValidator;
  QIntValidator   *_ipPortValidator;

  int              _timerId;
  int              _dacCode;
  bool             _doubleSpidr;

  int              _bgOut1, _bgOut2;
  bool             _dacOkay1, _dacOkay2;

  void timerEvent( QTimerEvent * );
  void initDataDisplay();
};

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

 private:
  SpidrController *_spidrController;

  QIntValidator   *_ipAddrValidator;
  QIntValidator   *_ipPortValidator;

  int              _timerId;

  bool             _skipVdd;

  void timerEvent( QTimerEvent * );
  void initDataDisplay();

  void closeEvent( QCloseEvent *event );
  void readAppSettings();
  void writeAppSettings();
};

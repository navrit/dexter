#include "ui_DummyGen.h"
#include <QDialog>

class SpidrController;
class QIntValidator;
class QTimerEvent;

class DummyGen : public QDialog, Ui_SpidrDummyGenDialog
{
  Q_OBJECT

 public:
  DummyGen();
  ~DummyGen();

 private slots:
  void connectOrDisconnect();
  void startOrStop();

 private:
  SpidrController *_spidrController;

  int              _regVal;

  QIntValidator   *_ipAddrValidator;
  QIntValidator   *_ipPortValidator;

  int              _timerId;

  void timerEvent( QTimerEvent * );
};

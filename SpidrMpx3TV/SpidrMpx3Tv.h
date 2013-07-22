#include "ui_spidrtv.h"
#include <QImage>
#include <QMainWindow>

class QTimerEvent;
class SpidrController;
class SpidrDaq;

class SpidrMpx3Tv : public QMainWindow, Ui_SpidrTv
{
  Q_OBJECT

 public:
  SpidrMpx3Tv();
  virtual ~SpidrMpx3Tv();

  void timerEvent( QTimerEvent * );

 private slots:
  void onOff();
  void changeCounterDepth();

 private:
  SpidrController *_controller;
  SpidrDaq        *_daq;
  QImage           _image;
  int              _counterDepth;

  void decodeAndDisplay();
};

#include "ui_spidrtpx3tv.h"
#include <QImage>
#include <QMainWindow>

class QTimerEvent;
class SpidrController;
class SpidrDaq;

class SpidrTpx3Tv : public QMainWindow, Ui_SpidrTpx3Tv
{
  Q_OBJECT

 public:
  SpidrTpx3Tv();
  virtual ~SpidrTpx3Tv();

  void timerEvent( QTimerEvent * );

 private slots:
  void onOff();
  void resetPixelCounters();

 private:
  SpidrController *_controller;
  SpidrDaq        *_daq;

  unsigned long    _pixelCounter[256][256];

  QImage           _image;

  void displayImage();
};

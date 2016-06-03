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
  void changeLutEnabled();
  void changeIntegrating();

 private:
  SpidrController *_controller;
  SpidrDaq        *_daq;
  bool             _isCompactSpidr;
  QImage           _image1, _image4;
  int              _deviceCount;
  int              _counterDepth;
  bool             _integrating;
  int             *_pixData; // To store integrated pixel data

  void decodeAndDisplay();
};

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
  void selectDeviceNr( int index );
  void resetPixelCounters();
  void changeTestMode( int state );
  void changeShutter( bool open );
  void setDacCoarse( int dac_val );
  void setDacFine( int dac_val );

 private:
  void displayError( const char *str );

 private:
  SpidrController *_controller;
  SpidrDaq        *_daq;

  int              _deviceNr;

  bool             _resetPixelConfig;

  unsigned long    _cntr;

  unsigned long    _pixelCounter[256][256];

  QImage           _image;

  void displayImage();
};

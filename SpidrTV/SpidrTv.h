#include "ui_spidrtv.h"
#include <QMainWindow>
#include "ReceiverThread.h"

class SpidrTv : public QMainWindow, Ui_SpidrTv
{
  Q_OBJECT

 public:
  SpidrTv();
  virtual ~SpidrTv();
  void timerEvent( QTimerEvent * );

 private slots:
  void onOff();
  void changeCounterDepth();
  void changeDeviceType();

 private:
  ReceiverThread *_recvr;
  QImage          _image;
  unsigned char   _rawFrame[MPX3_24BIT_RAW_SZ]; // Large enough for all modes
  int             _pixels[256*256];
  int             _counterDepth;
  int             _deviceType;

  // Look-up tables for Medipix3RX pixel data decoding
  int             _mpx3Rx6Bits[64];
  int             _mpx3Rx12Bits[4096];

  void decodeAndDisplay();

  void mpx3RawToPixel( unsigned char *raw_bytes,
		       int           *pixels,
		       int            counter_depth,
		       int            device_type );
};

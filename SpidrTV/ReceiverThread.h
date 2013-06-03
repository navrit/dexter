#include <QObject>
#include <QThread>
#include <QUdpSocket>
#include <QMutex>
#include <QWaitCondition>

#define MPX3_12BIT_RAW_SZ ((256 * 256 * 12) / 8)
#define MPX3_24BIT_RAW_SZ ((256 * 256 * 24) / 8)

class ReceiverThread :
    public QThread
{
  Q_OBJECT
 public:
  ReceiverThread( QString adapter,
		  unsigned short port,
		  int mpx3_counter_bits,
		  QObject * parent);
  virtual         ~ReceiverThread();

  void            stop();

  bool            copyFrame( unsigned char *data );

 protected:
  void            run();
#ifdef OLD_CODE
  void            run_alternative();
#endif // OLD_CODE

private:
  void            gotoNextFrameBuffer();

  bool            _stop;
  unsigned int    _frameSize;
  unsigned char  *_frameBuffer[2];
  bool            _frameFull[2];
  unsigned int    _frameIndex;
  unsigned char  *_currFrame;

  QString         _adapter;
  unsigned short  _port;

  unsigned int    _framesSkipped;
  unsigned int    _framesReceived;
  unsigned int    _packetsLost;
  unsigned int    _packetsReceived;
  unsigned int    _debugCounter;

  QString         _error;

  QMutex          _mutex;
  QWaitCondition  _condition;

 public:
  unsigned int    framesSkipped()    { return _framesSkipped; }
  unsigned int    framesReceived()   { return _framesReceived; }
  unsigned int    packetsLost()      { return _packetsLost; }
  unsigned int    packetsReceived()  { return _packetsReceived; }
  unsigned int    debugCounter()     { return _debugCounter; }

  bool            stopped()          { return _stop; }

  QString         error()            { return _error; }

 public slots:
   void waitForData();
};

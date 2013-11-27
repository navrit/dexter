#include <QUdpSocket>

#include "ReceiverThread.h"

// ----------------------------------------------------------------------------

ReceiverThread::ReceiverThread( int *ipaddr,
				int  port,
				QObject *parent )
  : QThread( parent ),
    _sock( 0 ),
    _port( port ),
    _suspend( false ),
    _suspended( false ),
    _stop( false ),
    _packetsReceived( 0 ),
    _packetsLost( 0 ),
    _bytesReceived( 0 ),
    _bytesLost( 0 ),
    _lastPacketSize( 0 ),
    _bufferWraps( 0 ),
    _bufferSize( RECV_BUF_SIZE ),
    _head( 0 ),
    _tail( 0 ),
    _headEnd( 0 ),
    _full( false ),
    _fullOccurred( false )
{
  _addr = (((ipaddr[3] & 0xFF) << 24) | ((ipaddr[2] & 0xFF) << 16) |
	   ((ipaddr[1] & 0xFF) << 8) | ((ipaddr[0] & 0xFF) << 0));
  _addrStr = (QString::number( ipaddr[3] ) + '.' +
	      QString::number( ipaddr[2] ) + '.' +
	      QString::number( ipaddr[1] ) + '.' +
	      QString::number( ipaddr[0] ));

  // Initialize the buffer
  memset( _recvBuffer, 0xFF, sizeof(_recvBuffer) );

  // Create and connect the socket
  _sock = new QUdpSocket();
  if( !_sock->bind( QHostAddress(_addr), _port ) )
    {
      _errString = QString("Failed to bind to adapter/port ") + _addrStr +
	QString(": ") + _sock->errorString();
      _stop = true;
    }

  // Start the thread (see run())
  this->start( QThread::TimeCriticalPriority );
}

// ----------------------------------------------------------------------------

ReceiverThread::~ReceiverThread()
{
  // In case the thread is still running...
  this->stop();
}

// ----------------------------------------------------------------------------

void ReceiverThread::stop()
{
  if( this->isRunning() )
    {
      _stop = true;
      //this->exit(); // Stop this thread's event loop
      this->wait(); // Wait until this thread (i.e. function run()) exits
      _sock->close();
      delete _sock;
    }
}

// ----------------------------------------------------------------------------

void ReceiverThread::run()
{
  while( !_stop )
    {
      if( _sock->waitForReadyRead( 100 ) )
	this->readDatagrams();

      // Synchronize with reset and set-buffersize actions
      // (but it doesn't protect us from calls to updateBytesConsumed()
      //  which may arrive after _suspend has been reset to false)
      if( _suspend && !_suspended ) _suspended = true;
    }
}

// ----------------------------------------------------------------------------

void ReceiverThread::readDatagrams()
{
  long long recvd_sz, space, tmphead;

  while( _sock->hasPendingDatagrams() )
    {
      // After 'full buffer' occurred all subsequent data is flushed
      // until this state is explicitly reset (by resetFullBufferOccured())
      if( _fullOccurred || _suspended )
	{
	  recvd_sz = _sock->readDatagram( _flushBuffer, 16384 ) ;
	  if( recvd_sz <= 0 || _suspended ) continue;

	  ++_packetsLost;
	  _bytesLost += recvd_sz;
	  _lastPacketSize = recvd_sz;
	  continue;
	}

      recvd_sz = _sock->readDatagram( &_recvBuffer[_head], 16384 );
      if( recvd_sz <= 0 ) continue;
      tmphead = _head + recvd_sz;

      // Wrap-around the end of the buffer (with some spare space)
      if( tmphead + 16384 > _bufferSize )
	{
	  // Remember position of last byte near the end of the buffer !
	  _headEnd = tmphead;
	  // And we continue from the start of the buffer
	  tmphead = 0;
	  // Just for interest, keep track of wrap-arounds
	  ++_bufferWraps;
	}

      // Check buffer space left
      space = _tail - tmphead;
      if( space >= 0 && space < 16384 )
	{
	  _full = true;
	  _fullOccurred = true;
	}
      _head = tmphead;

      // Statistics
      ++_packetsReceived;
      _bytesReceived += recvd_sz;
      _lastPacketSize = recvd_sz;
    }
}

// ----------------------------------------------------------------------------

long long ReceiverThread::bytesAvailable()
{
  // Return the number of data bytes available for processing at this time
  // without passing the end of the buffer
  long long bytes;
  long long t = _tail;
  long long h = _head;
  if( t < h )
    {
      bytes = h - t;
    }
  else
    {
      if( h == t && !_full )
	bytes = 0;
      else
	// Give the number of bytes left up to the end of the buffer
	// (so that the consumer does not have to deal with buffer wrap-around)
	bytes = _headEnd - _tail;
    }
  return bytes;
}

// ----------------------------------------------------------------------------

void ReceiverThread::updateBytesConsumed( long long bytes )
{
  if( _suspend ) return;

  // An amount of 'bytes' bytes have been consumed: update the tail pointer
  long long t = _tail + bytes;
  // Wrap-around the end of the buffer when appropriate
  if( t == _headEnd ) t = 0;
  _full = false;
  _tail = t;
}

// ----------------------------------------------------------------------------

void ReceiverThread::reset()
{
  // Prevent thread from writing anything more into the buffer
  _suspend = true;
  // Now wait to make sure this thread takes '_suspend' into account...
  volatile bool b = _suspended;
  while( !b ) b = this->suspended();

  _packetsReceived = 0;
  _packetsLost     = 0;
  _bytesReceived   = 0;
  _bytesLost       = 0;
  _lastPacketSize  = 0;
  _head            = 0;
  _tail            = 0;
  _full            = false;
  _fullOccurred    = false;

  _suspend         = false;
  _suspended       = false;
}

// ----------------------------------------------------------------------------

bool ReceiverThread::setBufferSize( long long size )
{
  if( size < 1 || size > RECV_BUF_SIZE ) return false;

  this->reset();
  _bufferSize = size;

  return true;
}

// ----------------------------------------------------------------------------

std::string ReceiverThread::ipAddressString()
{
  QString qs = _addrStr + ':' + QString::number( _port );
  return qs.toStdString();
}

// ----------------------------------------------------------------------------

std::string ReceiverThread::errorString()
{
  if( _errString.isEmpty() ) return std::string( "" );
  QString qs = "Port " + QString::number( _port ) + ": " + _errString;
  return qs.toStdString();
}

// ----------------------------------------------------------------------------

bool ReceiverThread::suspended()
{
  // This function was only added to prevent the compiler from optimizing
  // a while-loop (see reset()) on the _suspended variable!
  return _suspended;
}

// ----------------------------------------------------------------------------

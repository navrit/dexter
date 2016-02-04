#include <iostream>
using namespace std;

#include <QHostAddress>
#include <QString>

extern void usage();

// ----------------------------------------------------------------------------

quint32 get_addr_and_port( const char *str, int *portnr )
{
  QString qstr( str );
  if( qstr.contains( QChar(':') ) )
    {
      // A port number is provided: extract it
      bool ok;
      int p = qstr.section( ':', 1, 1).toInt( &ok );
      if( !ok )
	{
	  cout << "### Invalid port number: "
	       << qstr.section( ':', 1, 1 ).toStdString() << endl;
	  usage();
	  exit( 0 );
	}
      else
	{
	  *portnr = p;
	}
      // Remove the port number from the string
      qstr = qstr.section( ':', 0, 0 );
    }
  QHostAddress qaddr;
  if( !qaddr.setAddress( qstr ) )
    {
      cout << "### Invalid IP address: " << qstr.toStdString() << endl;
      usage();
      exit( 0 );
    }
  return qaddr.toIPv4Address();
}

// ----------------------------------------------------------------------------

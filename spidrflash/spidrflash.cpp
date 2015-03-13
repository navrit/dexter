#include <iostream>
#include <iomanip>
using namespace std;

#include <cstring> // For memcmp()

#include "McsReader.h"
#include "SpidrController.h"

#include <QHostAddress>
#include <QString>

#define error_out(str) cout<<str<<": "<<spidrctrl.errorString()<<endl

quint32 get_addr_and_port( const char *str, int *portnr );
int     my_atoi( const char *c );
void    usage();

// ----------------------------------------------------------------------------

int main( int argc, char **argv )
{
  if( !(argc == 4 || argc == 5) )
    {
      usage();
      return 1;
    }

  // Get arguments
  quint32 ipaddr = 0;
  int     portnr = 50000;
  ipaddr = get_addr_and_port( argv[1], &portnr );
  int flash_i = my_atoi( argv[3] );
  if( !(flash_i == 1 || flash_i == 2) )
    {
      cout << "### Illegal flash ID" << endl;
      usage();
      return 1;
    }
  bool program_it = false;
  bool dump_it    = false;
  if( argc == 5 )
    {
      if( string(argv[4]) == "prog" )
	program_it = true;
    }
  else
    {
      // Instead of file name, some special options...
      if( string(argv[2]) == "dump" )
	dump_it = true;
    }

  // Open a control connection to the SPIDR module
  SpidrController spidrctrl( (ipaddr>>24) & 0xFF,
			     (ipaddr>>16) & 0xFF,
			     (ipaddr>> 8) & 0xFF,
			     (ipaddr>> 0) & 0xFF, portnr );
  // Are we connected ?
  if( !spidrctrl.isConnected() ) {
    cout << spidrctrl.ipAddressString() << ": "
         << spidrctrl.connectionStateString() << ", "
         << spidrctrl.connectionErrString() << endl;
    return 1;
  }
  // Don't generate informational display output...
  spidrctrl.setLogLevel( 2 ); // Set to WARNING level

  McsReader mcs;
  string filename = string( argv[2] );
  if( !dump_it )
    {
      // Read the MCS/HEX file
      try
	{
	  mcs.setFile( filename );
	}
      catch( McsException &exc )
	{
	  string msg;
	  msg += filename;
	  msg += ": ";
	  msg += exc.toString();
	  cout << msg;
	  return 1;
	}
      cout << "MCS-file " << filename << ": min addr " << mcs.minAddr()
	   << ", max addr: " << mcs.maxAddr() << endl << endl;
    }

  unsigned char *flashmem = mcs.mem();
  int  address         = mcs.minAddr();
  int  bytes_to_do     = mcs.maxAddr() - mcs.minAddr();
  int  bytes_done      = 0;
  int  percentage      = -1;
  int  nbytes;
  unsigned char databytes[2048];
  if( dump_it )
    {
      // Just dump the contents of the flash device
      address     = 0;
      bytes_to_do = 0x1000000; // 16 MByte
      cout << hex << uppercase << setfill('0');
      while( bytes_done < bytes_to_do )
	{
	  if( spidrctrl.readFlash( flash_i, address, &nbytes, databytes ) )
	    {
	      int i;
	      for( i=0; i<nbytes; ++i, ++address )
		{
		  if( (address & 0x0F) == 0 )
		    cout << endl << setw(6) << address << ' ';
		  cout << ' ' << setw(2) << (unsigned int) databytes[i];
		}
	      address    += nbytes;
	      bytes_done += nbytes;

	      cout << endl;
	      char ch;
	      cin >> ch;
	      if( ch == 'q' || ch == 'Q' ) return 0;
	    }
	  else
	    {
	      error_out( "readFlash()" );
	      break;
	    }
	}
    }
  else if( program_it )
    {
      // If necessary, adjust the start address downwards to the start
      // of the flash memory (sub)sector in which it is located
      // (to make sure SPIDR erases this (sub)sector)
      //if( (address & 0xFFFF) != 0 ) // N25Q128 sector size is 64 KByte
      if( (address & 0xFFF) != 0 )    // N25Q128 subsector size is 4 KByte
	{
	  bytes_to_do += (address & 0xFFF);
	  address     &= 0xFFFFF000;
	}
      flashmem += address;

      cout << "Programming...     ";
      while( bytes_done < bytes_to_do )
	{
	  if( spidrctrl.writeFlash( flash_i, address, 1024, flashmem ) )
	    {
	      /* DEBUG
	      cout << hex << uppercase << setfill('0');
	      int i;
	      for( i=0; i<128; ++i )
		{
		  if( ((address+i) & 0x0F) == 0 )
		    cout << endl << setw(6) << address+i << ' ';
		  cout << ' ' << setw(2) << (unsigned int) flashmem[i];
		}
	      cout << endl << dec;
	      */
	      flashmem   += 1024;
	      address    += 1024;
	      bytes_done += 1024;

	      // Display percentage done
	      if( (100*bytes_done)/bytes_to_do != percentage )
		{
		  int p = (100*bytes_done)/bytes_to_do;
		  cout << "\b\b\b\b\b    \b\b\b\b" << setw(3) << p << "% ";
		  percentage = p;
		}
	    }
	  else
	    {
	      error_out( "writeFlash()" );
	      break;
	    }
	}
    }
  else
    {
      cout << "Verifying...     ";
      flashmem += address;
      while( bytes_done < bytes_to_do )
	{
	  if( spidrctrl.readFlash( flash_i, address, &nbytes, databytes ) )
	    {
	      // Check data retrieved against file contents
	      // and stop as soon as a difference is found
	      if( memcmp( flashmem, databytes, nbytes ) != 0 )
		{
		  cout << hex << uppercase << endl;
		  cout << "### Mismatch in datablock at address "
		       << address << endl;

		  // Display a number of differences in more detail
		  int i, cnt = 0;
		  for( i=0; i<nbytes; ++i )
		    {
		      if( databytes[i] != flashmem[i] )
			{
			  cout << "addr " << setw(6) << (address+i)
			       << ": " << setw(2)
			       << (unsigned int) databytes[i]
			       << " vs " << setw(2)
			       << (unsigned int) flashmem[i] << endl;
			  ++cnt;
			  if( cnt == 10 ) break; // for-loop
			}
		    }
		  break; // while-loop
		}

	      flashmem   += nbytes;
	      address    += nbytes;
	      bytes_done += nbytes;

	      // Display percentage done
	      if( (100*bytes_done)/bytes_to_do != percentage )
		{
		  int p = (100*bytes_done)/bytes_to_do;
		  cout << "\b\b\b\b\b    \b\b\b\b" << setw(3) << p << "% ";
		  percentage = p;
		}
	    }
	  else
	    {
	      error_out( "readFlash()" );
	      break;
	    }
	}
      if( bytes_done >= bytes_to_do )
	cout << endl << "Verified OKAY!" << endl;
    }
  return 0;
}

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

int my_atoi( const char *c )
{
  int value = 0;
  while( *c >= '0' && *c <= '9' )
    {
      value *= 10;
      value += (int) (*c-'0');
      ++c;
    }
  return value;
}

// ----------------------------------------------------------------------------

void usage()
{
  cout << endl << "Usage:" << endl
       << "spidrflash <ipaddr>[:<portnr>] <filename> <1|2> [prog]"
       << endl
       << "   Verify or program a Compact-SPIDR's flash device with"
       << " the given ID (1 or 2)."
       << endl
       << "   Verifies the flash memory contents against the file contents"
       << endl
       << "   on the module with the given IP address and port number "
       << "(default 50000)."
       << endl
       << "   Programs the file contents into flash memory when keyword"
       << endl
       << "   'prog' is appended."
       << endl
       << "   <filename> : name of the MCS/HEX file containing FPGA code."
       << endl;
}

// ----------------------------------------------------------------------------

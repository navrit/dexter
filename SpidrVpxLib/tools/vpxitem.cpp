#include <iostream>
#include <iomanip>
using namespace std;

#include <stdio.h>
#include <stdlib.h>

#include "SpidrController.h"
#ifdef WIN32
#include "stdint.h"
#include "wingetopt.h"
#endif

uint32_t get_addr_and_port( const char *str, int *portnr );
void usage();
void arg_error( char opt );
void arg_range( char opt, int min, int max );

#define error_out(str) cout<<str<<": "<<spidrctrl.errorString()<<endl

// Version identifier: year, month, day, release number
const int VERSION_ID = 0x16070400;

// ----------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  int      opt;
  uint32_t ipaddr    = 0;
  int      portnr    = 50000;
  char    *item_name = 0;
  char    *reg_name  = 0;
  int      item_i    = 0;
  int      reg_i     = 0;
  bool     write_it  = false;
  string   info;
  int      id;

  // Parse the options
  while( (opt = getopt(argc, argv, "hi:r:v")) != -1 )
    {
      switch( opt )
	{
	case 'h':
	  usage();
	  return 0;
	case 'i':
	  // SPIDR IP address
	  ipaddr = get_addr_and_port( optarg, &portnr );
	  break;
	case 'r':
          if( sscanf( optarg, "%d", &reg_i ) != 1 )
            arg_error( 'r' );
	  break;
	case 't':
          if( sscanf( optarg, "%d", &item_i ) != 1 )
            arg_error( 't' );
	  break;
	case 'v':
	  cout << "Version " << hex << VERSION_ID << dec << endl;
	  return 0;
	default: // '?'
	  usage();
	  return 0;
	}
    }

  if( optind < argc )
    {
      // Convert all chars to uppercase and change any '-' into '_'
      char *name = argv[optind];
      for( int i=0; i<strlen(name); ++i )
	{
	  name[i] = toupper( name[i] );
	  if( name[i] == '-' )
	    name[i] = '_';
	}

      // Does the request concern a register ?
      if( strncmp( "REG", name, 3 ) == 0 )
	{
	  reg_name = name;
	}
      else
	{
	  // Check if a number is provided:
	  // to be considered a register address..
	  if( !(name[0] >= '0' && name[0] <= '9' &&
		sscanf( name, "%x", &id ) == 1) )
	    item_name = name;
	}
    }
  else
    {
      cout << "Provide an item or register name" << endl;
      usage();
      return 0;
    }

  if( optind < argc-1 )
    {
      // A value to be written is provided, apparently
      char *towrite = argv[optind+1];
      bool  ishex = false;
      if( strlen(towrite) > 2 &&
	  towrite[0] == '0' && tolower(towrite[1]) == 'x' )
	ishex = true;

      // TO BE DONE...

      cout << "value: " << towrite << ", hex=" << ishex << endl;

      write_it = true;
    }

  if( item_name )
    id = SpidrController::itemId( item_name, info );
  else if( reg_name )
    id = SpidrController::regAddr( reg_name, info );

  if( ipaddr == 0 )
    {
      // Just want info about the item or register
      if( item_name )
	{
	  if( id >= 0 )
	    cout << item_name << ": " << info << endl;
	  else if( info.empty() )
	    cout << "### Item \"" << item_name << "\" not found" << endl;
	  else
	    cout << "Available options:" << endl << info;
	}
      else if( reg_name )
	{
	  if( id >= 0 )
	    cout << reg_name << ": " << info << endl;
	  else if( info.empty() )
	    cout << "### Reg \"" << reg_name << "\" not found" << endl;
	  else
	    cout << "Available options:" << endl << info;
	}
      else
	{
	  cout << "Reg 0x" << hex << setfill('0') << setw(4) << id
	       << ": " << SpidrController::regName( id ) << endl;
	}

      return 0;
    }
  else
    {
      if( id < 0 )
	{
	  if( !info.empty() )
	    {
	      cout << "Available options:" << endl << info;
	    }
	  else
	    {
	      if( item_name )
		cout << "### Item \"" << item_name << "\" not found" << endl;
	      else if( reg_name )
		cout << "### Reg \"" << reg_name << "\" not found" << endl;
	      return 1;
	    }
	}

      // Open a control connection to the SPIDR module
      // with the given address and port, or -if the latter was not provided-
      // the default port number 50000
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

      if( item_name )
	{
	  // Display some info about the requested item
	  // followed by its value
	  cout << info << ": ";

	  int val;
	  if( !spidrctrl.getVpxItem( id, item_i, reg_i, &val ) )
	    {
	      error_out( "###getVpxItem" );
	      return 1;
	    }

	  cout << val << endl;
	}
      else if( reg_name )
	{
	  // Display some info about the requested register
	  // followed by its contents
	  cout << info << ": ";

	  int nbytes;
	  unsigned char data[256];
	  if( !spidrctrl.getVpxReg( id + reg_i, &nbytes, data ) )
	    {
	      error_out( "###getVpxReg" );
	      return 1;
	    }

	  cout << hex << setfill( '0' );
	  for( int i=0; i<nbytes; ++i )
	    cout << " " << setw(2) << (unsigned int) data[i];
	  cout << dec << endl;
	}
      else
	{
	  // A literal (register) address was provided
	  int nbytes;
	  unsigned char data[256];
	  if( !spidrctrl.getVpxReg( id + reg_i, &nbytes, data ) )
	    {
	      error_out( "###getVpxReg" );
	      return 1;
	    }

	  // Display the requested register's contents
	  cout << hex << setfill( '0' )
	       << "Reg 0x" << setw(4) << id + reg_i << ":";
	  for( int i=0; i<nbytes; ++i )
	    cout << " " << setw(2) << (unsigned int) data[i];
	  cout << dec << endl;
	}
    }

  return 0;
}

// ----------------------------------------------------------------------------

void usage()
{
  cout <<
    "Usage: vpxitem [-h|v] [-i <ipaddr>] item_name|<addr>\n"
    "  -h         : this help text\n"
    "  -v         : display version\n"
    "  -i <ipaddr>: IP address of SPIDR\n"
    "  -r <index> : Velopix register index (in case of multiple registers)\n"
    "  -t <index> : Velopix register item index "
    "(in case of multiple item groups per register)\n"
    " item_name   : a name such as BXID_GRAY_OR_BIN or bxid-gray-or-bin.\n"
    " <addr>      : a register address (hexadecimal).\n";
}

// ----------------------------------------------------------------------------

void arg_error( char opt )
{
  cout << "### -" << opt << ": error in argument" << endl;
  usage();
  exit( 0 );
}

// ----------------------------------------------------------------------------

void arg_range( char opt, int min, int max )
{
  cout << "### -" << opt << ": argument not in range ("
       << min << ".." << max << ")" << endl;
  exit( 0 );
}

// ----------------------------------------------------------------------------

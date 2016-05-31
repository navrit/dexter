// INTEL-HEX file (.mcs) interpreter class for flash memory images.

#include <stdio.h>
#include <cstring>

#include "McsReader.h"

#ifdef WIN32
// Prevent some warnings...
#define snprintf _snprintf_s
#endif

static const int MEM_SZ        = 0x1000000;
static const int MEM_PAGE_SZ   = 0x100;
static const int MEM_SECTOR_SZ = 0x10000;

/* Indices in INTEL-HEX formatted line of code */
#define HEX_NO_OF_BYTES_I          1
#define HEX_ADDRESS_HI_I           3
#define HEX_ADDRESS_LO_I           5
#define HEX_TYPE_I                 7
#define HEX_DATA_I                 9

// ----------------------------------------------------------------------------

McsReader::McsReader()
  : _valid( false ),
    _minAddr( -1 ),
    _maxAddr(-1),
    _lineNo(0),
    _mem( 0 )
{
  // Allocate storage for full FLASH or EEPROM memory image;
  // e.g. for the N25Q128 device this is 16MBytes
  _mem = new byte[MEM_SZ];

  // Initialize memory image to all 0xFFs
  memset( _mem, 0xFF, MEM_SZ );
}

// ----------------------------------------------------------------------------

McsReader::~McsReader()
{
  if( _mem ) delete _mem;
}

// ----------------------------------------------------------------------------

byte* McsReader::page( int page_no )
{
  if( page_no < MEM_SZ/MEM_PAGE_SZ )
    return &(_mem[page_no*MEM_PAGE_SZ]);
  else
    return 0;
}

// ----------------------------------------------------------------------------

bool McsReader::pageIsEmpty( int page_no )
{
  if( page_no < MEM_SZ/MEM_PAGE_SZ )
    {
      bool empty = true;
      byte *page = &_mem[page_no*MEM_PAGE_SZ];
      for( int i=0; i<256; ++i )
	if( page[i] != 0xFF )
	  {
	    empty = false;
	    break;
	  }
      return empty;
    }
  else
    return true;
}

// ----------------------------------------------------------------------------

void McsReader::setFile( std::string filename )
{
  FILE  *fp;
  char  inp[100], *pch;
  int   line_no = 0;
  byte  no_of_bytes;
  int   addr, max_addr = 0, min_addr = MEM_SZ;
  int   segoffset = 0;
  byte  hextype;
  byte  databytes[32];
  byte  checksum;
  int   i;

  _valid    = false;
  _fileName = "";

  if( (fp = fopen( filename.c_str(), "r" )) == NULL )
    {
      McsException exc( "could not open file " );
      throw exc;
    }

  while( fgets( inp, sizeof(inp), fp ) != NULL )
    {
      ++line_no;

      /* Skip to first sensible input character */
      pch = strtok( inp, " \t\n" );

      if( pch == NULL ) continue;

      if( *pch == ':' )
	{
	  /* This is a line containing bytes to download */
	  try
	    {
	      this->interpretHexline( pch, &no_of_bytes, &addr, &hextype,
				      databytes, &checksum );
	    }
	  catch( McsException &exc )
	    {
	      fclose( fp );
	      // Add the line number to the exception
	      exc.setLineNo( line_no );
	      // Rethrow
	      throw;
	    }

	  switch( hextype )
	    {
	    case 0x00:
	      /* Line with data bytes (Data Record) */

	      /* Add segment offset to address */
	      addr += segoffset;

	      /* Keep track of highest/lowest memory address used */
	      if( addr+no_of_bytes-1 > max_addr )
		max_addr = addr + no_of_bytes - 1;
	      if( addr < min_addr ) min_addr = addr;

	      /* Copy databytes to memory image */
	      for( i=0; i<no_of_bytes; ++i )
		_mem[addr+i] = databytes[i];

	      break;

	    case 0x01:
	      /* Line signifying end-of-data (End-of-file Record) */
	      if( no_of_bytes != 0 || addr != 0 )
		{
		  fclose( fp );
		  McsException exc( "error in End-of-Data (RecType 1) ",
				    line_no );
		  throw exc;
		}
	      //printf( "Line %4d: End-of-Data hexline-type\n", line_no );
	      break;

	    case 0x02:
	      /* Line with Extended Segment Address Record (HEX86):
		 Type '02' is used to preset the Extended Segment Address.
		 With this segment address it is possible to send files up
		 to 1Mb in length. The Segment address is multiplied by 16
		 and then added to all subsequent address fields of type '00'
		 records to obtain the effective address. By default the
		 Extended Segment address will be $0000, until it is specified
		 by a type '02' record. The address field of a type '02'
		 record must be $00. The byte count field will be $02
		 (the segment address consists of 2 bytes).
		 The data field of the type '02' record contains the actual
		 Extended Segment address. Bits 3..0 of this Extended Segment
		 address always must be 0!  */
	      if( no_of_bytes != 2 || addr != 0 )
		{
		  fclose( fp );
		  McsException exc( "error in Ext Segm Addr (RecType 2) ",
				    line_no );
		  throw exc;
		}
	      else
		{
		  segoffset = ((databytes[0] << 8) | databytes[1]) * 16;
		  if( segoffset > 0x10000 )
		    {
		      fclose( fp );
		      McsException exc( "illegal Extended Segm Addr (RecType",
					line_no, segoffset );
		      throw exc;
		    }
		  //printf( "Line %4d: Extended-Segm-Addr x%x\n",
		  //  line_no, segoffset );
		}
	      break;

	    case 0x04:
	      /* Line with Extended Linear Address Record (HEX386):
		 Type '04' is used to contain the upper 16 bits (bits 16-31)
		 of the data address. */
	      if( no_of_bytes != 2 || addr != 0 )
		{
		  fclose( fp );
		  McsException exc( "error in Ext Linear Addr (RecType 4) ",
				    line_no );
		  throw exc;
		}
	      else
		{
		  segoffset = ((databytes[0] << 8) | databytes[1]) << 16;
		  if( segoffset > MEM_SZ-0x10000 )
		    {
		      fclose( fp );
		      McsException exc( "illegal Ext Linear Addr (RecType 4)",
				    line_no, segoffset );
		      throw exc;
		    }
		  //printf( "Line %4d: Extended-Linear-Addr 0x%X\n",
		  //  line_no, segoffset );
		}
	      break;

	    default:
	      fclose( fp );
	      McsException exc( "unknown hexline-type ", line_no, hextype );
	      throw exc;
	    }
	}
    }
  fclose( fp );

  _lineNo  = line_no;
  _minAddr = min_addr;
  _maxAddr = max_addr;
  if( _minAddr >= MEM_SZ )
    {
      McsException exc( "file seems to contain no HEX code " );
      throw exc;
    }
  if( _maxAddr >= MEM_SZ )
    {
      McsException exc( "maximum flash address exceeded ", line_no, max_addr );
      throw exc;
    }
  
  _valid    = true;
  _fileName = filename;
}

// ----------------------------------------------------------------------------

void McsReader::interpretHexline( char *hexline,
			       byte *pno_of_bytes,
			       int  *paddr,
			       byte *phextype,
			       byte *databytes,
			       byte *pchecksum )
{
  byte addr_hi, addr_lo;
  int  no_of_bytes, checksum, i;

  /* Does line have minimum number of characters ? */
  if( strlen(hexline) < 11 )
    {
      throw McsException( "line too short" );
    }

  /* Extract info about the code bytes */
  *pno_of_bytes = this->str2Byt( hexline + HEX_NO_OF_BYTES_I );
  no_of_bytes   = (int) *pno_of_bytes;
  addr_hi       = this->str2Byt( hexline + HEX_ADDRESS_HI_I );
  addr_lo       = this->str2Byt( hexline + HEX_ADDRESS_LO_I );
  *paddr        = ((int) addr_hi << 8) + (int) addr_lo;
  *phextype     = this->str2Byt( hexline + HEX_TYPE_I );

  /* Does line have enough characters to contain all data bytes ? */
  if( strlen(hexline) < (size_t) (11 + no_of_bytes*2) )
    {
      throw McsException( "line with missing data" );
    }

  /* Extract the code bytes */
  for( i=0; i<no_of_bytes; ++i )
    databytes[i] = this->str2Byt( hexline + HEX_DATA_I + i*2 );

  /* Extract the checksum */
  *pchecksum = this->str2Byt( hexline + HEX_DATA_I + no_of_bytes*2 );

  /* Check correctness of the check sum */
  checksum  = 0;
  checksum += no_of_bytes;
  checksum += addr_hi;
  checksum += addr_lo;
  checksum += (*phextype);
  for( i=0; i<no_of_bytes; ++i ) checksum += databytes[i];
  checksum = ((~checksum) + 1) & 0xFF;
  if( checksum != *pchecksum )
    {
      std::string str;
      char        ch[32];
      str += "checksum error (read ";
      //str += std::string( _itoa(*pchecksum, ch, 16) );
      snprintf( ch, 32, "%02X", *pchecksum );
      str += std::string( ch );
      str += ", calculated ";
      //str += std::string( _itoa(checksum, ch, 16) );
      snprintf( ch, 32, "%02X", checksum );
      str += std::string( ch );
      str += ")";
      throw McsException( str );
      //printf( "###HEX: checksum error (read %02x, calculated %02x)\n",
      //      *pchecksum, checksum );
    }
}

// ----------------------------------------------------------------------------

byte McsReader::str2Byt( char *str )
{
  byte val = 0;

  /* High-order nibble */
  if( *str >= '0' && *str <= '9' ) val += (*str - '0');
  if( *str >= 'A' && *str <= 'F' ) val += (*str - 'A') + 10;
  if( *str >= 'a' && *str <= 'f' ) val += (*str - 'a') + 10;

  val <<= 4;

  ++str;

  /* Low-order nibble */
  if( *str >= '0' && *str <= '9' ) val += (*str - '0');
  if( *str >= 'A' && *str <= 'F' ) val += (*str - 'A') + 10;
  if( *str >= 'a' && *str <= 'f' ) val += (*str - 'a') + 10;

  /* Return resulting hex-value */
  return val;
}

// ----------------------------------------------------------------------------
// CLASS McsException
// ----------------------------------------------------------------------------

McsException::McsException( std::string str, int line_no, int data )
  : _str( str ), _lineNo( line_no ), _data( data )
{
}

// ----------------------------------------------------------------------------

McsException::McsException( std::string str, int line_no )
  : _str( str ), _lineNo( line_no ), _data( 0 )
{
}

// ----------------------------------------------------------------------------

McsException::McsException( std::string str )
  : _str( str ), _lineNo( 0 ), _data( 0 )
{
}

// ----------------------------------------------------------------------------

McsException::~McsException()
{
}

// ----------------------------------------------------------------------------

void McsException::setLineNo( int line_no )
{
  _lineNo = line_no;
}

// ----------------------------------------------------------------------------

std::string McsException::toString()
{
  std::string str;// = "###MCS-file ";
  str += "line ";
  char ch[32];
  //str += std::string( _itoa(_lineNo, ch, 10) );
  snprintf( ch, 32, "%d", _lineNo );
  str += std::string( ch );
  str += ", ";
  str += _str;
  return str;
}

// ----------------------------------------------------------------------------

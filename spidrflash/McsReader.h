/* -------------------------------------------------------------------------
File   : McsReader.h

Descr  : INTEL-HEX file (.mcs) interpreter class for flash memory images.

History: 06MAR15; Created, based on ELMBloader's HexExe.cpp/.h.
---------------------------------------------------------------------------- */

#ifndef MCSREADER_H
#define MCSREADER_H

#include <string>

typedef unsigned char byte;

// ----------------------------------------------------------------------------

class McsReader
{
 public:

  McsReader();

  ~McsReader();

  unsigned char* mem()     { return _mem; }
  unsigned char* page( int page_no );
  bool  pageIsEmpty( int page_no );

  int   maxAddr() { return _maxAddr; }
  int   minAddr() { return _minAddr; }

  int   lineNo()  { return _lineNo; }

  bool  isValid() { return _valid; }

  std::string fileName() { return _fileName; }

  void  setFile( std::string filename );

 private:
  void  interpretHexline( char *hexline,
              unsigned char *pno_of_bytes,
			  int  *paddr,
              unsigned char *phextype,
              unsigned char *databytes,
              unsigned char *pchecksum );

  unsigned char  str2Byt( char *str );

  bool        _valid;
  int         _minAddr;
  int         _maxAddr;
  int         _lineNo;
  std::string _errString;
  std::string _fileName;

  // Pointer to storage for full FLASH or EEPROM memory image;
  // e.g. for the N25Q128 device: 16MBytes
  unsigned char *_mem;
};

// ----------------------------------------------------------------------------

class McsException
{
 public:
  McsException( std::string str, int line_no, int data );
  McsException( std::string str, int line_no );
  McsException( std::string str );

  ~McsException();

  void        setLineNo( int line_no );
  std::string toString();

 private:
  std::string _str;
  int         _lineNo;
  int         _data;
};

// ----------------------------------------------------------------------------
#endif // MCSREADER_H


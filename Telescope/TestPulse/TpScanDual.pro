# 
# Project file 
# 
# To generate a Visual Studio project: 
#   qmake -t vcapp <name>.pro 
# To generate a Makefile: 
#   qmake <name>.pro 
# 
# (or start from a 'SUBDIRS' .pro file to recursively generate your projects) 
# 
TEMPLATE = app 
TARGET   = TpScanDual  
 
# Create a Qt app 
QT -= gui
QT += core
CONFIG += qt thread console warn_on exceptions debug_and_release
 
CONFIG(debug, debug|release) { 
  OBJECTS_DIR = debug 
  MOC_DIR     = debug 
  UI_DIR      = debug 
  DESTDIR     = ../../Debug 
  LIBS       += -L../../Debug 
} 
 
CONFIG(release, debug|release) { 
  OBJECTS_DIR = release 
  MOC_DIR     = release 
  UI_DIR      = release 
  DESTDIR     = ../../Release 
  LIBS       += -L../../Release 
} 
 
INCLUDEPATH += ../../SpidrTpx3Lib 
 
LIBS += -lSpidrTpx3Lib 
 
SOURCES   += TpScanDual.cpp 
 
#HEADERS   +=  

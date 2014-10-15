# 
# Project file for the SpidrMon tool 
# 
# To generate a Visual Studio project: 
#   qmake -t vcapp SpidrDacs.pro 
# To generate a Makefile: 
#   qmake SpidrDacs.pro 
# 
# (or start from the 'SUBDIRS' .pro file to recursively generate the projects) 
# 
TEMPLATE = app 
TARGET   = Equalise
 
# Create a Qt app 
QT -= gui
QT += core
CONFIG += qt thread console warn_on exceptions debug_and_release
 
CONFIG(debug, debug|release) { 
  OBJECTS_DIR = debug 
  MOC_DIR     = debug 
  UI_DIR      = debug 
  DESTDIR     = ../../../Debug 
  LIBS       += -L../../../Debug 
} 
 
CONFIG(release, debug|release) { 
  OBJECTS_DIR = release 
  MOC_DIR     = release 
  UI_DIR      = release 
  DESTDIR     = ../../../Release 
  LIBS       += -L../../../Release 
} 

QMAKE_CXXFLAGS += -pthread -Wno-deprecated-declarations -m64 -I/home/timepix3/root/include

INCLUDEPATH += ../../../SpidrTpx3Lib 
INCLUDEPATH += ../Lib 
 
LIBS += -lSpidrTpx3Lib 
LIBS += -lSpidrEqualisation
LIBS += `root-config --glibs`
 
SOURCES   += Equalise.cpp 

#HEADERS   +=  

#
# Project file for the SpidrTpx3Lib library
#
# To generate a Visual Studio project:
#   qmake -t vclib SpidrTpx3Lib.pro
# To generate a Makefile:
#   qmake SpidrTpx3Lib.pro
#
# (or start from the 'SUBDIRS' .pro file to recursively generate the projects)
#
TEMPLATE = lib
TARGET   = SpidrTpx3Lib

# Create a shared library
QT -= gui
QT += core network
CONFIG += shared qt thread warn_on exceptions debug_and_release

DEFINES += MY_LIB_EXPORT

CONFIG(debug, debug|release) {
  OBJECTS_DIR = debug
  MOC_DIR     = debug
  UI_DIR      = debug
  DESTDIR     = ../Debug
}

CONFIG(release, debug|release) {
  OBJECTS_DIR = release
  MOC_DIR     = release
  UI_DIR      = release
  DESTDIR     = ../Release
}

win32 {
  LIBS += -lWs2_32
}

SOURCES += SpidrController.cpp

HEADERS += SpidrController.h
HEADERS += tpx3defs.h dacsdescr.h
HEADERS += spidrtpx3cmds.h

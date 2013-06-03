#
# Project file for the SpidrPixelman library
#
# To generate a Visual Studio project:
#   qmake -t vclib SpidrPixelman.pro
# To generate a Makefile:
#   qmake SpidrPixelman.pro
#
# (or start from the 'SUBDIRS' .pro file to recursively generate the projects)
#
TEMPLATE = lib
TARGET   = SpidrPixelman

# Create a shared library
#QT -= gui
#QT += core network
CONFIG += shared thread warn_on exceptions debug_and_release

DEFINES += MY_LIB_EXPORT

CONFIG(debug, debug|release) {
  OBJECTS_DIR = debug
  MOC_DIR     = debug
  UI_DIR      = debug
  DESTDIR     = ../Debug
  LIBS       += -L../Debug
}

CONFIG(release, debug|release) {
  OBJECTS_DIR = release
  MOC_DIR     = release
  UI_DIR      = release
  DESTDIR     = ../Release
  LIBS       += -L../Release
}

LIBS += -lSpidrLib

INCLUDEPATH += ../SpidrLib
INCLUDEPATH += common

SOURCES += SpidrMgr.cpp
SOURCES += spidrhwapi.cpp

HEADERS += ../SpidrLib/SpidrController.h
HEADERS += SpidrMgr.h
HEADERS += spidrhwapi.h
HEADERS += common/common.h common/mpx3hw.h common/mpxerrors.h

#
# Project file for the spidrflash program
#
# To generate a Visual Studio project:
#   qmake -t vcapp spidrflash.pro
# To generate a Makefile:
#   qmake spidrflash.pro
#
# (or start from the 'SUBDIRS' .pro file to recursively generate the projects)
#
TEMPLATE = app
TARGET   = spidrflash

# Create a console app
QT -= gui
QT += core network
CONFIG += qt console warn_on exceptions debug_and_release

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

LIBS += -lSpidrMpx3Lib

INCLUDEPATH += ../SpidrMpx3Lib

SOURCES += McsReader.cpp
SOURCES += spidrflash.cpp

HEADERS += McsReader.h

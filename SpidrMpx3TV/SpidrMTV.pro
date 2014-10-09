#
# Project file for the SpidrMTV tool
#
# To generate a Visual Studio project:
#   qmake -t vcapp SpidrMTV.pro
# To generate a Makefile:
#   qmake SpidrMTV.pro
#
# (or start from the 'SUBDIRS' .pro file to recursively generate the projects)
#
TEMPLATE = app
TARGET   = SpidrMTV

# Create a Qt app
QT += network
CONFIG += qt thread warn_on exceptions debug_and_release

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

win32 {
  RC_FILE = spidrtv.rc
}

INCLUDEPATH += ../SpidrMpx3Lib

LIBS += -lSpidrMpx3Lib

FORMS     += spidrtv.ui
RESOURCES += spidrtv.qrc

SOURCES   += main.cpp
SOURCES   += SpidrMpx3Tv.cpp

HEADERS   += SpidrMpx3Tv.h

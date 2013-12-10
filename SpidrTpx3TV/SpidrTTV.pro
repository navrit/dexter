#
# Project file for the SpidrTTV tool
#
# To generate a Visual Studio project:
#   qmake -t vcapp SpidrTTV.pro
# To generate a Makefile:
#   qmake SpidrTTV.pro
#
# (or start from the 'SUBDIRS' .pro file to recursively generate the projects)
#
TEMPLATE = app
TARGET   = SpidrTTV

# Create a Qt app
QT += network
contains(QT_MAJOR_VERSION,5) {
  QT += widgets
}
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
  RC_FILE = spidrtpx3tv.rc
}

INCLUDEPATH += ../SpidrTpx3Lib

LIBS += -lSpidrTpx3Lib

FORMS     += spidrtpx3tv.ui
RESOURCES += spidrtpx3tv.qrc

SOURCES   += main.cpp
SOURCES   += SpidrTpx3Tv.cpp

HEADERS   += SpidrTpx3Tv.h

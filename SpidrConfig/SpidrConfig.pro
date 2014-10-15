#
# Project file for the SpidrConfig tool
#
# To generate a Visual Studio project:
#   qmake -t vcapp SpidrConfig.pro
# To generate a Makefile:
#   qmake SpidrConfig.pro
#
# (or start from the 'SUBDIRS' .pro file to recursively generate the projects)
#
TEMPLATE = app
TARGET   = SpidrConfig

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
  RC_FILE = spidrconfig.rc
}

INCLUDEPATH += ../SpidrTpx3Lib

LIBS += -lSpidrTpx3Lib

FORMS     += SpidrConfig.ui
RESOURCES += spidrconfig.qrc

SOURCES   += main.cpp
SOURCES   += SpidrConfig.cpp

HEADERS   += SpidrConfig.h

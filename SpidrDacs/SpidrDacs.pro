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
TARGET   = SpidrDacs

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
  RC_FILE = spidrdacs.rc
}

INCLUDEPATH += ../SpidrTpx3Lib

LIBS += -lSpidrTpx3Lib

FORMS     += SpidrDacs.ui
RESOURCES += spidrdacs.qrc

SOURCES   += main.cpp
SOURCES   += SpidrDacs.cpp

HEADERS   += SpidrDacs.h

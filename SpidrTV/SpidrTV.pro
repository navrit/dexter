#
# Project file for the SpidrTV tool
#
# To generate a Visual Studio project:
#   qmake -t vcapp SpidrTV.pro
# To generate a Makefile:
#   qmake SpidrTV.pro
#
# (or start from the 'SUBDIRS' .pro file to recursively generate the projects)
#
TEMPLATE = app
TARGET   = SpidrTV

# Create a Qt app
QT += network
CONFIG += qt thread warn_on exceptions debug_and_release

CONFIG(debug, debug|release) {
  OBJECTS_DIR = debug
  MOC_DIR     = debug
  UI_DIR      = debug
  DESTDIR     = ../Debug
  LIBS       += -L../SpidrLib/debug
}

CONFIG(release, debug|release) {
  OBJECTS_DIR = release
  MOC_DIR     = release
  UI_DIR      = release
  DESTDIR     = ../Release
  LIBS       += -L../SpidrLib/release
}

win32 {
  RC_FILE = spidrtv.rc
}

FORMS     += spidrtv.ui
RESOURCES += spidrtv.qrc

SOURCES   += main.cpp
SOURCES   += SpidrTv.cpp
SOURCES   += ReceiverThread.cpp

HEADERS   += SpidrTv.h
HEADERS   += ReceiverThread.h

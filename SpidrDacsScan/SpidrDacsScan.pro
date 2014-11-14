#
# Project file for the SpidrDacsScan tool
#
# To generate a Visual Studio project:
#   qmake -t vcapp SpidrDacsScan.pro
# To generate a Makefile:
#   qmake SpidrDacsScan.pro
#
# (or start from the 'SUBDIRS' .pro file to recursively generate the projects)
#
TEMPLATE = app
TARGET   = SpidrDacsScan

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
  RC_FILE = spidrdacsscan.rc
}

INCLUDEPATH += ../SpidrTpx3Lib
INCLUDEPATH += ../QCustomPlot

LIBS += -lSpidrTpx3Lib
LIBS += -lQCustomPlot

FORMS     += SpidrDacsScan.ui
RESOURCES += spidrdacsscan.qrc

SOURCES   += main.cpp
SOURCES   += SpidrDacsScan.cpp

HEADERS   += SpidrDacsScan.h

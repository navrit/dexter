#
# Project file for the spidrtpx3libtest testprogram
#
# To generate a Visual Studio project:
#   qmake -t vcapp spidrlibtpx3test.pro
# To generate a Makefile:
#   qmake spidrlibtpx3test.pro
#
# (or start from the 'SUBDIRS' .pro file to recursively generate the projects)
#
TEMPLATE = app
TARGET   = spidrtpx3libtest

# Create a console app
QT -= gui
QT += core
CONFIG += qt thread console warn_on exceptions debug_and_release

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

LIBS += -lSpidrTpx3Lib

INCLUDEPATH += ../SpidrTpx3Lib

SOURCES += spidrtpx3libtest.cpp

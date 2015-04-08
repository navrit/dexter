#
# Project file for the spidrtest testprogram
#
# To generate a Visual Studio project:
#   qmake -t vcapp spidrtest.pro
# To generate a Makefile:
#   qmake spidrtest.pro
#
# (or start from the 'SUBDIRS' .pro file to recursively generate the projects)
#
TEMPLATE = app
TARGET   = spidrtest

# Create a console app
QT -= gui
QT += core network
CONFIG += qt thread console warn_on exceptions debug_and_release

CONFIG(debug, debug|release) {
  OBJECTS_DIR = debug
  MOC_DIR     = debug
  UI_DIR      = debug
  DESTDIR     = ../../Debug
  LIBS       += -L../../Debug
}

CONFIG(release, debug|release) {
  OBJECTS_DIR = release
  MOC_DIR     = release
  UI_DIR      = release
  DESTDIR     = ../../Release
  LIBS       += -L../../Release
}

LIBS += -lSpidrTpx3Lib

INCLUDEPATH += ..

SOURCES += spidrtest.cpp

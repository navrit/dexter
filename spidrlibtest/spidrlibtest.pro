#
# Project file for the spidrlibtest testprogram
#
# To generate a Visual Studio project:
#   qmake -t vcapp spidrlibtest.pro
# To generate a Makefile:
#   qmake spidrlibtest.pro
#
# (or start from the 'SUBDIRS' .pro file to recursively generate the projects)
#
TEMPLATE = app
TARGET   = spidrlibtest

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

LIBS += -lSpidrLib

INCLUDEPATH += ../SpidrLib

SOURCES += spidrlibtest.cpp

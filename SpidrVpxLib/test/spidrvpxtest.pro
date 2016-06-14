#
# Project file for the spidrtest testprogram
#
# To generate a Visual Studio project:
#   qmake -t vcapp spidrvpxtest.pro
# To generate a Makefile:
#   qmake spidrvpxtest.pro
#
# (or start from the 'SUBDIRS' .pro file to recursively generate the projects)
#
TEMPLATE = app
TARGET   = spidrvpxtest

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

LIBS += -lSpidrVpxLib

INCLUDEPATH += ..

SOURCES += getaddr.cpp spidrvpxtest.cpp

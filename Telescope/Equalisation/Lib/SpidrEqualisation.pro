#
# Project file for the SpidrEqualisation library
#
# To generate a Visual Studio project:
#   qmake -t vclib SpidrEqualisation.pro
# To generate a Makefile:
#   qmake SpidrEqualisation.pro
#
# (or start from the 'SUBDIRS' .pro file to recursively generate the projects)
#
TEMPLATE = lib
TARGET   = SpidrEqualisation

# Create a shared library
QT -= gui
QT += core network
CONFIG += shared qt thread warn_on exceptions debug_and_release

DEFINES += MY_LIB_EXPORT

CONFIG(debug, debug|release) {
  OBJECTS_DIR = debug
  MOC_DIR     = debug
  UI_DIR      = debug
  DESTDIR     = ../../../Debug
}

CONFIG(release, debug|release) {
  OBJECTS_DIR = release
  MOC_DIR     = release
  UI_DIR      = release
  DESTDIR     = ../../../Release
}

win32 {
  LIBS += -lWs2_32
}

QMAKE_CXXFLAGS += `root-config --cflags` 
#QMAKE_CXXFLAGS += -pthread -Wno-deprecated-declarations -m64 -I/home/mateus/root/include
#QMAKE_CXXFLAGS += -pthread -Wno-deprecated-declarations -m64 -I/home/timepix3/root/include
INCLUDEPATH += ../../../SpidrTpx3Lib

SOURCES += SpidrEqualisation.cpp
HEADERS += SpidrEqualisation.h

#
# Project file for the SpidrLib library
#
# To generate a Visual Studio project:
#   qmake -t vclib SpidrLib.pro
# To generate a Makefile:
#   qmake SpidrLib.pro
#
# (or start from the 'SUBDIRS' .pro file to recursively generate the projects)
#
TEMPLATE = lib
TARGET   = SpidrLib

# Create a shared library
QT -= gui
QT += core network
CONFIG += shared qt thread warn_on exceptions debug_and_release

DEFINES += MY_LIB_EXPORT

CONFIG(debug, debug|release) {
  OBJECTS_DIR = debug
  MOC_DIR     = debug
  UI_DIR      = debug
  DESTDIR     = ../Debug
}

CONFIG(release, debug|release) {
  OBJECTS_DIR = release
  MOC_DIR     = release
  UI_DIR      = release
  DESTDIR     = ../Release
}

win32 {
  LIBS += -lWs2_32
}
unix {
  DEFINES -= WIN32
}

SOURCES += SpidrController.cpp
SOURCES += SpidrDaq.cpp
SOURCES += ReceiverThread.cpp
SOURCES += FramebuilderThread.cpp

HEADERS += SpidrController.h
HEADERS += dacsdefs.h
HEADERS += dacsdescr.h
HEADERS += mpx3conf.h
HEADERS += spidrmpx3cmds.h
HEADERS += spidrdata.h
HEADERS += SpidrDaq.h
HEADERS += ReceiverThread.h
HEADERS += FramebuilderThread.h

#
# Project file for the SpidrMpx3Lib library
#
# To generate a Visual Studio project:
#   qmake -t vclib SpidrMpx3Lib.pro
# To generate a Makefile:
#   qmake SpidrMpx3Lib.pro
#
# (or start from the 'SUBDIRS' .pro file to recursively generate the projects)
#
TEMPLATE = lib
TARGET   = SpidrMpx3Lib

# Create a shared library
QT -= gui
QT += core network
contains(QT_MAJOR_VERSION,5) {
  QT += concurrent
}
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
SOURCES += ReceiverThreadC.cpp
SOURCES += FramebuilderThread.cpp
SOURCES += FramebuilderThreadC.cpp

HEADERS += SpidrController.h
HEADERS += mpx3dacsdescr.h
HEADERS += mpx3defs.h
HEADERS += spidrmpx3cmds.h
HEADERS += spidrdata.h
HEADERS += SpidrDaq.h
HEADERS += ReceiverThread.h
HEADERS += ReceiverThreadC.h
HEADERS += FramebuilderThread.h
HEADERS += FramebuilderThreadC.h

CONFIG   += static

#
# Project file for the SpidrVpxLib library
#
# To generate a Visual Studio project:
#   qmake -t vclib SpidrVpxLib.pro
# To generate a Makefile:
#   qmake SpidrVpxLib.pro
#
# (or start from the 'SUBDIRS' .pro file to recursively generate the projects)
#
TEMPLATE = lib
TARGET   = SpidrVpxLib

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
  QMAKE_TARGET_PRODUCT = "SpidrVpxLib (debug mode)"
  QMAKE_TARGET_DESCRIPTION = "API for control and readout of SPIDR-VPX (debug mode)"
}

CONFIG(release, debug|release) {
  OBJECTS_DIR = release
  MOC_DIR     = release
  UI_DIR      = release
  DESTDIR     = ../Release
  QMAKE_TARGET_PRODUCT = "SpidrVp3Lib"
  QMAKE_TARGET_DESCRIPTION = "API for control and readout of SPIDR-VPX"
}
QMAKE_TARGET_COMPANY = "NIKHEF"
QMAKE_TARGET_COPYRIGHT = "Copyright (C) by NIKHEF"

win32 {
  LIBS += -lWs2_32
}

SOURCES += SpidrController.cpp
#SOURCES += SpidrDaq.cpp
#SOURCES += ReceiverThread.cpp
#SOURCES += DatasamplerThread.cpp

HEADERS += SpidrController.h
#HEADERS += tpx3defs.h
#HEADERS += tpx3dacsdescr.h
HEADERS += spidrvpxcmds.h
#HEADERS += spidrvpxdata.h
#HEADERS += SpidrDaq.h
#HEADERS += ReceiverThread.h
#HEADERS += DatasamplerThread.h

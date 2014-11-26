#-------------------------------------------------
#
# Project created by QtCreator 2014-10-30T16:31:35
#
#-------------------------------------------------

TEMPLATE = app
TARGET = SpidrMpx3Eq

# QT       += core gui network
# Create a shared library
greaterThan(QT_MAJOR_VERSION, 4) {
	QT += widgets printsupport
} else {
	QT += core gui
}
QT += network

CONFIG   += debug

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

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

INCLUDEPATH += ../SpidrMpx3Lib
INCLUDEPATH += ../QCustomPlot

LIBS += -lSpidrMpx3Lib
LIBS += -lQCustomPlot

SOURCES += main.cpp
SOURCES += spidrmpx3eq.cpp
SOURCES += barchart.cpp
SOURCES += ThlScan.cpp
SOURCES += DACs.cpp

HEADERS += spidrmpx3eq.h
HEADERS += barchart.h
HEADERS += ThlScan.h
HEADERS += DACs.h

FORMS    += spidrmpx3eq.ui

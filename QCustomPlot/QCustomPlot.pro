#
# Project file for the QCustomPlot library
#
# To generate a Visual Studio project:
#   qmake -t vclib QCustomPlot.pro
# To generate a Makefile:
#   qmake QCustomPlot.pro
#
# (or start from the 'SUBDIRS' .pro file to recursively generate the projects)
#
TEMPLATE = lib
TARGET   = QCustomPlot

# Create a shared library
greaterThan(QT_MAJOR_VERSION, 4) {
	QT += widgets printsupport
} else {
	QT += core gui
}

CONFIG += static qt thread warn_on exceptions debug_and_release debug

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

SOURCES += qcustomplot.cpp
HEADERS += qcustomplot.h

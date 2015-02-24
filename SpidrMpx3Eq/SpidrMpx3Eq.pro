#-------------------------------------------------
#
# Project created by QtCreator 2014-10-30T16:31:35
#
#-------------------------------------------------

TEMPLATE = app
TARGET = Mpx3GUI

# QT       += core gui network
# Use as shared library
DEFINES += QCUSTOMPLOT_USE_LIBRARY
greaterThan(QT_MAJOR_VERSION, 4) {
	QT += widgets printsupport
} else {
	QT += core gui
}
QT += network opengl

CONFIG   += debug c++11

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

SOURCES += main.cpp \
    qcstmplotheatmap.cpp \
    histogram.cpp \
    qcstmplothistogram.cpp \
    qglheatmap.cpp
SOURCES += mpx3gui.cpp
SOURCES += barchart.cpp
SOURCES += ThlScan.cpp
SOURCES += DACs.cpp

HEADERS += mpx3gui.h \
    qcstmplotheatmap.h \
    histogram.h \
    qcstmplothistogram.h \
    qglheatmap.h
HEADERS += barchart.h
HEADERS += ThlScan.h
HEADERS += DACs.h

FORMS    += mpx3gui.ui

DISTFILES += \
    shaders/texturedquad.vert \
    shaders/grayscale.frag \
    shaders/heatmaps/redblue.frag \
    shaders/test.frag \
    shaders/passthrough.vert

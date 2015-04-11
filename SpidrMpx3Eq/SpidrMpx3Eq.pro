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
    qcstmequalization.cpp \
    qcstmvisualization.cpp \
    qcstmdacs.cpp \
    gradientwidget.cpp \
    qcstmglvisualization.cpp \
    qcstmglplot.cpp \
    qcstmvoxeltab.cpp \
    gradient.cpp \
    dataset.cpp \
    qcstmthreshold.cpp \
    voxelwidget.cpp
SOURCES += mpx3gui.cpp
SOURCES += barchart.cpp
SOURCES += ThlScan.cpp
SOURCES += DACs.cpp
SOURCES += mpx3equalization.cpp

HEADERS += mpx3gui.h \
    qcstmplotheatmap.h \
    histogram.h \
    qcstmplothistogram.h \ 
    qcstmequalization.h \
    qcstmvisualization.h \
    qcstmdacs.h \
    gradientwidget.h \
    qcstmglvisualization.h \
    qcstmglplot.h \
    qcstmvoxeltab.h \
    gradient.h \
    mpx3eq_common.h \
    dataset.h \
    qcstmthreshold.h \
    voxelwidget.h
HEADERS += barchart.h
HEADERS += ThlScan.h
HEADERS += DACs.h
HEADERS += mpx3equalization.h

FORMS    += mpx3gui.ui \
    qcstmequalization.ui \
    qcstmvisualization.ui \
    qcstmdacs.ui \
    gradientwidget.ui \
    qcstmglvisualization.ui \
    qcstmthreshold.ui \
    qcstmvoxeltab.ui

DISTFILES += \
    CHANGES.txt \
    shaders/heatmap.frag \
    shaders/passthrough.vert \
    config/heatmaps.json \
    shaders/simple3d.vert \
    shaders/simple3d.frag \
    NOTES.txt


    copydata.commands += $(COPY_DIR)  \"$$PWD/config\" \"$$DESTDIR/config\" &
    copydata.commands += $(COPY_DIR)  \"$$PWD/shaders\" \"$$DESTDIR/shaders\" &
    first.depends = $(first) copydata
    export(first.depends)
    export(copydata.commands)
    QMAKE_EXTRA_TARGETS += first copydata

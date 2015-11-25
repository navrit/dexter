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
QT += network opengl multimedia multimediawidgets
CONFIG   +=  c++11 debug_and_release

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
win32 {
  INCLUDEPATH += "C:/Program Files/Phidgets"
  LIBS        += "-LC:/Program Files/Phidgets"
}

LIBS += -lSpidrMpx3Lib
LIBS += -lQCustomPlot
LIBS += -lphidget21

SOURCES += main.cpp \
    qcstmplotheatmap.cpp \
    histogram.cpp \
    qcstmplothistogram.cpp \ 
    qcstmequalization.cpp \
    qcstmdacs.cpp \
    gradientwidget.cpp \
    qcstmglvisualization.cpp \
    qcstmglplot.cpp \
    qcstmvoxeltab.cpp \
    gradient.cpp \
    dataset.cpp \
    qcstmthreshold.cpp \
    voxelwidget.cpp \
    mpx3config.cpp \
    qcstmconfigmonitoring.cpp \
    qcstmruler.cpp \
    heatmapdisplay.cpp \
    qcstmct.cpp \
    color2drecoguided.cpp
SOURCES += mpx3gui.cpp
SOURCES += barchart.cpp
SOURCES += ThlScan.cpp
SOURCES += DataTakingThread.cpp
SOURCES += qcstmBHWindow.cpp
SOURCES += qcstmBHdialog.cpp
SOURCES += StepperMotorController.cpp

HEADERS += mpx3gui.h \
    qcstmplotheatmap.h \
    histogram.h \
    qcstmplothistogram.h \ 
    qcstmequalization.h \
    qcstmdacs.h \
    gradientwidget.h \
    qcstmglvisualization.h \
    qcstmglplot.h \
    qcstmvoxeltab.h \
    gradient.h \
    mpx3eq_common.h \
    dataset.h \
    qcstmthreshold.h \
    voxelwidget.h \
    mpx3config.h \
    qcstmconfigmonitoring.h \
    qcstmruler.h \
    heatmapdisplay.h \
    qcstmct.h \
    color2drecoguided.h
HEADERS += barchart.h
HEADERS += ThlScan.h
HEADERS += DataTakingThread.h
HEADERS += qcstmBHWindow.h
HEADERS += qcstmBHdialog.h
HEADERS += StepperMotorController.h

FORMS    += mpx3gui.ui \
    qcstmequalization.ui \
    qcstmdacs.ui \
    gradientwidget.ui \
    qcstmglvisualization.ui \
    qcstmthreshold.ui \
    qcstmvoxeltab.ui \
    qcstmconfigmonitoring.ui \
    heatmapdisplay.ui \
    qcstmct.ui \
    qcstmBHWindow.ui
    qcstmBHdialog.ui

DISTFILES += \
    shaders/heatmap.frag \
    shaders/passthrough.vert \
    config/heatmaps.json \
    shaders/simple3d.vert \
    shaders/simple3d.frag \
    NOTES.txt \
    config/mpx3.json


    copydata.commands += $(COPY_DIR)  \"$$PWD/config\" \"$$DESTDIR/config\" &
    copydata.commands += $(COPY_DIR)  \"$$PWD/shaders\" \"$$DESTDIR/shaders\" &
    first.depends = $(first) copydata
    export(first.depends)
    export(copydata.commands)
    QMAKE_EXTRA_TARGETS += first copydata

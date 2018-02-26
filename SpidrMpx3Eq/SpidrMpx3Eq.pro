#-------------------------------------------------
#
# Project created by QtCreator 2014-10-30T16:31:35
#
#-------------------------------------------------

TEMPLATE = app
TARGET = Mpx3GUI

# QT       += core gui network
greaterThan(QT_MAJOR_VERSION, 4) {
        QT += widgets printsupport
} else {
        QT += core gui
}
contains(QT_MAJOR_VERSION,5) {
  QT += concurrent
}

QT += network opengl multimedia multimediawidgets
CONFIG   +=  c++14 debug_and_release

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

    QMAKE_CFLAGS_RELEASE -= -O1
    QMAKE_CFLAGS_RELEASE -= -O2
    QMAKE_CFLAGS_RELEASE *= -O3

    QMAKE_CXXFLAGS_RELEASE -= -O1
    QMAKE_CXXFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS_RELEASE *= -O3

    CONFIG += warn_off
}

DEFINES    += EXPERT_MODE

INCLUDEPATH += ../SpidrMpx3Lib

win32 {
    message(Win32)
}

unix {
    message(Unix)
    LIBS += -ltiff
}

LIBS += -lSpidrMpx3Lib
#LIBS += -lQCustomPlot

SOURCES += main.cpp \
    qcstmplotheatmap.cpp \
    histogram.cpp \
    qcstmplothistogram.cpp \
    qcstmequalization.cpp \
    qcstmdacs.cpp \
    gradientwidget.cpp \
    qcstmglvisualization.cpp \
    qcstmglplot.cpp \
    gradient.cpp \
    dataset.cpp \
    mpx3config.cpp \
    qcstmconfigmonitoring.cpp \
    qcstmruler.cpp \
    heatmapdisplay.cpp \
    qcstmcorrectionsdialog.cpp \
    dataconsumerthread.cpp \
    mpx3gui.cpp \
    barchart.cpp \
    ThlScan.cpp \
    DataTakingThread.cpp \
    qcustomplot.cpp \
    TiffFile.cpp \
    datacontrollerthread.cpp

HEADERS += mpx3gui.h \
    qcstmplotheatmap.h \
    histogram.h \
    qcstmplothistogram.h \
    qcstmequalization.h \
    qcstmdacs.h \
    gradientwidget.h \
    qcstmglvisualization.h \
    qcstmglplot.h \
    gradient.h \
    mpx3eq_common.h \
    dataset.h \
    mpx3config.h \
    qcstmconfigmonitoring.h \
    qcstmruler.h \
    heatmapdisplay.h \
    qcstmcorrectionsdialog.h \
    dataconsumerthread.h \
    barchart.h  \
    ThlScan.h \
    DataTakingThread.h \
    qcustomplot.h \
    TiffFile.h \
    datacontrollerthread.h

FORMS    += mpx3gui.ui \
    qcstmequalization.ui \
    qcstmdacs.ui \
    gradientwidget.ui \
    qcstmglvisualization.ui \
    qcstmconfigmonitoring.ui \
    heatmapdisplay.ui \
    qcstmcorrectionsdialog.ui

    copydata.commands += $(COPY_DIR)  \"$$PWD/config\" \"$$DESTDIR/\" &
    first.depends = $(first) copydata
    export(first.depends)
    export(copydata.commands)
    QMAKE_EXTRA_TARGETS += first copydata

RESOURCES += \
    icons.qrc \
    shaders.qrc

# Remove if not compiling using a static Qt build, if you don't know what this means, then you need to comment the next line
CONFIG   += static


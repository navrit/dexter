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
QT += network opengl multimedia multimediawidgets concurrent
CONFIG   +=  c++14 debug_and_release

# Testing visualization:
QT += datavisualization
#greaterThan(QT_MAJOR_VERSION, 5.7) {
#    QT += datavisualization
#}

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
    INCLUDEPATH += C:/boost_1_60_0
    LIBS        += "-LC:/boost_1_60_0/stage/lib"
    INCLUDEPATH += "C:/Program Files/Phidgets/"
    LIBS        += "-LC:/Program Files/Phidgets"
    INCLUDEPATH += "C:/dlib/dlib-19.0"
}

unix {
    message(Unix)
    LIBS += -licuuc -licui18n -licudata -lopenblas -llapack -ltiff
    #LIBS += -licuuc -licui -licudata -lopenblas -llapack
}

LIBS += -lSpidrMpx3Lib
#LIBS += -lQCustomPlot
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
    gradient.cpp \
    dataset.cpp \
    qcstmthreshold.cpp \
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
    qcustomplot.cpp

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
    qcstmthreshold.h \
    mpx3config.h \
    qcstmconfigmonitoring.h \
    qcstmruler.h \
    heatmapdisplay.h \
    qcstmcorrectionsdialog.h \
    dataconsumerthread.h \
    barchart.h  \
    ThlScan.h \
    DataTakingThread.h \
    qcustomplot.h

FORMS    += mpx3gui.ui \
    qcstmequalization.ui \
    qcstmdacs.ui \
    gradientwidget.ui \
    qcstmglvisualization.ui \
    qcstmthreshold.ui \
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


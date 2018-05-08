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

INCLUDEPATH += ../SpidrMpx3Lib

win32 {
    message(Win32)
}

unix {
    message(Unix)
    LIBS += -ltiff -lzmq
}

LIBS += -lSpidrMpx3Lib  -lphidget21

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
    qcstmct.cpp \
    qcstmcorrectionsdialog.cpp \
    statsdialog.cpp \
    profiledialog.cpp \
    dataconsumerthread.cpp \
    optionsdialog.cpp \
    qcstmsteppermotor.cpp \
    thresholdscan.cpp \
    mpx3gui.cpp \
    barchart.cpp \
    ThlScan.cpp \
    DataTakingThread.cpp \
    TiffFile.cpp \
    datacontrollerthread.cpp \
    ../Qzmq/qzmqcontext.cpp \
    ../Qzmq/qzmqreprouter.cpp \
    ../Qzmq/qzmqsocket.cpp \
    ../Qzmq/qzmqvalve.cpp \
    zmqcontroller.cpp \
    qcstmBHWindow.cpp \
    qcstmBHdialog.cpp \
    StepperMotorController.cpp \
    qcustomplot.cpp \
    testpulseequalisation.cpp

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
    qcstmct.h \
    qcstmcorrectionsdialog.h \
    statsdialog.h \
    profiledialog.h \
    dataconsumerthread.h \
    optionsdialog.h \
    qcstmsteppermotor.h \
    thresholdscan.h \
    barchart.h  \
    ThlScan.h \
    DataTakingThread.h \
    TiffFile.h \
    datacontrollerthread.h \
    ../Qzmq/qzmqcontext.h \
    ../Qzmq/qzmqreprouter.h \
    ../Qzmq/qzmqreqmessage.h \
    ../Qzmq/qzmqsocket.h \
    ../Qzmq/qzmqvalve.h \
    zmqcontroller.h \
    qcstmBHWindow.h \
    qcstmBHdialog.h \
    StepperMotorController.h \
    qcustomplot.h \
    testpulseequalisation.h

FORMS    += mpx3gui.ui \
    qcstmequalization.ui \
    qcstmdacs.ui \
    gradientwidget.ui \
    qcstmglvisualization.ui \
    qcstmthreshold.ui \
    qcstmconfigmonitoring.ui \
    heatmapdisplay.ui \
    qcstmct.ui \
    qcstmBHWindow.ui \
    qcstmBHdialog.ui \
    qcstmcorrectionsdialog.ui \
    statsdialog.ui \
    profiledialog.ui \
    qcstmBHdialog.ui \
    optionsdialog.ui \
    qcstmsteppermotor.ui \
    thresholdscan.ui \
    qcstmct.ui \
    testpulseequalisation.ui

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


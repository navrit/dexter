#-------------------------------------------------
#
# Project created by QtCreator 2014-10-30T16:31:35
#
#-------------------------------------------------

TEMPLATE = app
TARGET = Dexter

QT += core gui concurrent network opengl printsupport
CONFIG   +=  c++1z debug_and_release

message("Explicitly enabling AVX2 instructions")
QMAKE_CFLAGS          *= -mavx2
QMAKE_CXXFLAGS        *= -mavx2

CONFIG(debug, debug|release) {
    OBJECTS_DIR = objects
    MOC_DIR     = moc
    UI_DIR      = ui
    DESTDIR     = ../Debug
    LIBS       += -L../Debug
}

CONFIG(release, debug|release) {
    OBJECTS_DIR = objects
    MOC_DIR     = moc
    UI_DIR      = ui
    DESTDIR     = ../Release
    LIBS       += -L../Release

    message("Enabling all optimisation flags as qmake sees fit")
    CONFIG *= optimize_full
    CONFIG += warn_off

    QMAKE_CFLAGS   *= -flto
    QMAKE_CXXFLAGS *= -flto
}

INCLUDEPATH += ../SpidrMpx3Lib/ moc

INCLUDEPATH += $$PWD/../Qzmq
include($$PWD/../Qzmq/Qzmq.pri)

win32 {
    message(Win32)
}

unix {
    message(Unix)
    LIBS += -ltiff

    message([RELEASE NOTE] Must run qmake in order to update the version/hash number. Do this before a release otherwise the version/hash numbers will be incorrect.)
    DEFINES += GIT_CURRENT_SHA1="\\\"$(shell git -C \""$$_PRO_FILE_PWD_"\" rev-parse --short HEAD)\\\""
}

LIBS += -lSpidrMpx3Lib -lphidget21 -lzmq

SOURCES += main.cpp \
    EnergyConfiguration.cpp \
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
    datacontrollerthread.cpp \
    thresholdscandelegate.cpp \
    zmqcontroller.cpp \
    qcstmBHWindow.cpp \
    qcstmBHdialog.cpp \
    StepperMotorController.cpp \
    qcustomplot.cpp \
    testpulseequalisation.cpp \
    commandhandler.cpp \
    MerlinInterface.cpp \
    tcpserver.cpp \
    tcpconnections.cpp \
    commandhandlerwrapper.cpp \
    GeneralSettings.cpp \
    EnergyCalibrator.cpp \
    RemoteThresholdDlg.cpp \
    tcpconnection.cpp \
    canvas.cpp \
    hdmiConfig.cpp

HEADERS += mpx3gui.h \
    EnergyConfiguration.h \
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
    datacontrollerthread.h \
    thresholdscandelegate.h \
    zmqcontroller.h \
    qcstmBHWindow.h \
    qcstmBHdialog.h \
    StepperMotorController.h \
    qcustomplot.h \
    testpulseequalisation.h \
    commandhandler.h \
    handlerfunctions.h \
    MerlinInterface.h \
    merlinCommandsDef.h \
    tcpserver.h \
    tcpconnections.h \
    commandhandlerwrapper.h \
    GeneralSettings.h \
    EnergyCalibrator.h \
    RemoteThresholdDlg.h \
    ServerStatus.h \
    tcpconnection.h \
    canvas.h \
    hdmiConfig.h

FORMS    += mpx3gui.ui \
    EnergyConfiguration.ui \
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
    testpulseequalisation.ui \
    RemoteThresholdDlg.ui \
    hdmiConfig.ui

    copydata.commands += $(COPY_DIR)  \"$$PWD/config\" \"$$DESTDIR/\" &
    first.depends = $(first) copydata
    export(first.depends)
    export(copydata.commands)
    QMAKE_EXTRA_TARGETS += first copydata

RESOURCES += \
    icons.qrc \
    shaders.qrc \
    stylesheet.qss \
    icons/HelveticaNeue.ttf

# Remove if not compiling using a static Qt build, if you don't know what this means, then you need to comment the next line
CONFIG   += static

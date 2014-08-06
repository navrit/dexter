#-------------------------------------------------
#
# Project created by QtCreator 2014-01-31T09:35:40
#
#-------------------------------------------------


QT       += core
CONFIG += console
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport
TEMPLATE = app

TARGET = FQM

SOURCES += main.cpp\
        mainwindow.cpp\
        qcustomplot.cpp

HEADERS  += mainwindow.h\
            qcustomplot.h

FORMS    += mainwindow.ui
CONFIG   -= app_bundle
CONFIG   += debug_and_release

QMAKE_CXXFLAGS += -std=c++0x
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7

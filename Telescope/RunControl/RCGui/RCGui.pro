#-------------------------------------------------
#
# Project created by QtCreator 2014-10-06T15:16:10
#
#-------------------------------------------------

QT       += core
CONFIG += console
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport
TEMPLATE = app
QMAKE_CXXFLAGS += -std=c++0x


TARGET = RCGui
OBJECTS_DIR = tmp

SOURCES += main.cpp\
           RCMainWindow.cpp\
           ../../DQM/GUI/qcustomplot.cpp

HEADERS  += RCMainWindow.h\
            ../../DQM/GUI/qcustomplot.h

FORMS    += RCMainWindow.ui
CONFIG   -= app_bundle

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7

#-------------------------------------------------
#
# Project created by QtCreator 2013-12-10T23:38:01
#
#-------------------------------------------------
#cache()
QT       += core
CONFIG += console
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport
TEMPLATE = app

TARGET = PreQM


#QMAKE_CFLAGS += -mmacosx-version-min=10.6
#QMAKE_CXXFLAGS += -mmacosx-version-min=10.6
OBJECTS_DIR = tmp

SOURCES += main.cpp\
           qcustomplot.cpp\
           ../src/data_holders/Ctel_chunk.cpp\
           ../src/data_holders/Cchip.cpp\
           ../src/data_holders/Ccluster.cpp\
           ../src/data_holders/Ctrack.cpp\
           ../src/data_holders/Ctrack_volume.cpp\
           ../src/data_holders/Cpix_hit.cpp\
           ../src/Chandy.cpp\
           ../src/Cold_tel_getter.cpp\
           ../src/CPS_tel_getter.cpp\
           ../src/Ccorrelation_plots.cpp\
           ../src/Ccorrel_line_finder.cpp\
           ../src/Ccorrel_aligner.cpp\
           ../src/Ccluster_maker.cpp\
           ../src/Ccluster_plots.cpp\
           ../src/Ctrack_maker.cpp\
           ../src/Cpixel_plots.cpp\
           ../src/Cchip_plots.cpp\
           ../src/CCOG_fitter.cpp\
           ../src/Ctrack_plots.cpp\
           ../src/Cpoor_mans_aligner.cpp\
           ../src/CDQM.cpp\
           ../CDQM_options.cpp\
           hist_widget.cpp \
           twoDhist_widget.cpp \
           DQM_widg.cpp



HEADERS  +=\
            qcustomplot.h\
            ../headers/data_holders/Ctel_chunk.h\
            ../headers/data_holders/Cchip.h\
            ../headers/data_holders/Ccluster.h\
            ../headers/data_holders/Ctrack.h\
            ../headers/data_holders/Ctrack_volume.h\
            ../headers/data_holders/Cpix_hit.h\
            ../headers/Cold_tel_getter.h\
            ../headers/CPS_tel_getter.h\
            ../headers/Ctrack_maker.h\
            ../headers/Ccluster_maker.h\
            ../headers/Cchip_plots.h\
            ../headers/CCOG_fitter.h\
            ../headers/Ccluster_plots.h\
            ../headers/Ccorrelation_plots.h\
            ../headers/Ccorrel_line_finder.h\
            ../headers/Ccorrel_aligner.h\
            ../headers/Cpixel_plots.h\
            ../headers/Ctrack_plots.h\
            ../headers/Cpoor_mans_aligner.h \
            ../headers/CDQM.h \
            ../CDQM_options.h\
            hist_widget.h \
            twoDhist_widget.h \
            DQM_widg.h

FORMS    += \
    mainwindow.ui


CONFIG   -= app_bundle

QMAKE_CXXFLAGS += -std=c++0x `root-config --cflags --libs`
QMAKE_LIBS += `root-config --libs`
QMAKE_INCPATH += `root-config --incdir`
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7

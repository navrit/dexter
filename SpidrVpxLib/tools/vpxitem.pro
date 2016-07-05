TEMPLATE = app
TARGET   = vpxitem

# Create a console app
QT -= gui
QT += core network
CONFIG += qt thread console warn_on exceptions debug_and_release

CONFIG(debug, debug|release) {
  OBJECTS_DIR = debug
  MOC_DIR     = debug
  UI_DIR      = debug
  DESTDIR     = ../../Debug
  LIBS       += -L../../Debug
}

CONFIG(release, debug|release) {
  OBJECTS_DIR = release
  MOC_DIR     = release
  UI_DIR      = release
  DESTDIR     = ../../Release
  LIBS       += -L../../Release
}

LIBS += -lSpidrVpxLib

INCLUDEPATH += ..

win32 {
  SOURCES += wingetopt.c
}
SOURCES += ../test/getaddr.cpp vpxitem.cpp

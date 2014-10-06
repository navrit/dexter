TEMPLATE = app
TARGET   = DummyGen

# Create a Qt app
QT += network
contains(QT_MAJOR_VERSION,5) {
  QT += widgets
}
CONFIG += qt thread warn_on exceptions debug_and_release

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

win32 {
  RC_FILE = dummygen.rc
}

INCLUDEPATH += ../../SpidrTpx3Lib

LIBS += -lSpidrTpx3Lib

FORMS     += DummyGen.ui
RESOURCES += dummygen.qrc

SOURCES   += main.cpp
SOURCES   += DummyGen.cpp

HEADERS   += DummyGen.h

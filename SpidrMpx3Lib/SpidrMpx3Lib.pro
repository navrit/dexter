#
# Project file for the SpidrMpx3Lib library
#
# To generate a Visual Studio project:
#   qmake -t vclib SpidrMpx3Lib.pro
# To generate a Makefile:
#   qmake SpidrMpx3Lib.pro
#
# (or start from the 'SUBDIRS' .pro file to recursively generate the projects)
#
TEMPLATE = lib
TARGET   = SpidrMpx3Lib

# Create a shared library
QT -= gui
QT += core network
contains(QT_MAJOR_VERSION,5) {
  QT += concurrent
}
CONFIG += shared qt thread warn_on exceptions debug_and_release

DEFINES += MY_LIB_EXPORT

CONFIG(debug, debug|release) {
  OBJECTS_DIR = debug
  MOC_DIR     = debug
  UI_DIR      = debug
  DESTDIR     = ../Debug
}

CONFIG(release, debug|release) {
  OBJECTS_DIR = release
  MOC_DIR     = release
  UI_DIR      = release
  DESTDIR     = ../Release

  message("Enabling all optimisation flags as qmake sees fit")
  CONFIG *= optimize_full
}

win32 {
  LIBS += -lWs2_32
}
unix {
  DEFINES -= WIN32
}

INCLUDEPATH += ../

SOURCES += SpidrController.cpp \
    ChipFrame.cpp \
    FrameSet.cpp \
    FrameSetManager.cpp \
    FrameAssembler.cpp \
    UdpReceiver.cpp
SOURCES += SpidrDaq.cpp
SOURCES += ReceiverThread.cpp
SOURCES += ReceiverThreadC.cpp
SOURCES += FramebuilderThread.cpp
SOURCES += FramebuilderThreadC.cpp

HEADERS += SpidrController.h \
    ChipFrame.h \
    FrameSet.h \
    OMR.h \
    FrameSetManager.h \
    configs.h \
    UdpReceiver.h \
    PacketContainer.h \
    FrameAssembler.h \
    ../spdlog/details/async_logger_impl.h \
    ../spdlog/details/circular_q.h \
    ../spdlog/details/console_globals.h \
    ../spdlog/details/file_helper.h \
    ../spdlog/details/fmt_helper.h \
    ../spdlog/details/log_msg.h \
    ../spdlog/details/logger_impl.h \
    ../spdlog/details/mpmc_blocking_q.h \
    ../spdlog/details/null_mutex.h \
    ../spdlog/details/os.h \
    ../spdlog/details/pattern_formatter.h \
    ../spdlog/details/periodic_worker.h \
    ../spdlog/details/registry.h \
    ../spdlog/details/thread_pool.h \
    ../spdlog/fmt/bundled/color.h \
    ../spdlog/fmt/bundled/colors.h \
    ../spdlog/fmt/bundled/core.h \
    ../spdlog/fmt/bundled/format-inl.h \
    ../spdlog/fmt/bundled/format.h \
    ../spdlog/fmt/bundled/locale.h \
    ../spdlog/fmt/bundled/ostream.h \
    ../spdlog/fmt/bundled/posix.h \
    ../spdlog/fmt/bundled/printf.h \
    ../spdlog/fmt/bundled/ranges.h \
    ../spdlog/fmt/bundled/time.h \
    ../spdlog/fmt/bin_to_hex.h \
    ../spdlog/fmt/fmt.h \
    ../spdlog/fmt/ostr.h \
    ../spdlog/sinks/android_sink.h \
    ../spdlog/sinks/ansicolor_sink.h \
    ../spdlog/sinks/base_sink.h \
    ../spdlog/sinks/basic_file_sink.h \
    ../spdlog/sinks/daily_file_sink.h \
    ../spdlog/sinks/dist_sink.h \
    ../spdlog/sinks/msvc_sink.h \
    ../spdlog/sinks/null_sink.h \
    ../spdlog/sinks/ostream_sink.h \
    ../spdlog/sinks/rotating_file_sink.h \
    ../spdlog/sinks/sink.h \
    ../spdlog/sinks/stdout_color_sinks.h \
    ../spdlog/sinks/stdout_sinks.h \
    ../spdlog/sinks/syslog_sink.h \
    ../spdlog/sinks/wincolor_sink.h \
    ../spdlog/async.h \
    ../spdlog/async_logger.h \
    ../spdlog/common.h \
    ../spdlog/formatter.h \
    ../spdlog/logger.h \
    ../spdlog/spdlog.h \
    ../spdlog/tweakme.h \
    ../spdlog/version.h
HEADERS += mpx3dacsdescr.h
HEADERS += mpx3defs.h
HEADERS += spidrmpx3cmds.h
HEADERS += spidrdata.h
HEADERS += SpidrDaq.h
HEADERS += ReceiverThread.h
HEADERS += ReceiverThreadC.h
HEADERS += FramebuilderThread.h
HEADERS += FramebuilderThreadC.h

CONFIG   += static

DISTFILES += \
    ../spdlog/README.md \
    ../spdlog/fmt/bundled/LICENSE.rst \
    ../spdlog/LICENSE

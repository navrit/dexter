#
# Qmake 'pro' file for the SPIDR-TPX3 subprojects
#
# For Windows:
# to generate a Visual Studio solution file and project files:
#   qmake -tp vc SPIDR-TPX3.pro -r
# or,
# using the Qt Visual Studio Add-In, select 'Open Qt Project File..."
#
# For Linux:
#   qmake SPIDR-TPX3.pro -r
# to generate a 32-bit version on a 64-bit machine:
#   qmake -spec linux-g++-32 SPIDR-TPX3.pro -r
#
# To compile with TLU support run
#  qmake "DEFINES += TLU" SPIDR-TPX3.pro -r
# To compile with probestation support run
#  qmake "DEFINES += CERN_PROBESTATION" SPIDR-TPX3.pro -r
#

TEMPLATE = subdirs

CONFIG += debug_and_release

# Libraries
SUBDIRS += SpidrTpx3Lib/SpidrTpx3Lib.pro
SUBDIRS += QCustomPlot/QCustomPlot.pro

# Executables
SUBDIRS += SpidrConfig/SpidrConfig.pro
SUBDIRS += SpidrDacs/SpidrDacs.pro
SUBDIRS += SpidrDacsScan/SpidrDacsScan.pro
SUBDIRS += SpidrTpx3Mon/SpidrMon.pro
SUBDIRS += SpidrTpx3TV/SpidrTTV.pro
SUBDIRS += SpidrTpx3Lib/test/spidrtest.pro
SUBDIRS += spidripconfig/spidripconfig.pro
SUBDIRS += spidripconfig/spidripmonconfig.pro
#SUBDIRS += Apps/DummyGen/DummyGen.pro
#SUBDIRS += Apps/PixConfTest/PixConfTest.pro

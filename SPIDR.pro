#
# Qmake 'pro' file for the SPIDR subprojects
#
# For Windows:
# to generate a Visual Studio solution file and project files:
#   qmake -tp vc SPIDR.pro -r
# or,
# using the Qt Visual Studio Add-In, select 'Open Qt Project File..."
#
# For Linux:
#   qmake SPIDR.pro -r
# to generate a 32-bit version on a 64-bit machine:
#   qmake -spec linux-g++-32 SPIDR.pro -r
#

TEMPLATE = subdirs

CONFIG += debug_and_release

# Libraries
SUBDIRS += SpidrMpx3Lib/SpidrMpx3Lib.pro
SUBDIRS += SpidrTpx3Lib/SpidrTpx3Lib.pro
#SUBDIRS += SpidrMpx3Pixelman/SpidrPixelman.pro

# Executables
SUBDIRS += SpidrDacs/SpidrDacs.pro
SUBDIRS += SpidrDacsScan/SpidrDacsScan.pro

SUBDIRS += SpidrMpx3Mon/SpidrMon.pro
SUBDIRS += SpidrTpx3Mon/SpidrMon.pro

SUBDIRS += spidripconfig/spidripconfig.pro
SUBDIRS += spidripconfig/spidripmonconfig.pro
SUBDIRS += spidrloglevel/spidrloglevel.pro

SUBDIRS += SpidrTpx3TV/SpidrTTV.pro
SUBDIRS += SpidrMpx3TV/SpidrMTV.pro
SUBDIRS += SpidrTV/SpidrTV.pro

SUBDIRS += SpidrMpx3Lib/spidrlibtest/spidrlibtest.pro
SUBDIRS += SpidrTpx3Lib/test/spidrtest.pro

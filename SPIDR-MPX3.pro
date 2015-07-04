#
# Qmake 'pro' file for the SPIDR-MPX3 subprojects
#
# For Windows:
# to generate a Visual Studio solution file and project files:
#   qmake -tp vc SPIDR-MPX3.pro -r
# or,
# using the Qt Visual Studio Add-In, select 'Open Qt Project File..."
#
# For Linux:
#   qmake SPIDR-MPX3.pro -r
# to generate a 32-bit version on a 64-bit machine:
#   qmake -spec linux-g++-32 SPIDR-MPX3.pro -r
#

TEMPLATE = subdirs

# Libraries
SUBDIRS += SpidrMpx3Lib/SpidrMpx3Lib.pro
SUBDIRS += QCustomPlot/QCustomPlot.pro
#SUBDIRS += SpidrMpx3Pixelman/SpidrPixelman.pro

# Executables
#SUBDIRS += SpidrMpx3TV/SpidrMTV.pro
#SUBDIRS += SpidrTV/SpidrTV.pro
#SUBDIRS += SpidrMpx3Mon/SpidrMon.pro
#SUBDIRS += spidrloglevel/spidrloglevel.pro
#SUBDIRS += SpidrMpx3Lib/spidrlibtest/spidrmpx3test.pro
SUBDIRS += SpidrMpx3Eq/SpidrMpx3Eq.pro

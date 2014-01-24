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
SUBDIRS += SpidrLib/SpidrLib.pro
SUBDIRS += SpidrPixelman/SpidrPixelman.pro

# Executables
SUBDIRS += SpidrMpx3TV/SpidrMTV.pro
SUBDIRS += SpidrTV/SpidrTV.pro
SUBDIRS += SpidrMon/SpidrMon.pro
SUBDIRS += spidrloglevel/spidrloglevel.pro
SUBDIRS += spidrlibtest/spidrlibtest.pro

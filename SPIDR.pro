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

# Libraries
SUBDIRS += SpidrLib/SpidrLib.pro

# Executables
SUBDIRS += SpidrTV/SpidrTV.pro
SUBDIRS += SpidrMon/SpidrMon.pro
SUBDIRS += spidrlibtest/spidrlibtest.pro
SUBDIRS += filereader/filereader.pro
SUBDIRS += SpidrPixelman/SpidrPixelman.pro

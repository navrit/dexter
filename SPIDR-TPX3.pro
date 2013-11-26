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

TEMPLATE = subdirs

# Libraries
SUBDIRS += SpidrTpx3Lib/SpidrTpx3Lib.pro

# Executables
SUBDIRS += SpidrDacs/SpidrDacs.pro
SUBDIRS += SpidrTpx3Mon/SpidrMon.pro
SUBDIRS += SpidrTpx3TV/SpidrTTV.pro
SUBDIRS += spidrtpx3libtest/spidrtpx3libtest.pro

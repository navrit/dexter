#
# Qmake 'pro' file for the SPIDR-TPX3-PROBESTATION subprojects
#
# For Windows:
# to generate a Visual Studio solution file and project files:
#   qmake -tp vc SPIDR-TPX3-PROBESTATION.pro -r
# or,
# using the Qt Visual Studio Add-In, select 'Open Qt Project File..."
#
# For Linux:
#   qmake SPIDR-TPX3-PROBESTATION.pro -r
# to generate a 32-bit version on a 64-bit machine:
#   qmake -spec linux-g++-32 SPIDR-TPX3-PROBESTATION.pro -r
#

TEMPLATE = subdirs


# Libraries
SUBDIRS += SpidrTpx3Lib/SpidrTpx3Lib-ProbeStation.pro


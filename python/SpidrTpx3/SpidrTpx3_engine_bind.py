#!/usr/bin/env python

from pybindgen import *
import sys

mod = Module('SpidrTpx3_engine')
mod.add_include('"../../SpidrTpx3Lib/SpidrDaq.h"')
mod.add_include('"../../SpidrTpx3Lib/SpidrController.h"')
mod.add_include('"../../SpidrTpx3Lib/tpx3defs.h"')

c2 = mod.add_class('SpidrController')
c2.add_constructor([param('int', 'ipaddr3'),param('int', 'ipaddr2'),param('int', 'ipaddr1'),param('int', 'ipaddr0'),param('int', 'port')])
c2.add_method('isConnected',           'bool',        [])
c2.add_method('connectionStateString', 'std::string', [])
c2.add_method('connectionErrString',   'std::string', [])
c2.add_method('ipAddressString',       'std::string', [])
c2.add_method('errorString',           'std::string', [])
c2.add_method('setDac',                'bool',        [param('int', 'dev_nr'),param('int', 'dac_code'),param('int', 'dac_valr')])
c2.add_method('dacMax',                'int',         [param('int', 'dac_code')])
c2.add_method('setSenseDac',           'bool',        [param('int', 'dev_nr'),param('int', 'dac_code')])
c2.add_method('getAdc',                'bool',        [param('int', 'dev_nr'),param('int*', 'adc_val', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('getRemoteTemp',         'bool',        [param('int*', 'mdegrees', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('getLocalTemp',          'bool',        [param('int*', 'mdegrees', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('getAvdd',               'bool',        [param('int*', 'mvolt', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'mamp', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'mwatt', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('getDvdd',               'bool',        [param('int*', 'mvolt', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'mamp', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'mwatt', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('setDacsDflt',           'bool',        [param('int', 'dev_nr')])
c2.add_method('resetDevice',           'bool',        [param('int', 'dev_nr')])
c2.add_method('resetDevices',           'bool',       [])
  
c2.add_method('resetPixelConfig',      None,          [])
c2.add_method('configPixel',           'bool',        [param('int', 'x'),param('int', 'y'),param('int', 'threshold'),param('int', 'testbit')])
c2.add_method('setPixelConfig',        'bool',        [param('int', 'dev_nr')])
#c2.add_method('getPixelConfig',        'bool',        [param('int', 'dev_nr'),param('int**', 'config')])

c2.add_method('getDeviceId',           'bool',        [param('int', 'dev_nr'),param('int*', 'id', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('getDac',                'bool',        [param('int', 'dev_nr'),param('int', 'dac_code'),param('int*', 'dac_val', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('getGenConfig',          'bool',        [param('int', 'dev_nr'),param('int*', 'config', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('getPllConfig',          'bool',        [param('int', 'dev_nr'),param('int*', 'config', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('getOutBlockConfig',     'bool',        [param('int', 'dev_nr'),param('int*', 'config', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('getTpPeriodPhase',      'bool',        [param('int', 'dev_nr'),param('int*', 'period', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'phase', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('getTpNumber',           'bool',        [param('int', 'dev_nr'),param('int*', 'number', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('uploadPacket',          'bool',        [param('int', 'dev_nr'),param('unsigned char*', 'packet', transfer_ownership=False,direction = Parameter.DIRECTION_IN,array_length=256),param('int', 'size')])

  
mod.add_enum('DAC_code', ['TPX3_IBIAS_PREAMP_ON','TPX3_IBIAS_PREAMP_OFF','TPX3_VPREAMP_NCAS','TPX3_IBIAS_IKRUM','TPX3_VFBK',
                          'TPX3_VTHRESH_FINE','TPX3_VTHRESH_COARSE','TPX3_IBIAS_DISCS1_ON','TPX3_IBIAS_DISCS1_OFF',
                          'TPX3_IBIAS_DISCS2_ON','TPX3_IBIAS_DISCS2_OFF','TPX3_IBIAS_PIXELDAC','TPX3_IBIAS_TPBUFIN',
                          'TPX3_IBIAS_TPBUFOUT','TPX3_VTP_COARSE','TPX3_VTP_FINE','TPX3_IBIAS_CP_PLL'])
mod.add_enum('Defines', ['ALL_PIXELS'])

c = mod.add_class('SpidrDaq')
c.add_constructor([param('int', 'ipaddr3'),param('int', 'ipaddr2'),param('int', 'ipaddr1'),param('int', 'ipaddr0'),param('int', 'port')])
c.add_constructor([param('SpidrController *', 'spidrctrl', transfer_ownership=False)])
c.add_method('classVersion',    'int',         [])
c.add_method('ipAddressString', 'std::string', [])
c.add_method('errorString',     'std::string', [])
c.add_method('openFile',        'bool',        [param('std::string', 'filename'),param('bool', 'overwrite') ] )
c.add_method('closeFile',       'bool',        [])
c.add_method('setFlush',        None,          [param('bool', 'flush')])
c.add_method('stop',            None,          [])



mod.add_include('"../SpidrTpx3/udp_server.h"')
mod.add_container('std::list<unsigned long>', 'unsigned long', 'list') # declare a container only once

c3 = mod.add_class('UDPServer')
c3.add_constructor([])
c3.add_method('start', None, [param ('unsigned int', 'port')])
c3.add_method('getN', 'std::list<unsigned long>',[param('unsigned int', 'N'),param('unsigned int', 'debug')])
c3.add_method('getH', 'std::list<unsigned long>',[param('unsigned int', 'val'),param('unsigned int', 'mask'),param('unsigned int', 'debug')])



#klass.add_method('getN', 'std::list<unsigned long>',[param('unsigned int', 'N')])
mod.generate(sys.stdout)


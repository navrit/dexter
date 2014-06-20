#!/usr/bin/env python

from pybindgen import *
import sys
from defines import load_defines
import getpass

def check_makefile_for_probestation(fname):
  f=open(fname,"r")
  for l in f.readlines():
    if l.find("PROBESTATION.pro")>=0:
      return True
  f.close()
  return False

CERN_PROBESTATION=0
    
if check_makefile_for_probestation('../Makefile'):
  CERN_PROBESTATION=1
for a in sys.argv:
  if a.strip()=="CERN_PROBESTATION":CERN_PROBESTATION=1

#if getpass.getuser()=='skulis':
#  CERN_PROBESTATION=1

mod = Module('SpidrTpx3_engine')
if CERN_PROBESTATION:
  print "#define CERN_PROBESTATION"
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

c2.add_method('getDeviceId',           'bool',        [param('int', 'dev_nr'), param('int*', 'id', transfer_ownership=True,direction = Parameter.DIRECTION_OUT)])
  
c2.add_method('classVersion',          'int',         [])

c2.add_method('getSoftwVersion',       'bool',        [param('int*', 'version', transfer_ownership=True,direction = Parameter.DIRECTION_OUT)])
c2.add_method('getFirmwVersion',       'bool',        [param('int*', 'version', transfer_ownership=True,direction = Parameter.DIRECTION_OUT)])

c2.add_method('setDac',                'bool',        [param('int', 'dev_nr'),param('int', 'dac_code'),param('int', 'dac_valr')])
c2.add_method('dacMax',                'int',         [param('int', 'dac_code')])
c2.add_method('setSenseDac',           'bool',        [param('int', 'dev_nr'),param('int', 'dac_code')])
c2.add_method('getAdc',                'bool',        [param('int', 'dev_nr'),param('int*', 'adc_val', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int', 'nr_of_samples')])

c2.add_method('getRemoteTemp',         'bool',        [param('int*', 'mdegrees', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('getLocalTemp',          'bool',        [param('int*', 'mdegrees', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('getAvdd',               'bool',        [param('int*', 'mvolt', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'mamp', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'mwatt', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('getDvdd',               'bool',        [param('int*', 'mvolt', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'mamp', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'mwatt', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])

c2.add_method('getAvddNow',            'bool',        [param('int*', 'mvolt', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'mamp', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'mwatt', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('getDvddNow',            'bool',        [param('int*', 'mvolt', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'mamp', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'mwatt', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])

c2.add_method('setDacsDflt',           'bool',        [param('int', 'dev_nr')])
c2.add_method('resetDevice',           'bool',        [param('int', 'dev_nr')])
c2.add_method('resetDevices',          'bool',        [])
c2.add_method('reinitDevice',          'bool',        [param('int', 'dev_nr')])
c2.add_method('reinitDevices',         'bool',        [])
  
c2.add_method('resetPixelConfig',      None,          [])
#c2.add_method('configPixel',           'bool',        [param('int', 'x'),param('int', 'y'),param('int', 'threshold'),param('int', 'testbit')])
c2.add_method('setPixelConfig',        'bool',        [param('int', 'dev_nr')])
#c2.add_method('getPixelConfig',        'bool',        [param('int', 'dev_nr'),param('int**', 'config')])

c2.add_method('getDeviceId',           'bool',        [param('int', 'dev_nr'),param('int*', 'id', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('getDac',                'bool',        [param('int', 'dev_nr'),param('int', 'dac_code'),param('int*', 'dac_val', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('getGenConfig',          'bool',        [param('int', 'dev_nr'),param('int*', 'config', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('setGenConfig',          'bool',        [param('int', 'dev_nr'),param('int', 'config')])
c2.add_method('getPllConfig',          'bool',        [param('int', 'dev_nr'),param('int*', 'config', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('setPllConfig',          'bool',        [param('int', 'dev_nr'),param('int', 'config')])

c2.add_method('getSlvsConfig',          'bool',        [param('int', 'dev_nr'),param('int*', 'config', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('setSlvsConfig',          'bool',        [param('int', 'dev_nr'),param('int', 'config')])

  
c2.add_method('getOutBlockConfig',     'bool',        [param('int', 'dev_nr'),param('int*', 'config', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('getTpPeriodPhase',      'bool',        [param('int', 'dev_nr'),param('int*', 'period', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'phase', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('setTpPeriodPhase',      'bool',        [param('int', 'dev_nr'),param('int', 'period'), param('int', 'phase')])
c2.add_method('getTpNumber',           'bool',        [param('int', 'dev_nr'),param('int*', 'number', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
c2.add_method('setTpNumber',           'bool',        [param('int', 'dev_nr'),param('int', 'number')])
c2.add_method('uploadPacket',          'bool',        [param('int', 'dev_nr'),param('unsigned char*', 'packet', transfer_ownership=False,direction = Parameter.DIRECTION_IN,array_length=256),param('int', 'size')])

c2.add_method('setGpioPin',            'bool',        [param('int', 'pin_nr'), param('int', 'state')] )

c2.add_method('setLogLevel',           'bool',        [param('int', 'level')])
c2.add_method('readEfuses',             'bool',        [param('int', 'dev_nr'),param('int*', 'efuses', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
if CERN_PROBESTATION:
  c2.add_method('burnEfuse',             'bool',        [param('int', 'dev_nr'),param('int', 'program_width'),param('int', 'selection')])

c2.add_method('resetTimer',            'bool',        [param('int', 'dev_nr')])
c2.add_method('getTimer',              'bool',        [param('int', 'dev_nr'),param('unsigned int*', 'timer_lo', transfer_ownership=False,direction = Parameter.DIRECTION_OUT), 
                                                                              param('unsigned int*', 'timer_hi', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])

c2.add_method('getShutterStart',       'bool',        [param('int', 'dev_nr'),param('unsigned int*', 'timer_lo', transfer_ownership=False,direction = Parameter.DIRECTION_OUT), 
                                                                              param('unsigned int*', 'timer_hi', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])

c2.add_method('getShutterEnd',       'bool',          [param('int', 'dev_nr'),param('unsigned int*', 'timer_lo', transfer_ownership=False,direction = Parameter.DIRECTION_OUT), 
                                                                              param('unsigned int*', 'timer_hi', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])

c2.add_method('getHeaderFilter',     'bool',          [param('int', 'dev_nr'),param('int*', 'eth_mask', transfer_ownership=False,direction = Parameter.DIRECTION_OUT), 
                                                                              param('int*', 'cpu_mask', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])

c2.add_method('setHeaderFilter',     'bool',          [param('int', 'dev_nr'),param('int', 'eth_mask'), 
                                                                              param('int', 'cpu_mask')])

#c2.add_method('flushFifoIn',       'bool',        [param('int', 'dev_nr')])
c2.add_method('t0Sync',            'bool',        [param('int', 'dev_nr')])

c2.add_method('resetPixelConfig',      None,        [])
#c2.add_method('configPixel',           'bool',      [param('int', 'x'),param('int', 'y'),param('int', 'threshold'), param('int', 'testbit')])
#c2.add_method('maskPixel',             'bool',      [param('int', 'x'),param('int', 'y')])
#c2.add_method('unmaskPixel',           'bool',      [param('int', 'x'),param('int', 'y')])
c2.add_method('setPixelThreshold',      'bool',      [param('int', 'x'),param('int', 'y'), param('int', 'threshold')])
c2.add_method('setPixelTestEna',        'bool',      [param('int', 'x'),param('int', 'y'), param('int', 'b')])
c2.add_method('setPixelMask',           'bool',      [param('int', 'x'),param('int', 'y'), param('int', 'b')])


c2.add_method('setPixelConfig',        'bool',      [param('int', 'dev_nr') , param('int', 'cols_per_packet') ])
c2.add_method('getPixelConfig',        'bool',      [param('int', 'dev_nr')])
c2.add_method('resetPixels',           'bool',      [param('int', 'dev_nr')])

c2.add_method('setCtprBit',            'bool',       [param('int', 'column'),param('int', 'val')])
c2.add_method('setCtprBits',           'bool',       [param('int', 'val')])
c2.add_method('setCtpr',               'bool',       [param('int', 'dev_nr')])

c2.add_method('sequentialReadout',    'bool',       [param('int', 'tokens')])
c2.add_method('datadrivenReadout',    'bool',       [])
c2.add_method('pauseReadout',         'bool',       [])
#c2.add_method('openShutter',          'bool',       [param('int', 'dev_nr'),param('int', 'len')])
c2.add_method('errorString',          'std::string',       [])

c2.add_method('setTriggerConfig',     'bool',       [param('int', 'trigger_mode'),param('int', 'trigger_period_us'),param('int', 'trigger_freq_hz'),param('int', 'nr_of_triggers')])
c2.add_method('startAutoTrigger',     'bool',       [])
c2.add_method('stopAutoTrigger',      'bool',       [])
c2.add_method('setOutputMask',        'bool',       [param('int', 'dev_nr'),param('int', 'mask')])
c2.add_method('setDecodersEna',       'bool',       [param('bool', 'enable')])

mod.add_enum('DAC_code', ['TPX3_IBIAS_PREAMP_ON','TPX3_IBIAS_PREAMP_OFF','TPX3_VPREAMP_NCAS','TPX3_IBIAS_IKRUM','TPX3_VFBK',
                          'TPX3_VTHRESH_FINE','TPX3_VTHRESH_COARSE','TPX3_IBIAS_DISCS1_ON','TPX3_IBIAS_DISCS1_OFF',
                          'TPX3_IBIAS_DISCS2_ON','TPX3_IBIAS_DISCS2_OFF','TPX3_IBIAS_PIXELDAC','TPX3_IBIAS_TPBUFIN',
                          'TPX3_IBIAS_TPBUFOUT','TPX3_VTP_COARSE','TPX3_VTP_FINE','TPX3_IBIAS_CP_PLL'])
mod.add_enum('Defines', ['ALL_PIXELS'])

defs=[]
if CERN_PROBESTATION:
  defs.append("CERN_PROBESTATION")
mod.add_enum('tpx3_defs',load_defines('../SpidrTpx3Lib/tpx3defs.h',defs=defs))
#define TPX3_PLL_BYPASSED          0x0001
#define TPX3_PLL_RUN               0x0002
#define TPX3_VCNTRL_PLL            0x0004
#define TPX3_DUALEDGE_CLK          0x0008
#define TPX3_PHASESHIFT_DIV_16     0x0000
#define TPX3_PHASESHIFT_DIV_8      0x0010
#define TPX3_PHASESHIFT_DIV_4      0x0020
#define TPX3_PHASESHIFT_DIV_2      0x0030
#define TPX3_PHASESHIFT_NR_1       0x0000
#define TPX3_PHASESHIFT_NR_2       0x0040
#define TPX3_PHASESHIFT_NR_4       0x0080
#define TPX3_PHASESHIFT_NR_8       0x00C0
#define TPX3_PHASESHIFT_NR_16      0x0100
#define TPX3_PLLOUT_CONFIG_MASK    0x3E00
#define TPX3_PLLOUT_CONFIG_SHIFT   9

c = mod.add_class('SpidrDaq')
c.add_constructor([param('int', 'ipaddr3'),param('int', 'ipaddr2'),param('int', 'ipaddr1'),param('int', 'ipaddr0'),param('int', 'port')])
c.add_constructor([param('SpidrController *', 'spidrctrl', transfer_ownership=False),      param('long long', 'bufsize'),param('int','device_nr')])
c.add_method('classVersion',    'int',         [])
c.add_method('ipAddressString', 'std::string', [])
c.add_method('errorString',     'std::string', [])
#c.add_method('openFile',        'bool',        [param('std::string', 'filename'),param('bool', 'overwrite') ] )
#c.add_method('closeFile',       'bool',        [])
c.add_method('setFlush',        None,          [param('bool', 'flush')])
c.add_method('stop',            None,          [])
c.add_method('setSampling',        None,          [param('bool', 'flush')])
c.add_method('setSampleAll',        None,          [param('bool', 'flush')])
c.add_method('getSample',        'bool',          [param('int', 'max_size'),param('int', 'timeout_ms')])

c.add_method('sampleData',        'char *',       [])
c.add_method('sampleSize',        'int',          [])

c.add_method('nextPixel',        'bool',          [param('int*', 'x', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),
                                                   param('int*', 'y', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),
                                                   param('int*', 'data', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),
                                                   param('int*', 'timestamp', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])

if 1:
  c.add_custom_method_wrapper( method_name="getSample2", wrapper_name="getSample2_imp", wrapper_body="""
PyObject *
getSample2_imp(PySpidrDaq *self, PyObject *args, PyObject *kwargs, PyObject **return_exception)
{
    PyObject *py_retval;
    bool retval;
    int max_size;
    int timeout_ms;
    const char *keywords[] = {"max_size", "timeout_ms", NULL};
	
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, (char *) "ii", (char **) keywords, &max_size, &timeout_ms)) {
        return NULL;
    }
    *(return_exception)=NULL;
    Py_BEGIN_ALLOW_THREADS
    retval = self->obj->getSample(max_size, timeout_ms);
	Py_END_ALLOW_THREADS
    py_retval = Py_BuildValue((char *) "N", PyBool_FromLong(retval));
    return py_retval;
}
""")


if 0:
  c.add_custom_method_wrapper( method_name="getSampleToTNDArray", wrapper_name="getSampleToTNDArray_imp", wrapper_body="""
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include "numpy/arrayobject.h"
 PyObject * getSampleToTNDArray_imp(PySpidrDaq *self, PyObject *args, PyObject *kwargs, PyObject **return_exception)
{
    PyObject *py_retval;
    bool retval;
    int max_size;
    int timeout_ms;
    const char *keywords[] = {"max_size", "timeout_ms", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, (char *) "ii", (char **) keywords, &max_size, &timeout_ms)) {
        return NULL;
    }
//    retval = 1;
//    py_retval = Py_BuildValue((char *) "N", PyBool_FromLong(retval));

    npy_intp  l[1]={2};
    PyObject*  myarray= PyArray_ZEROS(1, l, NPY_DOUBLE, 0);
    py_retval = Py_BuildValue((char *) "O", myarray);


//    	Py_BuildValue((char *) "N", PyBool_FromLong(retval));
    return py_retval;
}""")

mod.add_include('"../SpidrTpx3/udp_server.h"')
mod.add_container('std::list<unsigned long>', 'unsigned long', 'list') # declare a container only once

c3 = mod.add_class('UDPServer')
c3.add_constructor([])
c3.add_method('start', None, [param ('unsigned int', 'port')])
c3.add_method('isStarted', 'bool', [])
c3.add_instance_attribute('err', 'int')
c3.add_method('getN', 'std::list<unsigned long>',[param('unsigned int', 'N'),param('unsigned int', 'debug')])
c3.add_method('getH', 'std::list<unsigned long>',[param('unsigned long int', 'val'),param('unsigned long int', 'mask'),param('unsigned int', 'debug')])
c3.add_method('flush', None,[])
if 1:
  c3.add_custom_method_wrapper( method_name="getH2", wrapper_name="getH2_imp", wrapper_body="""
PyObject *
getH2_imp(PyUDPServer *self, PyObject *args, PyObject *kwargs, PyObject **return_exception)
{
    PyObject *py_retval;
    std::list< unsigned long > retval;
    unsigned long int val;
    unsigned long int mask;
    unsigned int debug;
    const char *keywords[] = {"val", "mask", "debug", NULL};
    Pystd__list__lt__unsigned_long__gt__ *py_std__list__lt__unsigned_long__gt__;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, (char *) "kkI", (char **) keywords, &val, &mask, &debug)) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    retval = self->obj->getH(val, mask, debug);
    Py_END_ALLOW_THREADS
    py_std__list__lt__unsigned_long__gt__ = PyObject_New(Pystd__list__lt__unsigned_long__gt__, &Pystd__list__lt__unsigned_long__gt___Type);
    py_std__list__lt__unsigned_long__gt__->obj = new std::list<unsigned long>(retval);
    py_retval = Py_BuildValue((char *) "N", py_std__list__lt__unsigned_long__gt__);
    return py_retval;
}
""")

if 1:
  c3.add_custom_method_wrapper( method_name="getN2", wrapper_name="getN2_imp", wrapper_body="""
PyObject *
getN2_imp(PyUDPServer *self, PyObject *args, PyObject *kwargs, PyObject **return_exception)
{
    PyObject *py_retval;
    std::list< unsigned long > retval;
    unsigned int N;
    unsigned int debug;
    const char *keywords[] = {"N", "debug", NULL};
    Pystd__list__lt__unsigned_long__gt__ *py_std__list__lt__unsigned_long__gt__;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, (char *) "II", (char **) keywords, &N, &debug)) {
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    retval = self->obj->getN(N, debug);
    Py_END_ALLOW_THREADS
    py_std__list__lt__unsigned_long__gt__ = PyObject_New(Pystd__list__lt__unsigned_long__gt__, &Pystd__list__lt__unsigned_long__gt___Type);
    py_std__list__lt__unsigned_long__gt__->obj = new std::list<unsigned long>(retval);
    py_retval = Py_BuildValue((char *) "N", py_std__list__lt__unsigned_long__gt__);
    return py_retval;
}
""")


#klass.add_method('getN', 'std::list<unsigned long>',[param('unsigned int', 'N')])
mod.generate(sys.stdout)


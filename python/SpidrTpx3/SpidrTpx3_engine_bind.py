#!/usr/bin/env python

from pybindgen import *
import sys
from defines import load_defines
import getpass

def grep_makefile_for_word(fname,word):
  f=open(fname,"r")
  for l in f.readlines():
    if l.find(word)>=0:
      return True
  f.close()
  return False

DEFINE_CERN_PROBESTATION=0
#if SpidrTpx3Lib was compiled with probestation support
if grep_makefile_for_word('../Makefile','CERN_PROBESTATION'):
  DEFINE_CERN_PROBESTATION=1

DEFINE_TLU=0
#if SpidrTpx3Lib was compiled with TLU
if grep_makefile_for_word('../Makefile','TLU'):
  DEFINE_TLU=1


mod = Module('SpidrTpx3_engine')
defs=[]
if DEFINE_CERN_PROBESTATION:
  print "#define CERN_PROBESTATION"
  defs.append("CERN_PROBESTATION")
if DEFINE_TLU:
  print "#define TLU"
  defs.append("TLU")
mod.add_include('"../../SpidrTpx3Lib/SpidrDaq.h"')
mod.add_include('"../../SpidrTpx3Lib/SpidrController.h"')
mod.add_include('"../../SpidrTpx3Lib/tpx3defs.h"')
mod.add_enum('Defines', ['ALL_PIXELS'])
mod.add_enum('tpx3_defs',load_defines('../SpidrTpx3Lib/tpx3defs.h',defs=defs))



SpidrController = mod.add_class('SpidrController')
SpidrController.add_constructor([param('int', 'ipaddr3'),param('int', 'ipaddr2'),param('int', 'ipaddr1'),param('int', 'ipaddr0'),param('int', 'port')])
SpidrController.add_method('isConnected',             'bool',        [])
SpidrController.add_method('connectionStateString',   'std::string', [])
SpidrController.add_method('connectionErrString',     'std::string', [])
SpidrController.add_method('ipAddressString',         'std::string', [])
SpidrController.add_method('errorString',             'std::string', [])
SpidrController.add_method('getDeviceId',             'bool',        [param('int', 'dev_nr'), param('int*', 'id', transfer_ownership=True,direction = Parameter.DIRECTION_OUT)])
SpidrController.add_method('classVersion',            'int',         [])
SpidrController.add_method('getSoftwVersion',         'bool',        [param('int*', 'version', transfer_ownership=True,direction = Parameter.DIRECTION_OUT)])
SpidrController.add_method('getFirmwVersion',         'bool',        [param('int*', 'version', transfer_ownership=True,direction = Parameter.DIRECTION_OUT)])
SpidrController.add_method('setDac',                  'bool',        [param('int', 'dev_nr'),param('int', 'dac_code'),param('int', 'dac_valr')])
SpidrController.add_method('dacMax',                  'int',         [param('int', 'dac_code')])
SpidrController.add_method('setSenseDac',             'bool',        [param('int', 'dev_nr'),param('int', 'dac_code')])
SpidrController.add_method('getAdc',                  'bool',        [param('int*', 'adc_val', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int', 'nr_of_samples')])
SpidrController.add_method('getRemoteTemp',           'bool',        [param('int*', 'mdegrees', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
SpidrController.add_method('getLocalTemp',            'bool',        [param('int*', 'mdegrees', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
SpidrController.add_method('getAvdd',                 'bool',        [param('int*', 'mvolt', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'mamp', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'mwatt', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
SpidrController.add_method('getDvdd',                 'bool',        [param('int*', 'mvolt', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'mamp', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'mwatt', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
SpidrController.add_method('getAvddNow',              'bool',        [param('int*', 'mvolt', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'mamp', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'mwatt', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
SpidrController.add_method('getDvddNow',              'bool',        [param('int*', 'mvolt', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'mamp', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'mwatt', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
SpidrController.add_method('setDacsDflt',             'bool',        [param('int', 'dev_nr')])
SpidrController.add_method('resetDevice',             'bool',        [param('int', 'dev_nr')])
SpidrController.add_method('resetDevices',            'bool',        [])
SpidrController.add_method('reinitDevice',            'bool',        [param('int', 'dev_nr')])
SpidrController.add_method('reinitDevices',           'bool',        [])
SpidrController.add_method('resetPixelConfig',        None,          [])
SpidrController.add_method('setPixelConfig',          'bool',        [param('int', 'dev_nr')])
SpidrController.add_method('getDeviceId',             'bool',        [param('int', 'dev_nr'),param('int*', 'id', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
SpidrController.add_method('getDac',                  'bool',        [param('int', 'dev_nr'),param('int', 'dac_code'),param('int*', 'dac_val', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
SpidrController.add_method('getGenConfig',            'bool',        [param('int', 'dev_nr'),param('int*', 'config', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
SpidrController.add_method('setGenConfig',            'bool',        [param('int', 'dev_nr'),param('int', 'config')])
SpidrController.add_method('getPllConfig',            'bool',        [param('int', 'dev_nr'),param('int*', 'config', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
SpidrController.add_method('setPllConfig',            'bool',        [param('int', 'dev_nr'),param('int', 'config')])
SpidrController.add_method('getSlvsConfig',           'bool',        [param('int', 'dev_nr'),param('int*', 'config', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
SpidrController.add_method('setSlvsConfig',           'bool',        [param('int', 'dev_nr'),param('int', 'config')])
SpidrController.add_method('getOutBlockConfig',       'bool',        [param('int', 'dev_nr'),param('int*', 'config', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
SpidrController.add_method('getTpPeriodPhase',        'bool',        [param('int', 'dev_nr'),param('int*', 'period', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'phase', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
SpidrController.add_method('setTpPeriodPhase',        'bool',        [param('int', 'dev_nr'),param('int', 'period'), param('int', 'phase')])
SpidrController.add_method('getTpNumber',             'bool',        [param('int', 'dev_nr'),param('int*', 'number', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
SpidrController.add_method('setTpNumber',             'bool',        [param('int', 'dev_nr'),param('int', 'number')])
SpidrController.add_method('uploadPacket',            'bool',        [param('int', 'dev_nr'),param('unsigned char*', 'packet', transfer_ownership=False,direction = Parameter.DIRECTION_IN,array_length=256),param('int', 'size')])
SpidrController.add_method('setGpioPin',              'bool',        [param('int', 'pin_nr'), param('int', 'state')] )
SpidrController.add_method('setSpidrReg',             'bool',        [param('int', 'addr'), param('int', 'val')] )
SpidrController.add_method('getSpidrReg',             'bool',        [param('int', 'addr'), param('int*', 'val', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)] )
SpidrController.add_method('setLogLevel',             'bool',        [param('int', 'level')])
SpidrController.add_method('readEfuses',              'bool',        [param('int', 'dev_nr'),param('int*', 'efuses', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
SpidrController.add_method('resetTimer',              'bool',        [param('int', 'dev_nr')])
SpidrController.add_method('getTimer',                'bool',        [param('int', 'dev_nr'),param('unsigned int*', 'timer_lo', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),
                                                                      param('unsigned int*', 'timer_hi', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
SpidrController.add_method('getShutterStart',         'bool',        [param('int', 'dev_nr'),param('unsigned int*', 'timer_lo', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),
                                                                      param('unsigned int*', 'timer_hi', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
SpidrController.add_method('getShutterEnd',           'bool',        [param('int', 'dev_nr'),param('unsigned int*', 'timer_lo', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),
                                                                      param('unsigned int*', 'timer_hi', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
SpidrController.add_method('getHeaderFilter',         'bool',        [param('int', 'dev_nr'),param('int*', 'eth_mask', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),
                                                                      param('int*', 'cpu_mask', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
SpidrController.add_method('setHeaderFilter',         'bool',        [param('int', 'dev_nr'),param('int', 'eth_mask'),
                                                                      param('int', 'cpu_mask')])
SpidrController.add_method('t0Sync',                  'bool',        [param('int', 'dev_nr')])
SpidrController.add_method('resetPixelConfig',        None,          [])
SpidrController.add_method('setPixelThreshold',       'bool',        [param('int', 'x'),param('int', 'y'), param('int', 'threshold')])
SpidrController.add_method('setPixelTestEna',         'bool',        [param('int', 'x'),param('int', 'y'), param('int', 'b')])
SpidrController.add_method('setPixelMask',            'bool',        [param('int', 'x'),param('int', 'y'), param('int', 'b')])
SpidrController.add_method('setPixelConfig',          'bool',        [param('int', 'dev_nr') , param('int', 'cols_per_packet') ])
SpidrController.add_method('getPixelConfig',          'bool',        [param('int', 'dev_nr')])
SpidrController.add_method('resetPixels',             'bool',        [param('int', 'dev_nr')])
SpidrController.add_method('setCtprBit',              'bool',        [param('int', 'column'),param('int', 'val')])
SpidrController.add_method('setCtprBits',             'bool',        [param('int', 'val')])
SpidrController.add_method('setCtpr',                 'bool',        [param('int', 'dev_nr')])
SpidrController.add_method('sequentialReadout',       'bool',        [param('int', 'tokens')])
SpidrController.add_method('datadrivenReadout',       'bool',        [])
SpidrController.add_method('pauseReadout',            'bool',        [])
SpidrController.add_method('errorString',             'std::string', [])
SpidrController.add_method('setShutterTriggerConfig', 'bool',        [param('int', 'trigger_mode'),param('int', 'trigger_period_us'),param('int', 'trigger_freq_hz'),param('int', 'nr_of_triggers')])
SpidrController.add_method('startAutoTrigger',        'bool',        [])
SpidrController.add_method('stopAutoTrigger',         'bool',        [])
SpidrController.add_method('setOutputMask',           'bool',        [param('int', 'dev_nr'),param('int', 'mask')])
SpidrController.add_method('setDecodersEna',          'bool',        [param('bool', 'enable')])
if DEFINE_CERN_PROBESTATION:
  SpidrController.add_method('burnEfuse',             'bool',        [param('int', 'dev_nr'),param('int', 'program_width'),param('int', 'selection')])
if DEFINE_TLU:
  SpidrController.add_method('tlu_enable',            'bool',        [param('int', 'dev_nr'),param('int', 'enable')])




SpidrDaq = mod.add_class('SpidrDaq')
SpidrDaq.add_constructor([param('int', 'ipaddr3'),param('int', 'ipaddr2'),param('int', 'ipaddr1'),param('int', 'ipaddr0'),param('int', 'port')])
SpidrDaq.add_constructor([param('SpidrController *', 'spidrctrl', transfer_ownership=False),      param('long long', 'bufsize'),param('int','device_nr')])
SpidrDaq.add_method('classVersion',    'int',         [])
SpidrDaq.add_method('ipAddressString', 'std::string', [])
SpidrDaq.add_method('errorString',     'std::string', [])
SpidrDaq.add_method('setFlush',        None,          [param('bool', 'flush')])
SpidrDaq.add_method('stop',            None,          [])
SpidrDaq.add_method('setSampling',     None,          [param('bool', 'flush')])
SpidrDaq.add_method('setSampleAll',    None,          [param('bool', 'flush')])
SpidrDaq.add_method('getSample',       'bool',        [param('int', 'max_size'),param('int', 'timeout_ms')])
SpidrDaq.add_method('sampleData',      'char *',      [])
SpidrDaq.add_method('sampleSize',      'int',         [])
SpidrDaq.add_method('nextPixel',       'bool',        [param('int*', 'x', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),
                                                       param('int*', 'y', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),
                                                       param('int*', 'data', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),
                                                       param('int*', 'timestamp', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
if 1:
  SpidrDaq.add_custom_method_wrapper( method_name="getSample2", wrapper_name="getSample2_imp", wrapper_body="""
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


if 1:
  SpidrDaq.add_custom_method_wrapper( method_name="getNumpyFrames", wrapper_name="getNumpyFrames_imp", wrapper_body="""
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>



int **pyMatrix_to_TwoDimArrayInt(PyArrayObject *arrayin)  {
	int **c=NULL;
	/*
        int *a;
	int i,n,m;
	n=arrayin->dimensions[0];
	m=arrayin->dimensions[1];
	
	c=(int **)malloc((size_t) (n*sizeof(int *)));
	if (!c)   {
		printf("In **ptrvector. Allocation of memory for double array failed.");
		exit(0);  
	}

	a=(int *) arrayin->data;  // pointer to arrayin data as double 
	for ( i=0; i<n; i++)  
		c[i]=&a[i*m]; 
*/
	return c;
}

PyObject * getNumpyFrames_imp(PySpidrDaq *self, PyObject *args, PyObject *kwargs, PyObject **return_exception)
{
 /*
  PyArrayObject *py_hits, *py_tot;
  //int dims[2]={256,256};
  
  const char *keywords[] = {"tot", "hits", NULL};
 
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, (char *) "O!O!", (char **) keywords, &PyArray_Type, &py_tot, &PyArray_Type, &py_hits)) {
     return NULL;
  }
  
  int mem_size= py_hits->dimensions[0]* py_hits->dimensions[0]* sizeof(int);
  
  int **tot =pyMatrix_to_TwoDimArrayInt(py_tot);
  int **hits=pyMatrix_to_TwoDimArrayInt(py_hits);
  
  memset ( py_tot->data, 0x0,  mem_size );
  memset ( py_hits->data, 0x0,  mem_size );
  
//  std::cout << hits->dimensions[0]<<" "<< hits->dimensions[1]<<std::endl;
//  std::cout << tot->dimensions[0]<<" "<< tot->dimensions[1]<<std::endl;

  int pixcnt = 0;
  
  Py_BEGIN_ALLOW_THREADS
    
//   frame = spidrdaq.sampleData();
   int    x, y, pixdata, timestamp;
   //int size  = self->obj->sampleSize();
  
   while( self->obj->nextPixel( &x, &y, &pixdata, &timestamp ) )
   {
     tot[x][y]=1;
     hits[x][y]+=1;
     pixcnt++;
   }
   
  Py_END_ALLOW_THREADS

  PyObject *py_retval = Py_BuildValue((char *) "i", pixcnt);
*/
  PyObject *py_retval = NULL;
    
  //bool retval=1;
//  PyObject *py_retval = Py_BuildValue((char *) "N", PyBool_FromLong(retval));
  return py_retval;
    
}
""")



if 0:
  SpidrDaq.add_custom_method_wrapper( method_name="getSampleToTNDArray", wrapper_name="getSampleToTNDArray_imp", wrapper_body="""
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

mod.generate(sys.stdout,_append_to_init="import_array();\n")


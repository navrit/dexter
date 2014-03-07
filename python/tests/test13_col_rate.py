from tpx3_test import *
import os
import random
import time
import logging
from SpidrTpx3_engine import *
import numpy
from scipy.optimize import curve_fit
import matplotlib.pyplot as plt
from scipy.special import erf
import numpy as np

def errorf(x, *p):
    a, mu, sigma = p
#    print x
    return 0.5*a*(1.0+erf((x-mu)/sigma))

# Define model function to be used to fit to the data above:
def gauss(x, *p):
    A, mu, sigma = p
    return A*numpy.exp(-(x-mu)**2/(2.*sigma**2))

def mkdir(d):
  if not os.path.exists(d):
    os.makedirs(d)  
    
class test13_col_rate(tpx3_test):
  """col rate"""


  def _execute(self):
#        self.tpx.setPixelConfig()
    def gen_col_mask(cols):
      r=[0xFF]*(256/8)
      if isinstance(cols, int):
        cols=[cols]
      for col_enabled in cols:
        col=255-col_enabled
        byte=int(col/8)
        bit=7-col%8
        r[byte]&=~(1<<bit)
      return r

    if 1:
        self.tpx.resetPixels()

        config=[]

        logging.debug("Read Config Matrix")
        ReadConfigMatrix=0x90
        buf=[ReadConfigMatrix]+gen_col_mask([0,2,4])
        self.tpx.send_byte_array(buf)
        resp=self.tpx.recv_mask(0x719f000000000000,0xFFFF000000000000)
        if len(resp)>1:
          for p in resp:
            print p
        time.sleep(10)
        
        logging.debug("Read sequential")
        ReadMatrixSeq=0xA0
        buf=[ReadMatrixSeq]+[0]*28+[0x00,0x0,0x7,0x0]
        self.tpx.send_byte_array(buf)
        resp=self.tpx.recv_mask(0x71a0000000000000,0xFFFF000000000000)
                
        #self.tpx.sequentialReadout()
            

class test13_pixel_rate(tpx3_test):
  """pixel rate """
  def _execute(self):
#        self.tpx.setPixelConfig()
    def gen_col_mask(cols):
      r=[0xFF]*(256/8)
      if isinstance(cols, int):
        cols=[cols]
      for col_enabled in cols:
        col=255-col_enabled
        byte=int(col/8)
        bit=7-col%8
        r[byte]&=~(1<<bit)
      return r

    if 1:
        self.tpx.resetPixels()

        config=[]

        logging.debug("Read Config Matrix")
        ReadConfigMatrix=0x90
        buf=[ReadConfigMatrix]+gen_col_mask([0,2,4])
        self.tpx.send_byte_array(buf)
        resp=self.tpx.recv_mask(0x719f000000000000,0xFFFF000000000000)
        if len(resp)>1:
          for p in resp:
            print p
        time.sleep(10)
        
        logging.debug("Read sequential")
        ReadMatrixSeq=0xA0
        buf=[ReadMatrixSeq]+[0]*28+[0x00,0x0,0x7,0x0]
        self.tpx.send_byte_array(buf)
        resp=self.tpx.recv_mask(0x71a0000000000000,0xFFFF000000000000)
                
        #self.tpx.sequentialReadout()
            


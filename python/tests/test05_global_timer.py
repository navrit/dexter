from tpx3_test import *
import os
import random
import time
import logging
from SpidrTpx3_engine import *
import numpy
from scipy.optimize import curve_fit
import matplotlib.pyplot as plt
import pycallgraph
from scipy.special import erf
import numpy as np
from dac_defaults import dac_defaults


class test05_global_timer(tpx3_test):
  """Global timer """

  def _execute(self,**keywords):
    self.tpx.resetPixels()
    self.tpx.resetTimer()
    
    gen_config = TPX3_ACQMODE_EVT_ITOT|TPX3_TESTPULSE_ENA | TPX3_GRAYCOUNT_ENA
    self.tpx.setGenConfig(gen_config)

    eth_filter,cpu_filter=self.tpx.getHeaderFilter()
    self.tpx.setHeaderFilter(eth_filter,cpu_filter|0x2000) #accept 0xD
  
    self.tpx.send(0x4A,0,0) #Start T0_Sync_command
    
    ctpr=[]
    for col in range(256):
      rbit=random.randint(0,1)
      self.tpx.setCtprBit(col,rbit)
      ctpr.append(rbit)
    self.tpx.setCtpr()
    timer1=self.tpx.getTimer()
    self.tpx.flush_udp_fifo(val=0x7145000000000000, mask=0xFFFF000000000000)
    timer2=self.tpx.getTimer()
    self.tpx.flush_udp_fifo(val=0x7145000000000000, mask=0xFFFF000000000000)
    if timer2>timer1:
      logging.info("Timer OK")
    else:
      logging.error("Timer problem (t2 (%d) < t1 (%d)"%(timer2,timer1))


    self.tpx.send(0xD0,0,0) #read ctpr
    dcol=[0]*128
    toa={}
    data=self.tpx.recv_mask(0x71D0000000000000, 0xFFFF000000000000)
    for d in data:
      if d.type==0xD:
        if dcol[d.addr]: 
          logging.warning("Multiple packets for double column %d"%(d.addr))
        else:
          expected=ctpr[2*d.addr+1]*2+ctpr[2*d.addr]
          if expected!=d.ctpr:
            logging.warning("Wrong CTPR value for double column %d (received:%d, expected:%d)"%(d.addr, d.ctpr, expected))
          else:
            #looks fine
            dcol[d.addr]=1
            if not d.toa in toa:
              toa[d.toa]=0
            toa[d.toa]+=1
              
          
      elif d.type!=0x7:
        logging.warning("Unexpected packet %s"%str(d))
    err=0
    if sum(dcol)!=128:
      logging.warning("Received data for only %d double columns (should be 128)"%(sum(dcol)) )
      missing=""
      for d in range(128):
        if dcol[d]==0:
           missing+="%d "%d
      logging.warning("Missing double columns %s"%missing )
        
      logging.warning("Received data:")
      for d in data:
        logging.warning("  %s"%str(d) )

      data=self.tpx.recv_mask(0x71D0000000000000, 0xFFFF000000000000)
      for d in data:
        logging.warning("  >> %s"%str(d) )
      err=1
    if len(toa.keys())>2:
      logging.warning("TOA spread to high (values:%s)"%(str(toa.keys())) )
      err=1
    if not err:
      logging.info("CTPR OK")
    
      
    

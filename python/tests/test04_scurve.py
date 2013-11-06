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
    
class test04_diagonal_scurves(tpx3_test):
  """Diagonal scurves"""




  def _execute(self):

    self.tpx.setDac(6,0)
    self.tpx.setPixelConfig()
    self.tpx.setDac(6,0)
    return
    
    self.tpx.resetPixels()
    self.tpx.setDacsDflt()
    self.tpx.setDac(TPX3_IBIAS_IKRUM,15)
    self.tpx.setDac(TPX3_VTP_COARSE,50)
    self.tpx.setDac(TPX3_VTHRESH_COARSE,7) 
    self.tpx.setDac(TPX3_VFBK,143) 
    self.tpx.setDac(TPX3_IBIAS_PREAMP_ON,150)
    self.tpx.setDac(TPX3_IBIAS_DISCS1_ON,100)


        
    self.tpx.setGenConfig(0x04|0x20)
    self.tpx.setPllConfig(0x291E)
    self.tpx.setTpPeriodPhase(0xF,0)
    self.tpx.setTpNumber(250)


    mkdir(self.fname)

    #flushing udp fifo
    self.tpx.flush_udp_fifo()
    
    for x in range(256):
      self.tpx.configCtpr(x,1)
    self.tpx.setCtpr()

    self.tpx.getGenConfig()
    
    #for amp in range ..
    amps=[112,112,124,135]
    for amp in range(4):
      self.tpx.setDac(TPX3_VTP_FINE,amps[amp]) # (0e-) slope 44.5e/LSB -> (112=1000e-)  (135=2000e-)
      self.tpx.setSenseDac(TPX3_VTP_COARSE)
      coarse=self.tpx.get_adc(64)
      self.tpx.setSenseDac(TPX3_VTP_FINE)
      fine=self.tpx.get_adc(64)
      dv=1000.0*abs(fine-coarse)
      electrons=20.0*dv
      logging.info("Test pulse voltage %.4f mv (~ %.0f e-)"%(dv,electrons))

      START_TH=0
      STOP_TH=350
      res=np.zeros((256,STOP_TH-START_TH))
      if 1:
        self.tpx.resetPixelConfig()
#        self.tpx.load_equalization("logs/F3_default_eq_bruteforce/test08_equalization/eq_codes.dat")
        self.tpx.setPixelMask(ALL_PIXELS,ALL_PIXELS,1)
        on=0
        st=15000
        for x in range(256):
           self.tpx.setPixelMask(x,x, 0)
           if amp>0:
             self.tpx.setPixelTestEna(x,x, testbit=True)
        self.tpx.setPixelConfig()
        if amp==0:
          st=500
        for i in range(START_TH,STOP_TH):
          self.tpx.setDac(TPX3_VTHRESH_FINE,i)
          self.tpx.openShutter(st)
          time.sleep(0.016)
          self.tpx.sequentialReadout()
          data=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)
          logging.info("Packets received %d"%(len(data)))
          for d in data:
#            print d
            if d.type==0xA and d.col==d.row:
              res[d.col][i-START_TH]=d.event_counter
            elif d.type!=0x7:
              logging.warning("Unexpected packet %s"%str(d))

      w2f=True
      if 1: #fitting
        codes=np.array(range(START_TH,STOP_TH))
        if w2f:
          dn=self.fname+"/amp%02d/"%(amp)
          mkdir(dn)
        for col in range(256):
              if w2f:
                 fn=dn+"/%03d_%03d.dat"%(col,col)
                 f=open(fn,"w")
              vals=res[col]
              for i,code in enumerate(codes):
                 val=vals[i]
                 f.write("%d %d\n"%(code,val))
              if w2f:
                f.close()



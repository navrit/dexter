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
    self.tpx.resetPixels()
    dac_defaults(self.tpx)

    self.tpx.setGenConfig(0x04|0x20)
    self.tpx.setPllConfig(0x291E)
    self.tpx.setTpPeriodPhase(0xF,0)
    self.tpx.setTpNumber(250)

    mkdir(self.fname)

    
    self.tpx.setCtprBits(1)
    self.tpx.setCtpr()

    self.tpx.getGenConfig()
    self.tpx.flush_udp_fifo()
    
    #for amp in range ..
    amps=[0,112,124,135]
    for amp in range(4):
      dv=0
      electrons=0
      if amp>0:
        self.tpx.setDac(TPX3_VTP_FINE,amps[amp]) # (0e-) slope 44.5e/LSB -> (112=1000e-)  (135=2000e-)
        self.tpx.setSenseDac(TPX3_VTP_COARSE)
        coarse=self.tpx.get_adc(64)
        self.tpx.setSenseDac(TPX3_VTP_FINE)
        fine=self.tpx.get_adc(64)
        dv=1000.0*abs(fine-coarse)
        electrons=20.0*dv
        logging.info("Test pulse voltage %.4f mv (~ %.0f e-)"%(dv,electrons))
      else:
        logging.info("Test pulse voltage 0.0 mv (0 e-)")

      START_TH=0
      STOP_TH=350
      res=np.zeros((256,STOP_TH-START_TH))
      if 1:
        self.tpx.resetPixelConfig()
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
        self.tpx.setShutterLen(st)
        for i in range(START_TH,STOP_TH):
          self.tpx.setDac(TPX3_VTHRESH_FINE,i)
          self.tpx.openShutter()
          time.sleep(0.016)
          self.tpx.sequentialReadout()
          data=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)
          logging.debug("Packets received %d"%(len(data)))
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
          logging.info("Saving files to %s"%dn)
          mkdir(dn)
        for col in range(256):
              if w2f:
                 fn=dn+"/%03d_%03d.dat"%(col,col)
                 f=open(fn,"w")
                 f.write("# amp step %d TPX3_VTP_FINE %d\n"%(amp,amps[amp]))
                 f.write("# voltage  %.4f (measured)\n"%(dv))
                 f.write("# charge   %.1f (estimated)\n"%(electrons))
              vals=res[col]
              for i,code in enumerate(codes):
                 val=vals[i]
                 f.write("%d %d\n"%(code,val))
              if w2f:
                f.close()



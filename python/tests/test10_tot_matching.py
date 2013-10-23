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

def mkdir(d):
  if not os.path.exists(d):
    os.makedirs(d)
    
class test10_tot_matching(tpx3_test):
  """TOT matching"""


  def _execute(self):
    self.tpx.resetPixels()
    self.tpx.setDacsDflt()
    self.tpx.setDac(TPX3_IBIAS_IKRUM,15)
    self.tpx.setDac(TPX3_VTP_COARSE,50)
    self.tpx.setDac(TPX3_VTHRESH_COARSE,7) 
    self.tpx.setDac(TPX3_VFBK,143) 
    self.tpx.setDac(TPX3_IBIAS_PREAMP_ON,150)
    self.tpx.setDac(TPX3_IBIAS_DISCS1_ON,100)
    


    PULSES=16
    self.tpx.setDac(TPX3_VTHRESH_FINE,190)
    self.tpx.setGenConfig(0x20|0x4)
    self.tpx.setPllConfig(0x001E)
    
    self.tpx.setTpPeriodPhase(0xF,8)
    self.tpx.setTpNumber(PULSES)

    self.tpx.resetPixelConfig()
    self.tpx.load_equalization("logs/F3_default_eq_bruteforce/test08_equalization/eq_codes.dat")
    to_be_masked= [(23L, 69L), (49L, 94L), (50L, 94L), (89L, 46L), (90L, 46L), (105L, 139L), (106L, 139L), (107L, 138L), (112L, 5L), (115L, 196L), (116L, 196L), (135L, 122L), (136L, 122L), (175L, 7L), (176L, 7L), (205L, 27L), (207L, 51L), (247L, 218L)]
    for x,y in to_be_masked:
       self.tpx.maskPixel(x,y)
    logging.info("Pixels masked %d)"%(len(to_be_masked)))
    self.tpx.setPixelConfig()

    mkdir(self.fname)

    #flushing udp fifo
    self.tpx.flush_udp_fifo()
    self.tpx.getGenConfig()

    for amp in range(15):
      
      self.tpx.setDac(TPX3_VTP_FINE,112+(1+amp)*23) # (0e-) slope 44.5e/LSB -> (112=1000e-)  (135=2000e-)
      self.tpx.setSenseDac(TPX3_VTP_COARSE)
      coarse=self.tpx.get_adc(64)

      self.tpx.setSenseDac(TPX3_VTP_FINE)
      fine=self.tpx.get_adc(64)
      dv=1000.0*abs(fine-coarse)
      electrons=20.0*dv
      logging.info("Test pulse voltage %.4f mv (~ %.0f e-)"%(dv,electrons))

      tot=np.zeros((256,256))
      evtcnt=np.zeros((256,256))

      for seq in range(256):
        self.tpx.flushFifoIn()
        self.tpx.maskPixel(ALL_PIXELS,ALL_PIXELS)
        for x in range(256):
          for y in range(256):
            if x%16==int(seq/16) and y%16==seq%16:
              self.tpx.unmaskPixel(x,y)
              self.tpx.configPixelTpEna(x,y, testbit=True)
        self.tpx.setPixelConfig()

        for x in range(256):
          if x%16==int(seq/16):
            self.tpx.configCtpr(x,1)
          else:
            self.tpx.configCtpr(x,0)
        self.tpx.setCtpr()

        self.tpx.openShutter(1000)

        self.tpx.sequentialReadout()
        data=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)
        logging.info("Packets received %d"%(len(data)))
        for d in data:
          if d.type==0xA:
            tot[d.col][d.row]=d.itot
            evtcnt[d.col][d.row]=d.event_counter
            if d.event_counter!=PULSES:
              logging.warning("Unexpected number of pulses for pixel (%d,%d) : %d"%(d.col,d.row,d.event_counter))
          elif d.type!=0x7:
            logging.warning("Unexpected packet %s"%str(d))

      np.savetxt(self.fname+"/itot%02d.dat"%amp,tot,fmt="%d")
      np.savetxt(self.fname+"/evtcnt%02d.dat"%amp,evtcnt,fmt="%d")

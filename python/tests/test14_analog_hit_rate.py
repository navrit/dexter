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
from dac_defaults import dac_defaults


def mkdir(d):
  if not os.path.exists(d):
    os.makedirs(d)  
    
class test14_analog_hit_rate(tpx3_test):
  """Analog hit rate"""

  def _execute(self):
    mkdir(self.fname)
    self.tpx.resetPixels()
    dac_defaults(self.tpx)
    self.tpx.setDac(TPX3_VTHRESH_FINE,0)
    self.tpx.setGenConfig( TPX3_ACQMODE_EVT_ITOT | TPX3_TESTPULSE_ENA )
    self.tpx.setPllConfig(0x291E)
    self.tpx.setTpPeriodPhase(0xF,0)
    self.tpx.setTpNumber(250)
    self.tpx.setCtprBits(1)
    self.tpx.setCtpr()
    self.tpx.getGenConfig()
    self.tpx.flush_udp_fifo()

    self.tpx.setShutterLen(1000000)
    
    self.tpx.setDac(TPX3_VTP_FINE,512)

    self.tpx.resetPixelConfig()
    self.tpx.setPixelMask(ALL_PIXELS,ALL_PIXELS,1)
    for col in range(256):
      for row in range(256):
        if col%2==0 and row%4==2:
          self.tpx.setPixelMask(col,row, 0)

        if  row%4==1 or row%4==3:
          self.tpx.setPixelTestEna(col,row, testbit=True)
    self.tpx.setPixelConfig()


    self.tpx.openShutter()
    time.sleep(0.016)
    self.tpx.sequentialReadout()
    data=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)
    logging.debug("Packets received %d"%(len(data)))
    for d in data:
      logging.info(str(d))
#            print d
#            if d.type==0xA and d.col==d.row:
#              res[d.col][i-START_TH]=d.event_counter
#            elif d.type!=0x7:
#              logging.warning("Unexpected packet %s"%str(d))


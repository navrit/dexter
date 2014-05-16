from tpx3_test import *
import os
import random
import time
import logging
from SpidrTpx3_engine import *
import numpy
from scipy.optimize import curve_fit
import matplotlib.pyplot as plt
from dac_defaults import dac_defaults
import zipfile
import shutil
import sys


class test23_countrate(tpx3_test):
  """Count Rate in data driven vs sequential readout"""

  def _execute(self,**keywords):
    self.tpx.resetPixels()
    dac_defaults(self.tpx)

    self.tpx.setDac(TPX3_IBIAS_IKRUM,15)
    self.tpx.setDac(TPX3_IBIAS_DISCS1_ON,128)
    self.tpx.setDac(TPX3_IBIAS_DISCS2_ON,32)
    self.tpx.setDac(TPX3_IBIAS_PREAMP_ON,128)    
    self.tpx.setDac(TPX3_IBIAS_PIXELDAC,120)
    self.tpx.setDac(TPX3_VTHRESH_COARSE,8) 
    self.tpx.setDac(TPX3_VFBK,164) 
    self.tpx.setDac(TPX3_VTHRESH_FINE,256)

    
    self.tpx.setPllConfig( (TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK | TPX3_PHASESHIFT_DIV_8 | TPX3_PHASESHIFT_NR_16 | 0x14<<TPX3_PLLOUT_CONFIG_SHIFT) )
    
    polarity=True
    genConfig_register=TPX3_ACQMODE_TOA_TOT | TPX3_GRAYCOUNT_ENA #| TPX3_FASTLO_ENA #| TPX3_FASTLO_ENA#TPX3_ACQMODE_EVT_ITOT 
    if not polarity: genConfig_register|=TPX3_POLARITY_EMIN
    self.tpx.setGenConfig( genConfig_register)

    shutter_length=1000000
    self.logging.info("Shutter length %d us"%shutter_length)
    self.tpx.setShutterLen(shutter_length)

    self.tpx.setCtprBits(0)
    self.tpx.setCtpr()

    self.tpx.resetPixelConfig()
    self.tpx.load_equalization('calib/eq_codes_finestep9.dat',\
                      maskname='calib/eq_mask_finestep9.dat')
    self.tpx.setPixelConfig()

    self.mkdir(self.fname)

    self.tpx.datadrivenReadout()
    self.tpx.resetTimer()
    self.tpx.t0Sync()

    f=open(self.fname+'/count_rate.dat',"w")

    TH_START=1175
    TH_STOP=1250
    TH_STEP=5
    for links in (1,2,4,8):
      link_mask=int(pow(2,links)-1)
      self.tpx.setOutputMask(link_mask)

      for thr in range(TH_START,TH_STOP,TH_STEP):
        event_counter=0
        pileup=0
        self.tpx.setThreshold(thr)

        genConfig_register=TPX3_ACQMODE_TOA_TOT | TPX3_GRAYCOUNT_ENA 
        self.tpx.setGenConfig( genConfig_register)

        self.tpx.datadrivenReadout()
        self.tpx.openShutter()
        temp=self.tpx.getTpix3Temp()
        evn=np.zeros((256,256), int)
        evn_total=np.zeros((256,256), int)
        pileup_m=np.zeros((256,256), int)

        dd_packets=0
        dd_pileups=0
        finish=0
        ev_total=0
        while not finish:
          data=self.tpx.get_N_packets(1024)
          for pck in data:
            if pck.isData():
              dd_packets+=1
              dd_pileups+=pck.pileup
            else:
              if pck.isEoR():
                finish=1

        shutter_start=self.tpx.getShutterStart()
        shutter_stop=self.tpx.getShutterEnd()
        dd_shutter_time=(shutter_stop-shutter_start)*0.025
        dd_rate=float(dd_packets)/dd_shutter_time

        l="DD  %d %4d %9d %9d %11.5f  %6.2f"%(links,thr,dd_packets,dd_pileups,dd_rate,temp)
        print l
        f.write("%s\n"%(l))
        f.flush()
      f.write("\n")


    for thr in range(TH_START,TH_STOP,TH_STEP):
        self.tpx.setThreshold(thr)

        self.tpx.pauseReadout()
        self.tpx.sequentialReadout(tokens=1)
        genConfig_register=TPX3_ACQMODE_EVT_ITOT 
        self.tpx.setGenConfig( genConfig_register)
        self.tpx.openShutter()
        temp=self.tpx.getTpix3Temp()
        data=self.tpx.get_frame()
        shutter_start=self.tpx.getShutterStart()
        shutter_stop=self.tpx.getShutterEnd()
        seq_shutter_time=(shutter_stop-shutter_start)*0.025

        seq_hits=0

        for d in data:
          if d.isData():
             seq_hits+=d.event_counter

        seq_rate=float(seq_hits)/seq_shutter_time
        l="SEQ   %4d %9d %11.5f  %6.2f"%(thr,seq_hits,seq_rate,temp)
        print l
        f.write("%s\n"%(l))
        f.flush()

    f.close()


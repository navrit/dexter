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

class test22_cosmic(tpx3_test):
  """cosmic"""

  def tpix3_temp(self):
    self.tpx.setSenseDac(TPX3_BANDGAP_TEMP)
    v_bg_temp=self.tpx.get_adc(32)
    self.tpx.setSenseDac(TPX3_BANDGAP_OUTPUT)
    v_bg=self.tpx.get_adc(32)
    return 88.75-607.3*(v_bg_temp-v_bg)     #Mpix3 extracted

  def SetThreshold(self,dac_value=1000):
    i=0
    coarse_found=0
    fine_found=352
    for coarse in range(16):
       for fine in range(352,512,1):
          if dac_value==i:
             coarse_found=coarse
             fine_found=fine
          i+=1
    self.tpx.setDac(TPX3_VTHRESH_COARSE,coarse_found)
    self.tpx.setDac(TPX3_VTHRESH_FINE,fine_found)
    #print "%d %d %d \n"%(i,coarse_found,fine_found)

  def _execute(self,**keywords):
    self.tpx.resetPixels()
    dac_defaults(self.tpx)

    self.tpx.setDac(TPX3_IBIAS_IKRUM,15)
    self.tpx.setDac(TPX3_IBIAS_DISCS1_ON,128)
    self.tpx.setDac(TPX3_IBIAS_DISCS2_ON,32)
    self.tpx.setDac(TPX3_IBIAS_PREAMP_ON,128)    
    self.tpx.setDac(TPX3_IBIAS_PIXELDAC,128)
    self.tpx.setDac(TPX3_VTHRESH_COARSE,5) 
    self.tpx.setDac(TPX3_VFBK,164) 
    self.tpx.setDac(TPX3_VTHRESH_FINE,256)

    
    self.tpx.setPllConfig( (TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK | TPX3_PHASESHIFT_DIV_8 | TPX3_PHASESHIFT_NR_1 | 0x14<<TPX3_PLLOUT_CONFIG_SHIFT) )
    
    polarity=True
    genConfig_register=TPX3_ACQMODE_TOA_TOT | TPX3_GRAYCOUNT_ENA | TPX3_FASTLO_ENA #TPX3_ACQMODE_EVT_ITOT 
    if not polarity: genConfig_register|=TPX3_POLARITY_EMIN
    self.tpx.setGenConfig( genConfig_register)
    self.tpx.setCtprBits(0)
    self.tpx.setCtpr()
    def load(fn):
      f=open(fn,"r")
      ret=[]
      for l in f.readlines():
        ll=[]
        for n in l.split():
          n=int(n)
          ll.append(n)
        ret.append(ll)
      f.close()
      return ret
    self.tpx.resetPixelConfig()

    self.tpx.load_equalization('calib/eq_codes.dat',\
                      maskname='calib/eq_mask.dat')


    self.tpx.setPixelMask(142,170,1)
    self.tpx.setPixelConfig()
    self.mkdir(self.fname)
    #self.tpx.setShutterLen(shutter_length)
    #self.tpx.sequentialReadout(tokens=1)
    self.tpx.datadrivenReadout()

    self.tpx.setSenseDac(TPX3_VFBK)
    v_fbk=self.tpx.get_adc(32)
    self.tpx.resetTimer()
    self.tpx.t0Sync()

    anim=['|','/','-','\\','|','/','-','\\']
    f=open(self.fname+'/cosmic_data.dat',"w")
    f.close()
    event_counter=0
#   for thr in range(1000,1210,5):


    thr =1150
    self.tpx.setSenseDac(TPX3_VTHRESH_FINE)
    self.SetThreshold(thr)

    self.tpx.presetFPGAFilters()
    data=self.tpx.get_N_packets(1024*64)

    self.tpx.shutterOn()
    time_start = time.time()
    time_elapsed = 0.0
#   v_thr=self.tpx.get_adc(8)
    finish=False
    while not finish:
      time_now = time.time()
      time_elapsed = time_now - time_start
      if time_elapsed>10:
        self.tpx.shutterOff()
        time.sleep(0.001)
        data=self.tpx.get_frame()
        finish=True
      else:
        data=self.tpx.get_N_packets(1024)
      sys.stdout.write('%c Time: %.3f s Packet counter: %d'%(13,time_elapsed, event_counter))
#      print '\n',len(data)
      if len(data):
        for pck in data:
          if pck.type==0xB:
            line="%d\t%d\t%d\t%d\t%d\t%d"%(event_counter,pck.col,pck.row,pck.toa,pck.tot,pck.ftoa)
            sys.stdout.write("\n %s"%line)
            event_counter+=1
            f=open(self.fname+'/cosmic_data.dat',"a")
            f.write(line+"\n")
            f.close()
          else:
            print pck
        sys.stdout.write("\n")
      sys.stdout.flush()
    self.logging.info("Events colected %d"%event_counter)



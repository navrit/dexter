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
  """Take cosmic run"""
  def _execute(self,**keywords):
    self.tpx.resetPixels()
    self.tpx.setDacsDflt()
    self.tpx.setDac(TPX3_IBIAS_IKRUM,15)
    self.tpx.setDac(TPX3_VTP_COARSE,50)
    self.tpx.setDac(TPX3_VTP_FINE,112) 
    self.tpx.setDac(TPX3_IBIAS_DISCS1_ON,128)
    self.tpx.setDac(TPX3_IBIAS_DISCS2_ON,32)
    self.tpx.setDac(TPX3_IBIAS_PREAMP_ON,128)    
    self.tpx.setDac(TPX3_IBIAS_PIXELDAC,128)
    self.tpx.setDac(TPX3_VTHRESH_COARSE,5) 
    self.tpx.setDac(TPX3_VFBK,164) 
    self.tpx.setDac(TPX3_VTHRESH_FINE,256)

    self.tpx.setPllConfig( TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK \
                         | TPX3_PHASESHIFT_DIV_8 | TPX3_PHASESHIFT_NR_1 \
                         | 0x14<<TPX3_PLLOUT_CONFIG_SHIFT )

    self.tpx.setOutputMask(0x84)

    self.tpx.setGenConfig( TPX3_ACQMODE_TOA_TOT | TPX3_GRAYCOUNT_ENA | TPX3_FASTLO_ENA)

    self.tpx.setCtprBits(0)
    self.tpx.setCtpr()

    self.tpx.resetPixelConfig()
   
    self.tpx.load_equalization('calib/eq_codes_finestep9.dat',\
                      maskname='calib/eq_mask_finestep9.dat')

    self.tpx.setPixelMask(95,108,1)
    self.tpx.setPixelConfig()

    self.tpx.datadrivenReadout()

    self.tpx.resetTimer()
    self.tpx.t0Sync()
    self.tpx.setDecodersEna()
    
    self.tpx.setThreshold(1150)
    #flush any remaing data
    data=self.tpx.get_N_packets(1024*64)

    self.mkdir(self.fname)
    fout_name=self.fname+'/cosmic_data.dat'
    f=open(fout_name,"w")
    self.logging.info("Storing raw hits to %s"%fout_name)
    f.write("#seq\tpix_col\tpix_row\ttoa\ttot\tftoa\n")
    tot=np.zeros((256,256), int)
    toa=np.zeros((256,256), int)

    event_counter=0
    time_start = time.time()
    time_elapsed = 0.0
    finish=False

    self.tpx.shutterOn()

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
      if len(data):
        f.write("# %.6f s\n"%time_elapsed)
        sys.stdout.write("\n#seq\tpix_col\tpix_row\ttoa\ttot\tftoa")
        for pck in data:
          if pck.isData():
            line="%d\t%d\t%d\t%d\t%d\t%d"%(event_counter,pck.col,pck.row,pck.toa,pck.tot,pck.ftoa)
            sys.stdout.write("\n %s"%line)
            event_counter+=1
            tot[pck.col][pck.row]=pck.tot
            toa[pck.col][pck.row]=pck.toa
            f.write(line+"\n")
        f.flush()
        sys.stdout.write("\n")
      sys.stdout.flush()
    f.close()
    self.logging.info("Events colected %d"%event_counter)
    self.save_np_array(tot, fn=self.fname+'/tot.map', info="  TOT Map saved to %s")
    self.save_np_array(tot, fn=self.fname+'/toa.map', info="  TOA Map saved to %s")



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


def mkdir(d):
  if not os.path.exists(d):
    os.makedirs(d)  
    
    
from test04_scurve import sGnuplot


class test15_shutter_length(tpx3_test):
  """Shutter length"""

  def _execute(self,**keywords):
    self.tpx.resetPixels()
    self.tpx.resetPixelConfig()
    self.tpx.setPixelMask(ALL_PIXELS,ALL_PIXELS,1)
    self.tpx.setPixelConfig()

    params={}


    params['gen_config'] = TPX3_ACQMODE_EVT_ITOT|TPX3_TESTPULSE_ENA |TPX3_GRAYCOUNT_ENA 
    params['pll_config'] = TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK | TPX3_PHASESHIFT_DIV_8 | TPX3_PHASESHIFT_NR_16

    self.tpx.setGenConfig(params['gen_config'])
    self.tpx.setPllConfig(params['pll_config'])
    shutter_len=1.0
    f=open(self.fname+".dat","w")
    
    while shutter_len<1000000:
    
      self.tpx.send(0x40,0,0)#reset timer
      self.tpx.send(0x4A,0,0)#t0sync
      shutter_len_int=int(shutter_len)
      self.tpx.setShutterLen(shutter_len_int)
      self.tpx.openShutter()
#      time.sleep(shutter_len*1e-8)
      self.tpx.flush_udp_fifo(0x71AF000000000000)
      start=self.tpx.getShutterStart()
      stop=self.tpx.getShutterEnd()
      slm=stop-start

      f.write("%.9e %.9e\n"%(shutter_len_int*1e-6, slm*25e-9))
      shutter_len*=1.5
    f.close()

    g=sGnuplot(self.fname+".png")
    g("set terminal png size 800,600","set grid", "set xlabel 'preset Shutter time [s]'", "set ylabel 'Measured shutter time [s]'" , "set format y '%.0s %c'" , "set format x '%.0s %c'", "set logscale","set xr [0.9e-6:]","set yr [0.9e-6:]")
    pcmd='plot "%s.dat" w lp pt 4 ps 0.5 t ""'%self.fname
    g(pcmd)
    g.run()
    logging.info("Plot saved to %s"%g.fout)


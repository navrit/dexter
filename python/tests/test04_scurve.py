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


class sGnuplot:
    def __init__(self,fname):
        self.fout=fname
        self.fgnu=fname[:-4]+".gnu"
        self.gnu=open(self.fgnu,"w")
        self("set output '%s'"%self.fout)

    def __call__(self, *args):
        for a in args:
          self.gnu.write(a+"\n")
    def run(self):
        self.gnu.close()
        os.system("gnuplot %s"%self.fgnu)

class test04_diagonal_scurves(tpx3_test):
  """Diagonal scurves
  
Parameters:
electrons - polarity bit set to electrons 
th_start  - threshold start [LSB] (defult 0)
th_stop   - threshold stop [LSB] (defult 511)
th_step   - threshold step size [LSB] (defult 4)"""

  def _execute(self,**keywords):
    self.tpx.resetPixels()
    params={}
    params['electrons']=False
    if 'electrons' in keywords :     params['electrons']=True
    params['amps']=[0,112,124,135]
    params['shutter_len']=15000
    params['shutter_len_noise']=500
    params['th_start']=0
    params['th_stop']=511
    params['th_step']=4
    if 'th_start' in keywords : params['th_start' ]=int(keywords['th_start'])
    if 'th_stop' in keywords : params['th_stop' ]=int(keywords['th_stop'])
    if 'th_step' in keywords : params['th_step' ]=int(keywords['th_step'])

    params['gen_config'] = TPX3_ACQMODE_EVT_ITOT|TPX3_TESTPULSE_ENA
    params['pll_config'] = TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK | TPX3_PHASESHIFT_DIV_8 | TPX3_PHASESHIFT_NR_16


    dac_defaults(self.tpx)
    if params['electrons']:
       params['gen_config']|=TPX3_POLARITY_EMIN
       params['shutter_len']=params['shutter_len']
       params['shutter_len_noise']=params['shutter_len_noise']/2

    self.tpx.setGenConfig(params['gen_config'])
    self.tpx.setPllConfig(params['pll_config'])
    self.tpx.setTpPeriodPhase(0xF,0)
    self.tpx.setTpNumber(250)
    self.tpx.setCtprBits(1)
    self.tpx.setCtpr()
    self.tpx.sequentialReadout()
    self.tpx.flush_udp_fifo(0x71CF000000000000)
    self.wiki_banner(**keywords)

    self.mkdir(self.fname)
    
    for amp in range(4):
      dv=0
      electrons=0
      if amp>0:
        self.tpx.setDac(TPX3_VTP_FINE,params['amps'][amp]) # (0e-) slope 44.5e/LSB -> (112=1000e-)  (135=2000e-)
        self.tpx.setSenseDac(TPX3_VTP_COARSE)
        coarse=self.tpx.get_adc(64)
        self.tpx.setSenseDac(TPX3_VTP_FINE)
        fine=self.tpx.get_adc(64)
        dv=1000.0*abs(fine-coarse)
        electrons=20.0*dv
        logging.info("Test pulse voltage %.4f mv (~ %.0f e-)"%(dv,electrons))
      else:
        logging.info("Test pulse voltage 0.0 mv (0 e-)")

      M=(params['th_stop']+1-params['th_start'])
      res={}
      if 1:
        self.tpx.resetPixelConfig()
        self.tpx.setPixelMask(ALL_PIXELS,ALL_PIXELS,1)
        for x in range(256):
            self.tpx.setPixelMask(x,x, 0)
            if amp>0:
                self.tpx.setPixelTestEna(x,x, testbit=True)
        self.tpx.setPixelConfig()
        
        st=params['shutter_len']
        if amp==0:
            st=params['shutter_len_noise']
        self.tpx.setShutterLen(st)
        
        for threshold in range(params['th_start'],params['th_stop']+1,params['th_step']):
            self.tpx.setDac(TPX3_VTHRESH_FINE,threshold)
            self.tpx.openShutter()
            time.sleep(0.001)
#            data=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)
            data=self.tpx.recv_mask(0x71AF000000000000, 0xFFFF000000000000)

            logging.debug("Packets received %d"%(len(data)))
            
            for d in data:
                if d.type==0xA and d.col==d.row:
                    if not d.col in res:
                        res[d.col]={}
                    if not d.row in res[d.col]:
                        res[d.col][d.row]={}
                    res[d.col][d.row][threshold]=d.event_counter
                elif d.type!=0x7:
                  logging.warning("Unexpected packet %s"%str(d))
            logging.debug("Last packet %s"%(data[-1]))

      w2f=True
      if 1: #fitting
        if w2f:
          dn=self.fname+"/amp%02d/"%(amp)
          logging.info("Saving files to %s"%dn)
          mkdir(dn)
          g=sGnuplot(dn+"plot.png")
          g("set terminal png size 800,600","set grid", "set xlabel 'Threshold[LSB]'", "set ylabel 'Counts'" )#, "set key out top cent samp 0.1"
          pcmd='plot '
          for col in res:
            for row in res[col]:
              if w2f:
                 fn=dn+"/%03d_%03d.dat"%(col,row)
                 f=open(fn,"w")
                 f.write("# amp step %d TPX3_VTP_FINE %d\n"%(amp,params['amps'][amp]))
                 f.write("# voltage  %.4f (measured)\n"%(dv))
                 f.write("# charge   %.1f (estimated)\n"%(electrons))
                 for code in sorted(res[col][row]):
                    f.write("%d %d\n"%(code,res[col][row][code]))
                 f.close()
                 if len(pcmd)>5:
                   pcmd+=','
                 pcmd+="'%s' w l t '' "%(fn)
          g(pcmd)
          g.run()



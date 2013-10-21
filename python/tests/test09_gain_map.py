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
    
class test09_gain_map(tpx3_test):
  """Gain map"""


  def threshold_scan(self,res,start,stop):
      mask=0
      for i in range(start,stop):
        self.tpx.setDac(TPX3_VTHRESH_FINE,i)
        self.tpx.openShutter(15000)
        time.sleep(0.016)
        self.tpx.sequentialReadout()
        data=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)
        logging.info("Packets received %d (to be masked %d)"%(len(data),mask))
        for d in data:
          if d.type==0xA:
            res[d.col][d.row][i-start]=d.cnt
          elif d.type!=0x7:
            logging.warning("Unexpected packet %s"%str(d))
      return res


  def _execute(self):
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

#    self.tpx.equalize("logs/f3.eq")
    self.tpx.load_equalization("logs/F3_default_eq_bruteforce/test08_equalization/eq_codes.dat")

        
    mkdir(self.fname)

    #flushing udp fifo
    data=self.tpx.recv_mask(0x1234000000000000, 0xFFFF000000000000)
    
    #for amp in range ..
    for amp in range(3):
      amps=[112,124,135]
      self.tpx.setDac(TPX3_VTP_FINE,amps[amp]) # (0e-) slope 44.5e/LSB -> (112=1000e-)  (135=2000e-)
      
      self.tpx.setSenseDac(TPX3_VTP_COARSE)
      coarse=self.tpx.get_adc(64)

      self.tpx.setSenseDac(TPX3_VTP_FINE)
      fine=self.tpx.get_adc(64)
      dv=1000.0*abs(fine-coarse)
      electrons=20.0*dv
      logging.info("Test pulse voltage %.4f mv (~ %.0f e-)"%(dv,electrons))

      START_TH=0
      STOP_TH=300
      res=np.zeros((256,256,STOP_TH-START_TH))
      for seq in range(16):
        self.tpx.resetPixelConfig()
        self.tpx.load_equalization("logs/F3_default_eq_bruteforce/test08_equalization/eq_codes.dat")
        self.tpx.maskPixel(ALL_PIXELS,ALL_PIXELS)
        on=0
        for x in range(256):
          for y in range(256):
            if x%4==int(seq/4) and y%4==seq%4:
              self.tpx.unmaskPixel(x,y)
              self.tpx.configPixelTpEna(x,y, testbit=True)
        for x in range(256):
          if x%4==int(seq/4):
            self.tpx.configCtpr(x,1)
          else:
            self.tpx.configCtpr(x,0)
        self.tpx.setCtpr()

        self.tpx.setPixelConfig()
        res=self.threshold_scan(res,start=START_TH,stop=STOP_TH)

      w2f=True
      if 1: #fitting
        fit_amp=np.zeros((256,256))
        fit_rms=np.zeros((256,256))
        codes=np.array(range(START_TH,STOP_TH))
        TOP_VAL=250
        if w2f:
          dn=self.fname+"/amp%02d/"%(amp)
          mkdir(dn)
        for col in range(256):
          if w2f:mkdir(dn+"/%03d"%col)
          logging.info("Fitting col %d"%col)
          for row in range(256):
              if w2f:
                 fn=dn+"/%03d/%03d_%03d.dat"%(col,col,row)
                 f=open(fn,"w")
              vals=res[col][row]
              mean_guess=0
              mindiff=1e3
              for i,code in enumerate(codes):
                 val=vals[i]
                 f.write("%d %d\n"%(code,val))
                 if abs(val-TOP_VAL/2)<mindiff:
                    mindiff=abs(val-TOP_VAL/2)
                    mean_guess=code
                 if val>TOP_VAL:
                   vals[i]=TOP_VAL
              if vals[codes.shape[0]-1]!=TOP_VAL or np.sum(vals)<10:
                logging.warning("Problem with pixel (%d,%d)"%(col,row))

              else:
                try:
#                  hist, bin_edges = numpy.histogram(codes,bins=int(max(codes)-min(codes)+1),range=(-0.5+min(codes), 0.5+max(codes)), weights=vals)
#                  bin_centres = (bin_edges[:-1] + bin_edges[1:])/2
#                  # p0 is the initial guess for the fitting coefficients (A, mu and sigma above)
#                  p0 = [max(vals), avr, 5.]
#                  coeff, var_matrix = curve_fit(gauss, bin_centres, hist, p0=p0)
                  p0 = [TOP_VAL,mean_guess,6.0]
                  coeff, var_matrix = curve_fit(errorf, codes, vals, p0=p0)
                except:
                  coeff=[0.0,0.0,0.0]
                fit_amp[col][row] =coeff[1]
                fit_rms[col][row] =coeff[2]

              if w2f:
                f.write("# amp %.3f rms %.3f \n"%(fit_amp[col][row],fit_rms[col][row]))
                f.close()

        np.savetxt(self.fname+"/amps%02d.dat"%amp,fit_amp,fmt="%.2f")
        np.savetxt(self.fname+"/rms%02d.dat"%amp,fit_rms,fmt="%.2f")


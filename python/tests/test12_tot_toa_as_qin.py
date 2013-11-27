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


    
class test12_tot_toa_as_qin(tpx3_test):
  """TOT and TOA scan vs Qin (diagonal)"""



  def _execute(self,**keywords):
    self.tpx.resetPixels()
    self.tpx.setDacsDflt()
    self.tpx.setDac(TPX3_IBIAS_IKRUM,15)
    self.tpx.setDac(TPX3_VTP_COARSE,50)
    self.tpx.setDac(TPX3_VTHRESH_COARSE,7) 
    self.tpx.setDac(TPX3_VFBK,143) 
    self.tpx.setDac(TPX3_IBIAS_PREAMP_ON,150)
    self.tpx.setDac(TPX3_IBIAS_DISCS1_ON,100)

    self.tpx.setGenConfig(TPX3_POLARITY_HPLUS | TPX3_ACQMODE_TOA_TOT | TPX3_GRAYCOUNT_ENA | TPX3_TESTPULSE_ENA | TPX3_FASTLO_ENA)
    self.tpx.getGenConfig()
    self.tpx.setPllConfig(TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK | TPX3_PHASESHIFT_DIV_8 | TPX3_PHASESHIFT_NR_16)
    
    self.tpx.setTpPeriodPhase(0xF,0)
    self.tpx.setTpNumber(1)

    self.tpx.load_equalization("logs/F3_default_eq_bruteforce/test08_equalization/eq_codes.dat")
    mkdir(self.fname)

    #flushing udp fifo
    data=self.tpx.recv_mask(0x1234000000000000, 0xFFFF000000000000)

    self.tpx.resetPixelConfig()
    self.tpx.load_equalization("logs/F3_default_eq_bruteforce/test08_equalization/eq_codes.dat")
    self.tpx.maskPixel(ALL_PIXELS,ALL_PIXELS)
    for x in range(0,256,255):
       self.tpx.unmaskPixel(x,x)
       self.tpx.configPixelTpEna(x,x, testbit=True)

    self.tpx.setPixelConfig()
       
    for x in range(256):
       self.tpx.configCtpr(x,0)
    self.tpx.configCtpr(0,1)
    self.tpx.configCtpr(255,1)
       
    self.tpx.setCtpr()

    result={}
    self.tpx.sequentialReadout()
    amp_vols={}

    self.tpx.setDac(TPX3_VTHRESH_FINE,240)

    for amp in range(90,512,1):

      self.tpx.setDac(TPX3_VTP_FINE,amp)
      
      self.tpx.setSenseDac(TPX3_VTP_COARSE)
      coarse=self.tpx.get_adc(64)
      self.tpx.setSenseDac(TPX3_VTP_FINE)
      fine=self.tpx.get_adc(64)
      dv=1000.0*abs(fine-coarse)
      electrons=20.0*dv
      logging.info("Test pulse voltage %.4f mv (~ %.0f e-)"%(dv,electrons))
      amp_vols[amp]=dv

      tot=[]
      toa=[]
      for loop in range(64):
        self.tpx.send(0x40,0,0)#reset timer
        self.tpx.send(0x4A,0,0)#t0sync

        self.tpx.openShutter(100)
        time.sleep(0.001)
        self.tpx.sequentialReadout()

        data=self.tpx.recv_mask(0x71A0000000000000,0xFFFF000000000000)
        self.tpx.flushFifoIn()
        shutter=self.tpx.getShutterStart()
        cnt=0

        for pck in data:
#          print pck
          if pck.type==0xA :
            if not pck.col in result:
              result[pck.col]={}
            if not pck.row in result[pck.col]:
              result[pck.col][pck.row]={}
            if not amp  in result[pck.col][pck.row]:
              result[pck.col][pck.row][amp]=[]

            v=16*(pck.toa-(shutter&0x3FFF)) 
            v-=float(pck.ftoa)
            tot.append(pck.tot)
            toa.append(v)
            result[pck.col][pck.row][amp].append( (v,pck.tot) )
          elif pck.type!=0x7:
            logging.warning("Unexpected packet %s"%str(pck))

    for col in range(256):
         row=col
         print "(%d,%d)"%(col,row),
         f=open(self.fname+"/%03d_%03d.dat"%(col,row),"w")
         if col not in result:
           logging.error("No data for column %d"%col)
           continue
         if row not in result[col]:
           logging.error("No data for row %d in column %d"%(row,col))
           continue

         for amp in sorted(result[col][row]):
           toal=[]
           totl=[]
           for toa,tot in result[col][row][amp]:
             toal.append(float(toa))
             totl.append(float(tot))
           tota=numpy.array(totl)
           toaa=numpy.array(toal)
           
           f.write("%3d  %.4f %.5e %.5e %.5e %.5e "%(amp,amp_vols[amp],numpy.mean(toaa), numpy.std(toaa),numpy.mean(tota), numpy.std(tota) ) )

           f.write("cnts: %3d "%len(result[col][row][amp]) )

           f.write("toa:" )
           for toa in toal:
             f.write(" %d "%toa )

           f.write("tot:" )
           for tot in totl:
             f.write(" %d "%tot )
           f.write("\n")
            


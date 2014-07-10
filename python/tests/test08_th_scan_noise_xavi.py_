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
from equalize_x import eq_broute_force

def zipdir(fname,path):
    zip = zipfile.ZipFile(fname, 'w')
    print fname, path
    for root, dirs, files in os.walk(path):
        for file in files:
#            print file
            zip.write(os.path.join(root, file))
    zip.close()
            
# Define model function to be used to fit to the data above:
def gauss(x, *p):
    A, mu, sigma = p
    return A*numpy.exp(-(x-mu)**2/(2.*sigma**2))

    


class test08_equalization_x(tpx3_test):
  """Threshold scan over noise"""

  def threshold_scan_x(self,res=None, fromThr=0, toThr=512, threshold=10):
      peak=numpy.zeros((256,256), int)
      if res==None:
        res={}
        for x in range(256):
          res[x]={}
          for y in range(256):
            res[x][y]=0

      mask=0
      anim=['|','/','-','\\','|','/','-','\\']
      for i in range(fromThr,toThr,1):

        self.tpx.setDac(TPX3_VTHRESH_FINE,i)
        data=self.tpx.recv_mask(0x7102000000000000, 0xFFFF000000000000)

        if 0 and len(data)>1:
          for d in data:          
            logging.warning("after setdac %s"%(str(d)))

      # self.tpx.datadrivenReadout()
        self.tpx.openShutter()
        data=self.tpx.get_frame()
        
        if 1:
          print "%c %s %d/%d (packets %d)"%(13,anim[i%len(anim)],i,512,len(data)),
          if i==511:print
          sys.stdout.flush()

        
        for d in data:
#          logging.debug(str(d))
          if d.type==0xA or d.type==0xB:
            if  d.event_counter >= threshold and res[d.col][d.row]==0: 
              res[d.col][d.row]=i
              self.tpx.setPixelMask(d.col,d.row,1)
              mask+=1
            
          elif d.type!=0x7:
            logging.warning("Unexpected packet %s"%str(d))


        if mask>7500:
          logging.debug("Masking %d pixels"%mask)
#          self.tpx.flushFifoIn()
          self.tpx.pauseReadout()
          self.tpx.setPixelConfig()
          self.tpx.sequentialReadout(tokens=4)
          self.tpx.flush_udp_fifo(0x71FF000000000000)#flush until load matrix

          mask=0

      return res

  def _execute(self,**keywords):
    self.tpx.resetPixels()
    dac_defaults(self.tpx)

#    self.tpx.setDac(TPX3_IBIAS_IKRUM,128)
#    self.tpx.setDac(TPX3_IBIAS_PREAMP_ON,128)
#    self.tpx.setDac(TPX3_IBIAS_DISCS1_ON,128)
    
    self.tpx.setDac(TPX3_IBIAS_IKRUM,15)
    self.tpx.setDac(TPX3_IBIAS_DISCS1_ON,128)
    self.tpx.setDac(TPX3_IBIAS_DISCS2_ON,32)
    self.tpx.setDac(TPX3_IBIAS_PREAMP_ON,128)    
    self.tpx.setDac(TPX3_IBIAS_PIXELDAC,128)
    self.tpx.setDac(TPX3_VTHRESH_COARSE,7) 
    self.tpx.setDac(TPX3_VTHRESH_FINE,256)

    eth_filter,cpu_filter=self.tpx.getHeaderFilter()
    self.tpx.setHeaderFilter(0x0c80,cpu_filter) # 
    eth_filter,cpu_filter=self.tpx.getHeaderFilter()
    self.tpx.setGenConfig( TPX3_ACQMODE_EVT_ITOT | TPX3_GRAYCOUNT_ENA)
    self.tpx.setPllConfig( (TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK | TPX3_PHASESHIFT_DIV_8 | TPX3_PHASESHIFT_NR_16 | 0x14<<TPX3_PLLOUT_CONFIG_SHIFT) )
    self.tpx.setCtprBits(0)
    self.tpx.setCtpr()
    
    self.tpx.resetPixelConfig()
    self.tpx.setPixelMask(ALL_PIXELS,ALL_PIXELS,1)
    self.tpx.setPixelThreshold(ALL_PIXELS,ALL_PIXELS,8)
    self.tpx.pauseReadout()
    self.tpx.setPixelConfig()
    self.tpx.sequentialReadout(tokens=4)
    self.tpx.flush_udp_fifo(0x71FF000000000000)#flush until load matrix

    self.logging.info("Optimization of DC operating point")

    self.tpx.setSenseDac(TPX3_VTHRESH_COARSE)
    vthcorse=self.tpx.get_adc(1)
    time.sleep(0.001)
    vthcorse=self.tpx.get_adc(8)
    self.logging.info("  TPX3_VTHRESH_COARSE code=7 voltage=%.1f mV"%(1000.0*vthcorse))
    
    best_vfbk_val=0
    best_vfbk_code=0
    self.tpx.setSenseDac(TPX3_VFBK)
    for code in range(64,192):
      self.tpx.setDac(TPX3_VFBK,code) 
      time.sleep(0.001)
      vfbk=self.tpx.get_adc(8)
      if abs(vfbk-vthcorse)<abs(best_vfbk_val-vthcorse):
        best_vfbk_val=vfbk
        best_vfbk_code=code
    self.tpx.setDac(TPX3_VFBK,best_vfbk_code) 
    vfbk=self.tpx.get_adc(1)
    time.sleep(0.001)
    vfbk=self.tpx.get_adc(8)
    self.logging.info("  TPX3_VFBK code=%d voltage=%.1f mV"%(best_vfbk_code,(1000.0*vfbk)))

    best_vthfine_val=0
    best_vthfine_code=0
    self.tpx.setSenseDac(TPX3_VTHRESH_FINE)
    for code in range(200,300):
      self.tpx.setDac(TPX3_VTHRESH_FINE,code) 
      time.sleep(0.001)
      vthfine=self.tpx.get_adc(8)
      if abs(vfbk-vthfine)<abs(best_vthfine_val-vthfine):
        best_vthfine_val=vthfine
        best_vthfine_code=code
    self.tpx.setDac(TPX3_VTHRESH_FINE,best_vthfine_code) 
    vthfine=self.tpx.get_adc(1)
    time.sleep(0.001)
    vthfine=self.tpx.get_adc(8)
    self.logging.info("  TPX3_VTHRESH_FINE code=%d voltage=%.1f mV"%(best_vthfine_code,(1000.0*vthfine)))
    
    
    
    self.mkdir(self.fname)

    self.tpx.setShutterLen(500)
    self.tpx.sequentialReadout(tokens=4)
    self.wiki_banner(**keywords)

    logging.info("Filters eth %04x cpu %04x"%(eth_filter,cpu_filter))

    avr=[]
    std=[]	
    for cdac in range(0,16,15):
      logdir=self.fname+"/0x%0X/"%cdac
      logging.info("DACs %0x (logdir:%s)"%(cdac,logdir))
      self.mkdir(logdir)

      res={}
      for x in range(256):
        res[x]={}
        for y in range(256):
          res[x][y]=0

      self.logging.info("DAC=0x%X"%cdac)

      self.tpx.resetPixelConfig()
      self.tpx.setPixelMask(ALL_PIXELS,ALL_PIXELS,0)
      self.tpx.setPixelThreshold(ALL_PIXELS,ALL_PIXELS,cdac)

      self.tpx.pauseReadout()
      self.tpx.setPixelConfig()
      self.tpx.sequentialReadout(tokens=4)
      self.tpx.flush_udp_fifo(0x71FF000000000000)#flush until load matrix

      res=self.threshold_scan_x()

      #ThrFinder by threshold only Count>100
      thr_level_array=[]
          
      for col in range(256):  
        for row in range(256):
          #logging.info("%d %d %d %d"%(col,row,code,res[col][row][code]))
          if res[col][row] > 0:
            thr_level_array.append(res[col][row])
            #print "FOUND: %d %d %d %d"%(col,row,res[col][row][code],thr_level[col][row])     
 
#        print "DAC=0x%X MEAN=%.2f STDEV=%.2f %d"%(cdac,numpy.mean(thr_level_array),numpy.std(thr_level_array),len(thr_level_array))
      self.logging.info("          MEAN   = %.2f"%(numpy.mean(thr_level_array)))
      self.logging.info("          STDEV  = %.2f"%(numpy.std(thr_level_array)))
      fn=(self.fname+"/dacs%X_bl_thr.dat"%cdac)
      f=open(fn,"w")
      for col in range(256):
          for row in range(256):
            f.write("%d "%(res[row][col]))
          f.write("\n")  
      f.close()

    # Equalization
    eq_broute_force(self.fname)
    fn=self.fname+"/eq_codes.dat"
    self.logging.info("Storing measured codes to %s"%fn)
    eq_codes=numpy.loadtxt(fn, dtype=int)
    
    fn=self.fname+"/eq_mask.dat"
    self.logging.info("Storing mask to %s"%fn)
    eq_mask=numpy.loadtxt(fn, dtype=int)

    res={}
    for x in range(256):
      res[x]={}
      for y in range(256):
        res[x][y]=0
    
    for seq in range(4):
        logging.info("  seq %0x/4"%seq)
        self.tpx.resetPixelConfig()
        self.tpx.setPixelMask(ALL_PIXELS,ALL_PIXELS,1)
        self.tpx.setPixelThreshold(ALL_PIXELS,ALL_PIXELS,0xf)
    
        for x in range(256):
          for y in range(256):
            if x%2==int(seq/2) and y%2==seq%2:
              self.tpx.setPixelMask(x,y,eq_mask[y][x])
              self.tpx.setPixelThreshold(x,y,eq_codes[y][x])
    
        self.tpx.pauseReadout()
        self.tpx.setPixelConfig()
        self.tpx.sequentialReadout(tokens=4)
        self.tpx.flush_udp_fifo(0x71FF000000000000)#flush until load matrix
  
        res=self.threshold_scan_x(res, fromThr=200, toThr=300)
    
    fn=self.fname+"/eq_bl_measured.dat"
    self.logging.info("Storing measured baseline to %s"%fn)
    f=open(fn,"w")
    thr_level_array=[]
    for col in range(256):
      for row in range(256):
        if res[row][col] > 0 and eq_mask[row][col]==0:
          thr_level_array.append(res[row][col])
        f.write("%d "%(res[row][col]))
      f.write("\n")
    f.close()
    self.logging.info("Equalized")
    self.logging.info("          MEAN   = %.2f"%(numpy.mean(thr_level_array)))
    self.logging.info("          STDEV  = %.2f"%(numpy.std(thr_level_array)))
    self.logging.info("          MASKED = %d"  %(65536-len(thr_level_array)))
	  
	  

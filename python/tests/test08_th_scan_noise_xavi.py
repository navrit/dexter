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

      for seq in range(1):
        logging.info("  seq %0x/4"%seq)
        self.tpx.resetPixelConfig()
        self.tpx.setPixelMask(ALL_PIXELS,ALL_PIXELS,1)
        self.tpx.setPixelThreshold(ALL_PIXELS,ALL_PIXELS,cdac)
        
        #for x in range(256):
        #  for y in range(256):
        #    if x%2==int(seq/2) and y%2==seq%2:
        #      self.tpx.setPixelMask(x,y,0)
        #      self.tpx.setPixelThreshold(x,y,cdac)

        for x in range(256):
          for y in range(256):
              self.tpx.setPixelMask(x,y,0)
              self.tpx.setPixelThreshold(x,y,cdac)

	
	self.tpx.pauseReadout()
        self.tpx.setPixelConfig()
        self.tpx.sequentialReadout(tokens=4)
        self.tpx.flush_udp_fifo(0x71FF000000000000)#flush until load matrix
    	
        res=self.threshold_scan_x()
        
        
      if 0: #fitting
        bad_pixels=[]
        w2f=True
        fit_res={}
        for col in range(256):
          fit_res[col]={}
          if w2f:self.mkdir(logdir+"/%03d"%col)
          if col in res:
            logging.info("Fitting col %d"%col)
          else:
            logging.info("No hits in col %d"%col)
            continue
          for row in range(256):
            if col in res and row in res[col]:
              if w2f:fn=logdir+"/%03d/%03d_%03d.dat"%(col,col,row)
              if w2f:f=open(fn,"w")
              codes=[]
              vals=[]
              avr=0.0
              N=0
              for code in sorted(res[col][row]):
                 val=res[col][row][code]
                 if w2f and val>0:
                   f.write("%d %d\n"%(code,val))
                 if val>2:
                   codes.append(code)
                   vals.append(val)
                   avr+=code*val
                   N+=val
              if N>2:
                avr=avr/N
                try:
#                  hist, bin_edges = numpy.histogram(codes,bins=int(max(codes)-min(codes)+1),range=(-0.5+min(codes), 0.5+max(codes)), weights=vals)
#                  bin_centres = (bin_edges[:-1] + bin_edges[1:])/2
                  # p0 is the initial guess for the fitting coefficients (A, mu and sigma above)
                  p0 = [max(vals), avr, 8.]
                  coeff, var_matrix = curve_fit(gauss, codes, vals, p0=p0)
                except:
                  coeff=[0.0,0.0,0.0]
                fit_res[col][row] =coeff
              else:
                fit_res[col][row] =[0.0,0.0,0.0]
                logging.warning("No hits for pixel (%d,%d)"%(col,row))
                bad_pixels.append( (col,row) )
              if w2f:
                f.write("# %.3f %.3f\n"%(fit_res[col][row][1],fit_res[col][row][2]))
                f.close()

        
	  
	     
        fn=(self.fname+"/dacs%X_bl.dat"%cdac,self.fname+"/dacs%X_rms.dat"%cdac)
        f=(open(fn[0],"w"),open(fn[1],"w"))
        for row in range(256):
          for col in range(256):
            for i in range(2):
              if col in fit_res and row in fit_res[col]:
                f[i].write("%.3f "%abs(fit_res[col][row][1+i]))
              else:
                f[i].write("0 ")
          for i in range(2):
            f[i].write("\n")
        f[0].close()
        f[1].close()
        logging.info("Bad pixels for DAC=0x%X : %s"%(cdac,str(bad_pixels) ))
        if w2f:
          aname=logdir[:-1]+".zip"
          logging.info("Creating arhive %s"%aname)
          zipdir(aname,logdir)
          shutil.rmtree(logdir)

      if 1: #ThrFinder by threshold only Count>100
	  thr_level_array=[]
          
	  for col in range(256):  
	    for row in range(256):
 	        #logging.info("%d %d %d %d"%(col,row,code,res[col][row][code]))
		if res[col][row] > 0:
		  thr_level_array.append(res[col][row])
		  #print "FOUND: %d %d %d %d"%(col,row,res[col][row][code],thr_level[col][row])     
		  
	  print "DAC=0x%X MEAN=%.2f STDEV=%.2f %d"%(cdac,numpy.mean(thr_level_array),numpy.std(thr_level_array),len(thr_level_array))
	  fn=(self.fname+"/dacs%X_bl_thr.dat"%cdac)
	  f=open(fn,"w")
	  for col in range(256):
            for row in range(256):
              f.write("%d "%(res[row][col]))
	    f.write("\n")  
	  f.close()
	  
# Equalization
    eq_broute_force(self.fname)
	 
    eq_codes=numpy.loadtxt(self.fname+"/eq_codes.dat", dtype=int)
    eq_mask=numpy.loadtxt(self.fname+"/eq_mask.dat", dtype=int)
	  
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
    
    
    fn=(self.fname+"/eq_bl_measured.dat")
    f=open(fn,"w")
    thr_level_array=[]
    for col in range(256):
      for row in range(256):
        if res[row][col] > 0 and eq_mask[row][col]==0:
	  thr_level_array.append(res[row][col])
	f.write("%d "%(res[row][col]))
      f.write("\n")  
    f.close()
    print "EQUALIZED!!! MEAN=%.2f STDEV=%.2f MASKED=%d"%(numpy.mean(thr_level_array),numpy.std(thr_level_array),65536-len(thr_level_array))
	  
	  

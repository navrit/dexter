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
from Gnuplot import Gnuplot


def zipdir(fname,path):
    zip = zipfile.ZipFile(fname, 'w')
    for root, dirs, files in os.walk(path):
        for file in files:
#            print file
            zip.write(os.path.join(root, file))
    zip.close()
            
# Define model function to be used to fit to the data above:
def gauss(x, *p):
    A, mu, sigma = p
    return A*numpy.exp(-(x-mu)**2/(2.*sigma**2))

class test08_noise_scan(tpx3_test):

  """Threshold scan over noise"""
  def threshold_scan(self,res=None):
      self.warning_detailed_restart()

      peak=numpy.zeros((256,256), int)
      mask=0
      anim=['|','/','-','\\','|','/','-','\\']
      for i in range(0,512,1):
        self.tpx.setDac(TPX3_VTHRESH_FINE,i)
#        self.tpx.resetPixels()
        data=self.tpx.recv_mask(0x7102000000000000, 0xFFFF000000000000)

        self.tpx.openShutter()
        data=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)

        #sanity check
        if len(data)==0:
             self.logging.error("No data !!")
        elif data[-1].raw&0xFFFF00000000!=0x71A000000000:
             self.logging.error("Last packet %s"%(str(data[-1])))
             self.logging.error("Packets received %d (to be masked %d)"%(len(data),mask))
        #graphical progress bar
        if 1:
          print "%s %d/%d (packets %d)%c"%(anim[i%len(anim)],i,512,len(data),13),
          if i==511:print
          sys.stdout.flush()

        #data analisis
        for d in data:
#          logging.debug(str(d))
          if d.type==0xA:
            if d.itot<0:
              self.warning_detailed("Bad itot : "+str(d))
            if d.event_counter<0:
              self.warning_detailed("Bad event counter : "+str(d))
            res[d.col][d.row][i]=d.event_counter
            if  d.event_counter>=200 and peak[d.col][d.row]==0: 
              peak[d.col][d.row]=1
            elif d.event_counter<=2 and peak[d.col][d.row]==1:
              self.tpx.setPixelMask(d.col,d.row,1)
              mask+=1
              peak[d.col][d.row]=2
          elif d.type!=0x7:
            logging.warning("Unexpected packet %s"%str(d))

        if mask>7500:
          logging.debug("Masking %d pixels"%mask)
          self.tpx.pauseReadout()
          self.tpx.setPixelConfig()
          self.tpx.sequentialReadout()
#          self.tpx.resetPixels()
          self.tpx.flush_udp_fifo(0x71FF000000000000)#flush until load matrix
          mask=0
      self.warning_detailed_summary()
      return res

  def _execute(self,**keywords):
    self.tpx.reinitDevice()
    self.tpx.resetPixels()
    eth_filter,cpu_filter=self.tpx.getHeaderFilter()
    self.tpx.setHeaderFilter(0x0c80,cpu_filter) # 
    self.tpx.setGenConfig( TPX3_ACQMODE_EVT_ITOT )
    self.tpx.setPllConfig( (TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK | TPX3_PHASESHIFT_DIV_8 | TPX3_PHASESHIFT_NR_16 | 0x14<<TPX3_PLLOUT_CONFIG_SHIFT) )
    self.tpx.setCtprBits(0)
    self.tpx.setCtpr()
    self.tpx.setShutterLen(400)
    self.tpx.sequentialReadout()
    self.tpx.setLogLevel(2)#LVL_WARNING

    self.tpx.setDac(TPX3_IBIAS_IKRUM,15)
    self.tpx.setDac(TPX3_VTHRESH_COARSE,7) 
    self.tpx.setDac(TPX3_VTHRESH_FINE,256)

    self.logging.info("Optimization of DC operating point")

    self.tpx.setSenseDac(TPX3_VTHRESH_COARSE)
    self.tpx.setDac(TPX3_VTHRESH_COARSE,7) 
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
      vfbk=self.tpx.get_adc(2)
      if abs(vfbk-vthcorse)<abs(best_vfbk_val-vthcorse):
        best_vfbk_val=vfbk
        best_vfbk_code=code
    self.tpx.setDac(TPX3_VFBK,best_vfbk_code) 
    vfbk=self.tpx.get_adc(1)
    time.sleep(0.001)
    vfbk=self.tpx.get_adc(8)
    self.logging.info("  TPX3_VFBK code=%d voltage=%.1f mV"%(best_vfbk_code,(1000.0*vfbk)))

    self.wiki_banner(**keywords)


    #prepare result dictionary
    res={}
    for x in range(256):
      res[x]={}
      for y in range(256):
        res[x][y]={}
    
    #performe noise scans in 4 steps (one pixel out of 4 active at the time)
    for seq in range(4):
      logging.info("  seq %0x/4"%seq)
      cdac=8
#      self.tpx.resetPixelConfig()
      self.tpx.setPixelMask(ALL_PIXELS,ALL_PIXELS,1)
      self.tpx.setPixelThreshold(ALL_PIXELS,ALL_PIXELS,cdac)
      on=0
      for x in range(256):
        for y in range(256):
          if x%2==int(seq/2) and y%2==seq%2:
            self.tpx.setPixelMask(x,y,0)
      self.mask_bad_pixels()

      self.tpx.pauseReadout()
      self.tpx.setPixelConfig()
      self.tpx.sequentialReadout()
      self.tpx.flush_udp_fifo(0x71FF000000000000)#flush until load matrix
        
      res=self.threshold_scan(res)
        
    do_fit=1
    w2f=True
    logdir=self.fname+"/details/"
    if do_fit:
      bad_pixels=[]
      missing_pixels=[]
      fit_res={}
      mean_values=[]
      rms_values=[]
      self.warning_detailed_restart()
      for col in range(256):
        fit_res[col]={}
        if w2f:self.mkdir(logdir+"/%03d"%col)
        if col in res:
          print "                   Fitting col %d %c"%(col,13),
          sys.stdout.flush()
          
        else:
          self.warning_detailed("No hits in col %d"%col)
          for row in range(256):
            missing_pixels.append( (col,row) )
            fit_res[col][row] =[-1.0,-1.0,-1.0]
          continue
        for row in range(256):
          if col in res and row in res[col]:
            if w2f:
              fn=logdir+"/%03d/%03d_%03d.dat"%(col,col,row)
              f=open(fn,"w")
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

            fit_res[col][row] =[-1.0,-1.0,-1.0]
            if N>2:
              avr=avr/N
              try:
                p0 = [max(vals), avr, 6.]
                coeff, var_matrix = curve_fit(gauss, codes, vals, p0=p0)
                mean=coeff[1]
                rms=coeff[2]
                if mean<0 or mean>512:
                  mean=256.0
                  bl_off_pixels.add( (col,row) )
                if rms<2 or rms>30:
                  mean=6.0
                  noise_off_pixels.add( (col,row) )

                mean_values.append(mean)
                rms_values.append(rms)
                fit_res[col][row] =coeff
              except:
                pass
            else:
              if not (col,row) in self.bad_pixels:
                self.warning_detailed("No hits for pixel (%d,%d)"%(col,row))
                missing_pixels.append( (col,row) )
            if w2f:
              f.write("# avr:%.3f rms:%.3f\n"%(fit_res[col][row][1],fit_res[col][row][2]))
              f.close()
      self.warning_detailed_summary()

      bl_mean=numpy.mean(mean_values)
      bl_rms=numpy.std(mean_values)
      noise_mean=numpy.mean(rms_values)
      noise_rms=numpy.std(rms_values)
      
      self.results["BL_MEAN"]=bl_mean
      self.results["BL_RMS"]=bl_rms
      self.results["NOISE_MEAN"]=noise_mean
      self.results["NOISE_RMS"]=noise_rms
      
      self.logging.info("")
      self.logging.info("Baseline %3.2f std.dev. %3.2f"%(bl_mean,bl_rms))
      self.logging.info("Noise    %3.2f std.dev. %3.2f"%(noise_mean,noise_rms))
      
      fn=(self.fname+"/bl.map",self.fname+"/rms.map",self.fname+"/problematic.map")
      f=(open(fn[0],"w"),open(fn[1],"w"),open(fn[2],"w"))
      
      bl_off_pixels=[]
      noise_off_pixels=[]
      for row in range(256):
        for col in range(256):
            if col in fit_res and row in fit_res[col]:
              bl=fit_res[col][row][1]
              noise= fit_res[col][row][2]
              if fit_res[col][row][2]<-1.0:
                noise=- fit_res[col][row][2]

              problem=0
              if bl>0 and (bl>bl_mean+6.0*bl_rms  or bl<bl_mean-6.0*bl_rms) : 
                 bl_off_pixels.append( (col,row) )
                 problem=1
              if noise>0 and (noise>noise_mean*1.5 or noise<noise_mean*0.5) : 
                 noise_off_pixels.append( (col,row) )
                 problem=2
              if bl<0 or noise<0:
                 problem=3

              f[0].write("%.3f "%bl)
              f[1].write("%.3f "%noise)
              f[2].write("%d "%problem)
              

        for i in range(3):
          f[i].write("\n")
      for i in range(3):
        f[i].close()

      self.logging.info("")
      self.warn_info( "Missing pixels (very distant baseline?) (%d) : %s"%(len(missing_pixels),str(missing_pixels)) , len(missing_pixels)>0 )
      self.warn_info("Pixels with distant baseline (%d) : %s"%(len(bl_off_pixels),str(bl_off_pixels) ) , len(bl_off_pixels)>0 )
      self.warn_info("Pixels with distant noise (%d) : %s"%(len(noise_off_pixels),str(noise_off_pixels) ) , len(noise_off_pixels)>0 )


      self.add_bad_pixels(missing_pixels)
      self.add_bad_pixels(bl_off_pixels)
      self.add_bad_pixels(noise_off_pixels)
  
      self.results["MISSING"]=len(missing_pixels)
      self.results["DISTANT_BASELINE"]=len(bl_off_pixels)
      self.results["DISTANT_NOISE"]=len(noise_off_pixels)

      self.logging.info("")
      self.logging.info("Saving baseline map to %s"%fn[0])
      self.logging.info("Saving noise map to %s"%fn[1])
      self.logging.info("Saving bad pixels map to %s"%fn[2])
      
      self.logging.info("")

      if w2f:
        aname=logdir[:-1]+".zip"
        logging.info("Creating arhive %s"%aname)
        zipdir(aname,logdir)
        shutil.rmtree(logdir)

    return









#      if len(bl_off_pixels)>0:
#        g=Gnuplot()
#        g("set terminal png")
#        fn="%s/thscan_bad_bl.png"%self.fname
#        g("set output '%s'"%fn)
#        g("set grid ")
#        g("set xti 32")
#        g("set xlabel 'Threshold [LSB]'")
#        g("set ylabel 'Counts'")
#        g("set key out horiz cent top samp 1")
#        pcmd="plot "
#        for i,p in enumerate(bl_off_pixels):
#          col,row=p
#          if i>0: pcmd+=','
#          pname=logdir+"/%03d/%03d_%03d.dat"%(col,col,row)
#          pcmd+="'%s' w l t '(%d,%d)'"%(pname,col,row)
#        g(pcmd)
#        self.logging.info("Saving bad baseline scans to %s"%fn)

#        g=Gnuplot()
#        g("set terminal png")
#        fn="%s/thscan_bad_noise.png"%self.fname
#        g("set output '%s'"%fn)
#        g("set grid ")
#        g("set xti 32")
#        g("set xlabel 'Threshold [LSB]'")
#        g("set ylabel 'Counts'")
#        g("set key out horiz cent top samp 1")
#        pcmd="plot "
#        for i,p in enumerate(noise_off_pixels):
#          col,row=p
#          if i>0: pcmd+=','
#          pname=logdir+"/%03d/%03d_%03d.dat"%(col,col,row)
#          pcmd+="'%s' w l t '(%d,%d)'"%(pname,col,row)
#        g(pcmd)
#        self.logging.info("Saving bad noise scans to %s"%fn)

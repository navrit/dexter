from tpx3_test import *#tpx3_test
import random
import time
import logging
import numpy as np
from Gnuplot import Gnuplot
from scipy import  sqrt, stats
from SpidrTpx3_engine import ALL_PIXELS, TPX3_VTHRESH_COARSE


class test07_clock_phasing(tpx3_test):
  """Pixel matrix VCO and clock phasing in TOT&TOA mode"""

  def tpix3ClockPhase(self, col=0, phase_num=16):
    if not col%2:
        coeff=(col%(phase_num*2))/(phase_num/8.0)
    else:
        coeff=((col-1)%(phase_num*2))/(phase_num/8.0)
    if coeff:
        coeff=16-coeff

    return coeff

  def _execute(self,**keywords):

    self.tpx.setTpNumber(1)
#    self.tpx.setGenConfig(0x268)
    self.tpx.setGenConfig( TPX3_ACQMODE_TOA_TOT | TPX3_GRAYCOUNT_ENA | TPX3_FASTLO_ENA | TPX3_TESTPULSE_ENA | TPX3_SELECTTP_DIGITAL )#| TPX3_SELECT_TOA_CLK)

#    self.tpx.setGenConfig( TPX3_ACQMODE_TOA_TOT | TPX3_GRAYCOUNT_ENA | TPX3_FASTLO_ENA | TPX3_TESTPULSE_ENA | TPX3_SELECTTP_DIGITAL)
#   self.tpx.setPllConfig(0x11E| 0x15<<9)
    self.tpx.setPllConfig((TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK | TPX3_PHASESHIFT_DIV_8 | TPX3_PHASESHIFT_NR_16 ))#| 0x14<<TPX3_PLLOUT_CONFIG_SHIFT) )

    self.tpx.setShutterLen(1000)

    eth_filter,cpu_filter=self.tpx.getHeaderFilter()
    self.tpx.setHeaderFilter(0x0c80,cpu_filter) # 
    eth_filter,cpu_filter=self.tpx.getHeaderFilter()
    self.tpx.resetPixelConfig()
    self.tpx.setPixelTestEna(ALL_PIXELS,ALL_PIXELS, testbit=True)
    self.tpx.setPixelMask(ALL_PIXELS,ALL_PIXELS, True)
    self.tpx.setPixelConfig()
    
    self.tpx.sequentialReadout(tokens=8)
    data=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)
    pcmd="plot "
    
    missing_pixels=set()
    outliers=set()
    toa_results=[]

   
    for seq,phase in enumerate( (0,8) ):
      self.warning_detailed_restart()
      received=np.zeros((256,256), int)
      toa=np.zeros((256,256), int)
      bad_tot=np.zeros((256,256), int)
      CTPR_SPACING=16
      for ctpr in range(CTPR_SPACING):
        self.tpx.setCtprBits(0)
        c=ctpr
        while c<256:
          self.tpx.setCtprBit(c,1)
          c+=CTPR_SPACING
        self.tpx.setCtpr()

        self.tpx.resetPixels()
        self.tpx.setTpPeriodPhase(1,phase%16)
        #self.tpx.resetTimer()
        #self.tpx.setGenConfig( TPX3_ACQMODE_TOA_TOT | TPX3_FASTLO_ENA | TPX3_TESTPULSE_ENA | TPX3_SELECTTP_DIGITAL)
        self.tpx.t0Sync()
        self.tpx.openShutter()
#        self.logging.info("CTPR step %d, Waiting for data"%ctpr)
        data=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)
      
        self.logging.info("CTPR step %d, reveiced %d packets"%(ctpr,len(data)))
        shutter=self.tpx.getShutterStart()
        print "PLL=%0x"%self.tpx.getPllConfig()
        print "GenConfig=%0x"%self.tpx.getGenConfig()

        for pck in data:
          if pck.type in (0xB,0xA):
            received[pck.col][pck.row]=1
            if not pck.tot in (64,65,66) and not (pck.col,pck.row) in self.bad_pixels:
              self.warning_detailed("Unexpected TOT value %d for pixel (%d,%d)"%(pck.tot,pck.col,pck.row))
              bad_tot[pck.col][pck.row]=1
              self.bad_pixels.add( (pck.col,pck.row) )
            v=float(pck.toa-(shutter&0x3FFF)) 
            if v<0: v+=0x4000
            v=v*16 - ( float(pck.ftoa) + self.tpix3ClockPhase(col=pck.col, phase_num=16))
            toa[pck.col][pck.row]=v
          elif not pck.type in (0x7,0x4):
            self.warning_detailed("Unexpeced packet %s"%str(pck))

      cnt=np.sum(received)
      self.logging.info("Pixel packets received: %d"%(cnt))
      if cnt!=65536:
        self.logging.warning("Phase 0x%0x. Missing pixel packets : %d"%(phase,65536-cnt))
        for c in range (256):
          for r in range (256):
            if not received[c][r] : missing_pixels.add( (c,r) )
      
      self.save_np_array(toa, fn=self.fname+'/phase%02x.map'%phase, info="  TOA Map saved to %s")

      fn=self.fname+'/phase%02x.cols'%phase
      self.logging.info("Cols saved to %s"%fn)
      f=open(fn,"w")
      diffs=np.zeros((256,256), float)
      for col in range(256):
        x=[]
        y=[]
        dcol=int(col/2)
        if np.sum(received[col])==0:
           self.warning_detailed("No data for column %d"%col)
           continue

        for row in range(256):
          if (col,row) in self.bad_pixels: continue
          if not received[col][row] : 
             self.warning_detailed("No data for pixel (%d,%d)"%(col,row))
             continue
          offset=0
          if dcol%16!=0:
            offset=(15-(dcol-1)%16)
          yy=toa[col][row]-offset
          if yy<910 or yy>970:
            outliers.add( (col,row) )
            self.warning_detailed("TOA+FTOA for pixel (%d,%d) is %d. Classifying pixel as outlier."%(col,row,yy))
            continue
          x.append(row)
          y.append(yy)
        if len(x)> 1 and len(y)>1:
          (a_s,b_s,r,tt,stderr)=stats.linregress(x,y)
          f.write("%3d %.6f %.6f %.6f\n" % (col, a_s,b_s,stderr))
          for row in range(256):
            if   (not received[col][row]) \
               or ((col,row) in self.bad_pixels) \
               or ((col,row) in outliers) \
               or ((col,row) in missing_pixels) :  
               continue
            fit=row*a_s+b_s
            offset=0
            if dcol%16!=0:
              offset=(15-(dcol-1)%16)
            y=toa[col][row]-offset
            diffs[col][row]=(fit-y)
      f.close()

      mean=np.mean(diffs)
      stddev=np.std(diffs)
      l3lsb=0
      h3lsb=0
      for c in range(256):
        for r in range(256):
          if diffs[c][r]>3: 
            h3lsb+=1
            outliers.add( (c,r) )
          if diffs[c][r]<-3: 
            l3lsb+=1
            outliers.add( (c,r) )


      self.logging.info("")
      self.logging.info("Phase %d"%phase)
      self.logging.info("  Mean :           %.3f"%mean)
      self.logging.info("  Std. dev. :      %.3f"%stddev)
      self.warn_info("  Higher > 3 LSB : %d"%h3lsb, h3lsb>0 )
      self.warn_info("  Lower < -3 LSB : %d"%l3lsb, l3lsb>0 )
    

      self.results['TOA_TEST_PHASE%0x_MEAN'%phase]="%.4f"%mean
      self.results['TOA_TEST_PHASE%0x_STDDEV'%phase]="%.4f"%stddev
      self.results['TOA_TEST_PHASE%0x_HIGHER'%phase]="%d"%h3lsb
      self.results['TOA_TEST_PHASE%0x_LOWER'%phase]="%d"%l3lsb

      toa_results.append(toa)
      self.warning_detailed_summary()



    if len(toa_results)==2:
      toa_diff=toa_results[0]-toa_results[1]
      self.save_np_array(toa_diff, fn=self.fname+'/diff.map', info="  TOA diff map saved to %s")

      l3lsb=0
      h3lsb=0
      HLIMIT=int(8+3.0)
      LLIMIT=int(8-3.0)
      
      for c in range(256):
        for r in range(256):
          if (c,r) in missing_pixels: continue
          if toa_diff[c][r]>HLIMIT: 
            h3lsb+=1
            outliers.add( (c,r) )
          if toa_diff[c][r]<LLIMIT: 
            l3lsb+=1            
            outliers.add( (c,r) )

      for c in range(256):
        for r in range(256):
          if (c,r) in missing_pixels:
            toa_diff[c][r]=8
          if toa_diff[c][r]<0 or toa_diff[c][r]>16: 
            toa_diff[c][r]=8
            outliers.add( (c,r) )

      dmean=np.mean(toa_diff)
      dstd=np.std(toa_diff)


      self.logging.info("")
      self.logging.info("Difference")
      self.logging.info("  Diff mean      : %.3f"%dmean)
      self.logging.info("  Diff std       : %.3f"%dstd)
      self.warn_info("  Higher > 3 LSB : %d"%h3lsb, h3lsb>0 )
      self.warn_info("  Lower < -3 LSB : %d"%l3lsb, l3lsb>0 )
      
      self.results['TOA_TEST_DIFF_MEAN']="%.3f"%dmean
      self.results['TOA_TEST_DIFF_STDDEV']="%.3f"%dstd
      self.results['TOA_TEST_DIFF_HIGHER']="%d"%h3lsb
      self.results['TOA_TEST_DIFF_LOWER']="%d"%l3lsb


    self.add_bad_pixels(outliers)

    btot_cnt=0
    for c in range(256):
      for r in range(256):
        if bad_tot[c][r]>0:
          btot_cnt+=1
          self.bad_pixels.add( (c,r) )
    
    self.results['TOA_TEST_MISSING_PIXELS']=len(missing_pixels)
    self.results['TOA_TEST_OUTLIERS']=len(outliers)
    self.results['TOA_TEST_BAD_TOT']=btot_cnt

    self.logging.info("")
    self.logging.info("Total number of outliers :%d"%len(outliers))
    self.logging.info("Pixels with bad TOT :%d"%btot_cnt)

    return 

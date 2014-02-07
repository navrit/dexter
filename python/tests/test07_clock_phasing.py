from tpx3_test import tpx3_test
import random
import time
import logging
import numpy as np
from Gnuplot import Gnuplot
from scipy import  sqrt, stats
from SpidrTpx3_engine import ALL_PIXELS, TPX3_VTHRESH_COARSE


class test07_clock_phasing(tpx3_test):
  """Pixel matrix VCO and clock phasing in TOT&TOA mode"""

  def _execute(self,**keywords):
    self.tpx.reinitDevice()
    self.tpx.setTpNumber(1)
    self.tpx.resetPixels()
    self.tpx.resetPixelConfig()
    self.tpx.setTpNumber(1)
    self.tpx.setDac(TPX3_VTHRESH_COARSE, 0)

    self.tpx.setCtprBits(1)
    self.tpx.setCtpr()
    
#    gc=self.tpx.getGenConfig()
#    pll=self.tpx.getPllConfig()
    self.tpx.setGenConfig(0x268)
    self.tpx.setPllConfig(0x11E| 0x15<<9)

    self.tpx.setShutterLen(1000)
    eth_filter,cpu_filter=self.tpx.getHeaderFilter()
    self.tpx.setHeaderFilter(0x0c80,cpu_filter) # 
    eth_filter,cpu_filter=self.tpx.getHeaderFilter()

    self.tpx.resetPixelConfig()
    self.tpx.setPixelTestEna(ALL_PIXELS,ALL_PIXELS, testbit=True)
    self.tpx.setPixelConfig()
    g=Gnuplot()
    g("set terminal png")
    g("set grid")
    g("set xlabel 'Time error [LSB/1.56ns]'")
    g("set ylabel 'Entries'")
    g("set output '%s/hist.png"%self.fname)
    pstr=""
#    min_toa,max_toa
#    self.tpx.datadrivenReadout()
    self.tpx.sequentialReadout(tokens=2)
    data=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)
    ret_values={}
    for seq,phase in enumerate( (0,8) ):
      self.tpx.setTpPeriodPhase(1,phase)

      self.tpx.send(0x40,0,0)#reset timer
      self.tpx.send(0x4A,0,0)#t0sync

      self.tpx.openShutter()
      self.tpx.flush_udp_fifo(val=0x714A000000000000, mask=0xFFFF000000000000)

      self.logging.info("Wait for data")
      data=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)
      self.logging.info("Reveiced %d packets"%(len(data)))
      shutter=self.tpx.getShutterStart()

      cnt=0
      result={}
      for col in range(256):
        result[col]={}
        for row in range(256):
          result[col][row]=0.0

      for pck in data:
         if pck.type in (0xB,0xA):
           if not pck.tot in (64,65):
             self.logging.warning("Unexpected TOT value %d for pixel (%d,%d)"%(pck.tot,pck.col,pck.row))
           
           cnt+=1
           v=float(pck.toa-(shutter&0x3FFF)) 
           if v<0: v+=0x4000
         
           v=v*16 - float(pck.ftoa)
           result[pck.col][pck.row]=v
         elif pck.type !=0x7:
           self.logging.warning("Unexpeced packet %s"%str(pck))

      self.logging.info("Pixel packets received: %d"%(cnt))
      fn=self.fname+'/phase%02x.cols'%phase
      self.logging.info("Cols saved to %s"%fn)
      f=open(fn,"w")
      diffs=[]
      for col in range(256):
        x=[]
        y=[]
        dcol=int(col/2)
        if not col in result:
           self.logging.warrning("No data for column %d"%col)
           continue
        mis=0
        for row in range(256):
          if not row in result[col] : 
             self.logging.warning("No data for pixel (%d,%d)"%(col,row))
             mis+=1
             continue
          x.append(row)
          offset=0
          if dcol%16!=0:
            offset=(15-(dcol-1)%16)
          yy=result[col][row]-offset
          y.append(yy)
          
        (a_s,b_s,r,tt,stderr)=stats.linregress(x,y)
        f.write("%3d %.6f %.6f %.6f %d\n" % (col, a_s,b_s,stderr,mis))
        for row in range(256):
          if not row in result[col] : 
             continue
          fit=row*a_s+b_s
          offset=0
          if dcol%16!=0:
            offset=(15-(dcol-1)%16)
          y=result[col][row]-offset

          diffs.append(fit-y)

#      f2.close()
      if mis>0:
        self.logging.warning("Pixels missing: %d"%(mis))

      f.close()
      hist, bin_edges=np.histogram(diffs, range=(-16,16),bins=320)
      fn=self.fname+'/phase%02x.hst'%phase
      def save_hist(fname,hist,bin_edges):
        f=open(fname,"w")
        for i in range(len(bin_edges)-1):
          f.write("%.4e %d\n"%(bin_edges[i],hist[i]))
          f.write("%.4e %d\n"%(bin_edges[i+1],hist[i]))
        f.close()
      save_hist(fn,hist,bin_edges)
      
      g("plot '%s' w lp t 'phase=0x%0x'"%(fn,phase))
      self.logging.info("Errors saved to %s"%fn)
      stddev=np.std(diffs)
      ret_values['PHASE%0x_STDDEV'%phase]="%.4f"%stddev
      self.logging.info("Std. dev. %.3f"%stddev)
      l5s=0
      h5s=0
      sd5=3.0#stddev*.0
      for d in diffs:
        if d>sd5: h5s+=1
        if d<-sd5: l5s+=1
      self.logging.info("Higher > 3 LSB : %d"%h5s)
      self.logging.info("Lower < -3 LSB : %d"%l5s)
      ret_values['PHASE%0x_HIGHER'%phase]="%d"%h5s
      ret_values['PHASE%0x_LOWER'%phase]="%d"%l5s

      fn=self.fname+'/phase%02x.map'%phase
      self.logging.info("Plot saved to %s"%fn)
      f=open(fn,"w")
      for row in range(256):
        for col in range(256):
          if col in result and row in result[col] : f.write("%.2f "%result[col][row])
          else: f.write("0 ")
        f.write("\n")
      f.close()
      
    fn=self.fname+"/results.txt"
    self.dict2file(fn,ret_values)
    self.logging.info("Results stored to %s"%fn)


#(0x44) and High (0x45)

from tpx3_test import tpx3_test
import random
import time
import logging
import numpy as np
from scipy import  sqrt, stats
from SpidrTpx3_engine import ALL_PIXELS, TPX3_VTHRESH_COARSE

class test07_clock_phasing(tpx3_test):
  """Pixel matrix VCO and clock phasing in TOT&TOA mode"""

  def _execute(self,**keywords):
    self.tpx.setTpNumber(1)
    self.tpx.resetPixels()
    self.tpx.resetPixelConfig()
    self.tpx.setTpPeriodPhase(1,1)
    self.tpx.setTpNumber(1)
    self.tpx.setDac(TPX3_VTHRESH_COARSE, 0)

    self.tpx.setCtprBits(1)
    self.tpx.setCtpr()
    
#    gc=self.tpx.getGenConfig()
    self.tpx.setGenConfig(0x268)

    self.tpx.setPllConfig(0x11E)
    pll=self.tpx.getPllConfig()

   
#    self.tpx.flush_udp_fifo()
    self.tpx.setShutterLen(1000)

    self.tpx.resetPixelConfig()
    self.tpx.setPixelTestEna(ALL_PIXELS,ALL_PIXELS, testbit=True)
    self.tpx.setPixelConfig()

    self.tpx.send(0x40,0,0)#reset timer
    self.tpx.send(0x4A,0,0)#t0sync

    self.tpx.datadrivenReadout()
#    self.tpx.sequentialReadout(tokens=4)
#    data=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)

    self.tpx.openShutter()
#       data=self.tpx.recv_mask(0x1111,0xFFFF)
    logging.info("Wait for data")
    data=self.tpx.recv_mask(0x71B0000000000000, 0xFFFF000000000000)
    logging.info("Reveiced %d packets"%(len(data)))
    shutter=self.tpx.getShutterStart()
    cnt=0
    result={}
    for col in range(256):
      result[col]={}
      for row in range(256):
        result[col][row]=None

    for pck in data:
         if pck.type==0xB:
            cnt+=1
            v=float(pck.toa-(shutter&0x3FFF)) 
            if v<0: v+=0x4000
            v-=float(pck.ftoa)/16
            result[pck.col][pck.row]=v

    print "Total pck count ",cnt
    fn=self.fname+'.cols'
    logging.info("Cols saved to %s"%fn)
    f=open(fn,"w")
    for col in range(256):
      x=[]
      y=[]
      if not col in result:
         logging.warrning("No data for column %d"%col)
         continue
      mis=0
      for row in range(256):
        if not row in result[col] : 
           logging.warning("No data for pixel (%d,%d)"%(col,row))
           mis+=1
           continue
        x.append(row)
        y.append(result[col][row])
#      z = np.polyfit(x, y, 1)
      (a_s,b_s,r,tt,stderr)=stats.linregress(x,y)
      f.write("%3d %.6f %.6f %.6f %d\n" % (col, a_s,b_s,stderr,mis))
  
#      print col,z
    f.close()
    
    
    fn=self.fname+'.map'
    logging.info("Plot saved to %s"%fn)
    f=open(fn,"w")
    for row in range(256):
      for col in range(256):
        if col in result and row in result[col] : f.write("%.2f "%result[col][row])
        else: f.write("0 ")
      f.write("\n")
    f.close()



#(0x44) and High (0x45)

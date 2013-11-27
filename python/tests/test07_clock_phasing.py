from tpx3_test import tpx3_test
import random
import time
import logging
from SpidrTpx3_engine import ALL_PIXELS, TPX3_VTHRESH_COARSE

class test07_clock_phasing(tpx3_test):
  """Pixel matrix VCO and clock phasing in TOT&TOA mode"""

  def _execute(self,**keywords):
    self.tpx.setTpNumber(1)
    self.tpx.flushFifoIn()
    self.tpx.resetPixels()
    self.tpx.resetPixelConfig()
    self.tpx.setTpPeriodPhase(1,1)
    self.tpx.setTpNumber(1)
    self.tpx.setDac(TPX3_VTHRESH_COARSE, 0)
#    self.tpx.setPixelConfig()
    for c in range(256):
      self.tpx.configCtpr(c,1)
    self.tpx.setCtpr()
    
#    gc=self.tpx.getGenConfig()
    self.tpx.setGenConfig(0x268)

    self.tpx.setPllConfig(0x11E)
    pll=self.tpx.getPllConfig()

   
    result={}
    if 1:
        self.tpx.resetPixelConfig()
        for x in range(256):
         result[x]={}
         for y in range(256):
          self.tpx.configPixel(x,y,threshold=0, testbit=True)
        self.tpx.setPixelConfig()
        self.tpx.send(0x40,0,0)#reset timer
        self.tpx.send(0x4A,0,0)#t0sync

        self.tpx.datadrivenReadout()
        self.tpx.openShutter(100)
        data=self.tpx.recv_mask(0x1111,0xFFFF)
        self.tpx.flushFifoIn()
        shutter=self.tpx.getShutterStart()
        cnt=0
        for pck in data:
          if pck.type==0xB:
            cnt+=1
            v=float(pck.toa-(shutter&0x3FFF)) 
            if v<0: v+=0x4000
            v-=float(pck.ftoa)/16
            result[pck.col][pck.row]=v
        print "Total pck count ",cnt
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

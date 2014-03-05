from tpx3_test import tpx3_test
import random
import time
import logging
import numpy as np
from Gnuplot import Gnuplot
from scipy import  sqrt, stats
from SpidrTpx3_engine import ALL_PIXELS, TPX3_VTHRESH_COARSE


class test04_pixel_kidnapper(tpx3_test):
  """Pixel kidnaper test"""

  def _execute(self,**keywords):
    self.tpx.reinitDevice()
    self.tpx.setTpNumber(1)
    self.tpx.resetPixels()

    self.tpx.setCtprBits(1)
    self.tpx.setCtpr()
    
    self.tpx.setGenConfig(0x268)
    self.tpx.setPllConfig(0x11E| 0x15<<9)

    self.tpx.setShutterLen(1000)

    eth_filter,cpu_filter=self.tpx.getHeaderFilter()
    self.tpx.setHeaderFilter(0x0c80,cpu_filter) # 
    eth_filter,cpu_filter=self.tpx.getHeaderFilter()

    self.tpx.resetPixelConfig()
    self.tpx.setPixelTestEna(ALL_PIXELS,ALL_PIXELS, testbit=True)
    self.tpx.setPixelMask(ALL_PIXELS,ALL_PIXELS, True)
    self.tpx.setPixelConfig()

    self.tpx.setTpPeriodPhase(1,0)
    
    self.tpx.sequentialReadout(tokens=4)
    data=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)
    self.results={}

    bad_tot=[]
    kidnappers=[]
    missing=set()
    received=(np.zeros((256,256)) , np.zeros((256,256)))
    for seq in range(2):

      self.warning_detailed_restart()
      self.logging.info("Round %d"%seq)
      self.tpx.openShutter()
      data=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)
      self.logging.info("Reveiced %d packets"%(len(data)))
      cnt=0
      
      for pck in data:
         if pck.type in (0xB,0xA):
           received[seq][pck.col][pck.row]=1
           if seq==0 and (not pck.tot in (64,65,66)):
             self.warning_detailed("Unexpected TOT value %d for pixel (%d,%d)"%(pck.tot,pck.col,pck.row))
             if not (pck.col,pck.row) in self.bad_pixels:
               bad_tot.append( (pck.col,pck.row) )
         elif pck.type !=0x7:
           self.warning_detailed("Unexpeced packet %s"%str(pck))

      cnt=np.sum(received[seq])
      self.logging.info("Pixel packets received: %d"%(cnt))
      if cnt!=65536:
        self.logging.warning("Missing pixel packets : %d"%(65536-cnt))
        if seq==0:
          self.logging.warning("Problems already in the first round !! Results may be unreliable")

      self.warning_detailed_summary()

    self.warning_detailed_restart()
    for col in range(256):
      for row in range(256):
        if not received[0][col][row] and not received[1][col][row]:
          self.warning_detailed("Missing pixel (%d,%d) (in both rounds)"%(col,row))
          missing.add( (col,row) )
        elif  received[0][col][row] and not received[1][col][row]:
          self.warning_detailed("Kidnapper pixel (%d,%d)"%(col,row))
          if not (col,row) in self.bad_pixels:
             kidnappers.append( (col,row) )
    self.warning_detailed_summary()


#    mask_pixels=bad_tot+kidnappers
    for p in kidnappers:
      self.bad_pixels.add( p )

    for p in bad_tot:
      self.bad_pixels.add( p )

    if len(kidnappers)>0:
       self.update_category("K")

    self.results['KID_TEST_KIDNAPPERS']=len(kidnappers)
    self.results['KID_TEST_BAD_TOT']=len(bad_tot)
    self.results["KID_TEST_MISSING_PACKETS"]=len(missing)

    self.logging.info("")

    self.warn_info("Kidnapper pixels    : %d"%(len(kidnappers)), len(kidnappers)>0)
    self.warn_info("Pixels with bad tot : %d"%(len(bad_tot)), len(bad_tot)>0)
    self.warn_info("Missing pixels      : %d"%(len(missing)), len(missing)>0)


    
    
    return 
#(0x44) and High (0x45)

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
    cat=keywords['category']
    mask_pixels=keywords['mask_pixels']
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
    
    self.tpx.sequentialReadout(tokens=4)
    data=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)
    ret_values={}

    self.tpx.setTpPeriodPhase(1,0)
    bad_tot=[]
    kidnappers=[]
    for seq in range(2):
      self.logging.info("Round %d"%seq)
      self.tpx.openShutter()
      data=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)
      self.logging.info("Reveiced %d packets"%(len(data)))
      shutter=self.tpx.getShutterStart()
      cnt=0
      result={}
      for pck in data:
         if pck.type in (0xB,0xA):
           if seq==0 and (not pck.tot in (64,65,66)):
             self.logging.warning("Unexpected TOT value %d for pixel (%d,%d)"%(pck.tot,pck.col,pck.row))
             if not (pck.col,pck.row) in mask_pixels:
               bad_tot.append( (pck.col,pck.row) )
           cnt+=1
           if not pck.col in result:
             result[pck.col]={}

           result[pck.col][pck.row]=1
         elif pck.type !=0x7:
           self.logging.warning("Unexpeced packet %s"%str(pck))

      self.logging.info("Pixel packets received: %d"%(cnt))
      if cnt!=65536:
        self.logging.warning("Missing pixel packets : %d"%(65536-cnt))
        if seq==0:
          self.logging.warning("Problems already in the first round !! Probably the rest of this test is urMissing pixel packets : %d"%(65536-cnt))

      if seq==0 and len(bad_tot):
        cols = {}
        for col,row in bad_tot:
          if not col in cols: cols[col]=0
          cols[col]+=1
        self.logging.warning("There are pixels with bad TOT values (most likely there are nois packets received): %d"%(len(bad_tot)))
        for col in sorted(cols.keys()):
          self.logging.warning("Column %3d bad pixels: %d"%(col,cols[col]))
        cat="B_badcol%d"%(len(cols.keys()))
          
      for col in range(256):
        for row in range(256):
          if (not col in result) or (not row in result[col]) or  (not result[col][row]):
            self.logging.warning("Missing pixel data (%d,%d) during round %d"%(col,row,seq))
            if not (col,row) in mask_pixels:
               kidnappers.append( (col,row) )
      ret_values["MISSING_PACKETS"]=65536-cnt

    mask_pixels=bad_tot+kidnappers
    if len(kidnappers)>0:
       self.logging.warning("Kidnappers %d"%(len(kidnappers)))
       if cat=='A':
         cat='B_kidnapper'
       elif cat[0]=='B':
         cat+='_kidnapper'

    ret_values['KIDNAPPERS']=len(kidnappers)
    ret_values['BAD_TOT']=len(bad_tot)

    fn=self.fname+"/results.txt"
    self.dict2file(fn,ret_values)
    self.logging.info("Results stored to %s"%fn)
    
    return {'category':cat,'info':keywords['info'], 'continue':True, 'mask_pixels':mask_pixels}
#(0x44) and High (0x45)

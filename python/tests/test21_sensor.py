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

class test21_sensor(tpx3_test):
  """Threshold scan over noise frame based"""

  def _execute(self,**keywords):
    self.tpx.resetPixels()
    dac_defaults(self.tpx)

    self.tpx.setDac(TPX3_IBIAS_IKRUM,5)
    self.tpx.setDac(TPX3_IBIAS_DISCS1_ON,100)
    self.tpx.setDac(TPX3_IBIAS_DISCS2_ON,128)
    self.tpx.setDac(TPX3_IBIAS_PREAMP_ON,128)    
    self.tpx.setDac(TPX3_IBIAS_PIXELDAC,128)
    self.tpx.setDac(TPX3_VFBK,164) 

    self.tpx.setDecodersEna(True)
    self.tpx.setOutputMask(255)
    self.tpx.setPllConfig( (TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK | TPX3_PHASESHIFT_DIV_8 | TPX3_PHASESHIFT_NR_16 | 0x14<<TPX3_PLLOUT_CONFIG_SHIFT) )
    
    polarity=True
    genConfig_register=TPX3_ACQMODE_EVT_ITOT 
    if not polarity: genConfig_register|=TPX3_POLARITY_EMIN
    shutter_length=1000000
    self.logging.info("Shutter length %d us"%shutter_length)
    self.tpx.setGenConfig( genConfig_register)
    self.tpx.setCtprBits(0)
    self.tpx.setCtpr()
    def load(fn):
      f=open(fn,"r")
      ret=[]
      for l in f.readlines():
        ll=[]
        for n in l.split():
          n=int(n)
          ll.append(n)
        ret.append(ll)
      f.close()
      return ret

    self.tpx.resetPixelConfig()
    self.tpx.load_equalization('calib/eq_codes_noiseFloor_ik5_w2l6_2.dat',\
                      maskname='calib/eq_mask_noiseFloor_ik5_w2l6_2.dat')
#    self.tpx.load_equalization('calib/eq_codes_noiseFloor_ik5_w2c8.dat',\
#                      maskname='calib/eq_mask_noiseFloor_ik5_w2c8.dat')
#    self.tpx.setPixelMask(142,170,1)
    self.tpx.setPixelConfig()

    self.mkdir(self.fname)
    self.tpx.setShutterLen(shutter_length)
    self.tpx.sequentialReadout(tokens=1)
    v_fbk=self.tpx.get_adc(32)
    self.tpx.resetTimer()
    self.tpx.t0Sync()
    FROM_THR=1130
    TO_THR=1200
    STEP=200
    evn_total=np.zeros(((TO_THR-FROM_THR)/STEP+1,256,256), int)
#    evn_total=np.zeros((500,256,256), int)
    total_evnt_frames=np.zeros(((TO_THR-FROM_THR)/STEP+1,1), int)
    for frame in range(1):
     f=open(self.fname+"/thscan_frame%d.dat"%frame,"w")
     f.close()
     for thr in range(FROM_THR,TO_THR+1,STEP):
      index_thr=(TO_THR-thr)/STEP
      #print index_thr
      self.tpx.setThreshold(thr)
      v_thr=self.tpx.get_adc(8)
      self.tpx.openShutter()
      data=self.tpx.get_frame()
      shutter_start=self.tpx.getShutterStart()
      shutter_stop=self.tpx.getShutterEnd()
      evn=np.zeros((256,256), int)
      #itot=np.zeros((256,256), int)

      num_pixels=0
      total_evnt=0
      total_itot=0

      for d in data:
        if d.type==0xA:
#           print d
#           if evn[d.col][d.row]: print "!!"
           num_pixels+=1
           total_evnt+=d.event_counter
           total_itot+=d.itot
           evn[d.col][d.row]=d.event_counter
           evn_total[index_thr][d.col][d.row]+=d.event_counter
           total_evnt_frames[index_thr]+=d.event_counter

           #itot[d.col][d.row]=d.itot
        elif d.type!=0x7:
           self.warning_detailed("Unexpected packet %s"%str(d))
      temp=self.tpx.getTpix3Temp()
      shutterTime=(shutter_stop-shutter_start)*0.000025
      hitRate=total_evnt/(1000*shutterTime)
      if num_pixels>0:
        pixelhitRate=hitRate*1000000/num_pixels
      else:
        pixelhitRate=0
      print "%d Thr=%d (%.3f Ke-) pixels %d total count %d total temp %.1fC ChipRate %.3f Mhit/s PixelRate %.3f hit/s"\
            %(frame,thr,(v_fbk-v_thr)*20,num_pixels,total_evnt_frames[index_thr],temp,hitRate,pixelhitRate)

      f=open(self.fname+"/thscan_frame%d.dat"%frame,"a")
      line="%d %d %d %d %.2f %.3f\n"%(frame,thr,num_pixels,total_evnt_frames[index_thr],(v_fbk-v_thr)*20,temp)
      f.write(line)
      f.close()
      self.save_np_array(evn, fn=self.fname+'/evn_thr%d_frame%d.map'%(thr,frame))#, info="  EVN to %s")
      self.save_np_array(evn_total[index_thr], fn=self.fname+'/evn_thr%d.map'%(thr))#, info="  EVN to %s")
      #self.save_np_array(itot, fn=self.fname+'/itot_frame%d.map'%frame)#, info="  iTOT saved to %s")


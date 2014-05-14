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

class test22_sensor(tpx3_test):
  """Threshold scan over noise frame based"""

  def tpix3_temp(self):
    self.tpx.setSenseDac(TPX3_BANDGAP_TEMP)
    v_bg_temp=self.tpx.get_adc(32)
    self.tpx.setSenseDac(TPX3_BANDGAP_OUTPUT)
    v_bg=self.tpx.get_adc(32)
    return 88.75-607.3*(v_bg_temp-v_bg)     #Mpix3 extracted

  def SetThreshold(self,dac_value=1000):
    i=0
    coarse_found=0
    fine_found=352
    for coarse in range(16):
       for fine in range(352,512,1):
          if dac_value==i:
             coarse_found=coarse
             fine_found=fine
          i+=1
    self.tpx.setDac(TPX3_VTHRESH_COARSE,coarse_found)
    self.tpx.setDac(TPX3_VTHRESH_FINE,fine_found)
    #print "%d %d %d \n"%(i,coarse_found,fine_found)

  def _execute(self,**keywords):
    self.tpx.resetPixels()
    dac_defaults(self.tpx)

    self.tpx.setDac(TPX3_IBIAS_IKRUM,15)
    self.tpx.setDac(TPX3_IBIAS_DISCS1_ON,128)
    self.tpx.setDac(TPX3_IBIAS_DISCS2_ON,32)
    self.tpx.setDac(TPX3_IBIAS_PREAMP_ON,128)    
    self.tpx.setDac(TPX3_IBIAS_PIXELDAC,128)
    self.tpx.setDac(TPX3_VTHRESH_COARSE,5) 
    self.tpx.setDac(TPX3_VFBK,164) 
    self.tpx.setDac(TPX3_VTHRESH_FINE,256)

    eth_filter,cpu_filter=self.tpx.getHeaderFilter()
    self.tpx.setHeaderFilter(0x0c80,cpu_filter) # 
    eth_filter,cpu_filter=self.tpx.getHeaderFilter()
    
    self.tpx.setPllConfig( (TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK | TPX3_PHASESHIFT_DIV_8 | TPX3_PHASESHIFT_NR_1 | 0x14<<TPX3_PLLOUT_CONFIG_SHIFT) )
    
    polarity=True
    genConfig_register=TPX3_ACQMODE_TOA_TOT | TPX3_GRAYCOUNT_ENA | TPX3_FASTLO_ENA #TPX3_ACQMODE_EVT_ITOT 
    if not polarity: genConfig_register|=TPX3_POLARITY_EMIN
    shutter_length=10000000
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
    self.tpx.load_equalization('logs/W2H11/00_20140502_101303/test08_equalization_pixeDAC_scan/eq_code_pixelDAC_scan.dat',\
                      maskname='logs/W2H11/00_20140502_101303/test08_equalization_pixeDAC_scan/eq_code_pixelDAC_scanMask2.dat')

    self.tpx.load_equalization('logs/W2H11/303_20140505_161145/test08_equalization_pixeDAC_scan/eq_code_pixelDAC_scan.dat',\
                      maskname='logs/W2H11/303_20140505_161145/test08_equalization_pixeDAC_scan/eq_code_pixelDAC_scanMask.dat')

    self.tpx.load_equalization('logs/W2H11/472_20140507_103747/test08_equalization_x/eq_codes.dat',\
                      maskname='logs/W2H11/472_20140507_103747/test08_equalization_x/eq_mask.dat')

    self.tpx.load_equalization('logs/W2L6/20_20140514_083602/test08_equalization_x/eq_codes.dat',\
                      maskname='logs/W2L6/20_20140514_083602/test08_equalization_x/eq_mask.dat')


    #mask=load('logs/W2H11/303_20140505_161145/test08_equalization_pixeDAC_scan/eq_code_pixelDAC_scanMask.dat')
    self.tpx.setPixelMask(142,170,1)
    #f=open("logs/W2H11/303_20140505_161145/test08_equalization_pixeDAC_scan/eq_code_pixelDAC_scanMask3.dat","w")
    #for col in range(256):
    #  for row in range(256):
    #     f.write("%d "%(mask[row][col]))
    #  f.write("\n")  
    #f.close()
    #self.tpx.setPixelMask(200,197,1)
    #self.tpx.setPixelMask(ALL_PIXELS,ALL_PIXELS,1)
    #for x in range(256):
    #        for y in range(256):
    #             if x>y:
    #                self.tpx.setPixelMask(x,y,0)
    self.tpx.setPixelConfig()

    self.tpx.flush_udp_fifo(0x718F000000000000)
    self.mkdir(self.fname)
    self.tpx.setShutterLen(shutter_length)
    self.tpx.sequentialReadout(tokens=1)
    #self.tpx.datadrivenReadout()
    data=self.tpx.get_frame()
    #self.tpx.resetTimer()
    #self.tpx.t0Sync()
    self.tpx.setSenseDac(TPX3_VFBK)
    v_fbk=self.tpx.get_adc(32)
    if 0:
      f=open(self.fname+"/thscan.dat","w")
      for th in range(272,512,8):
        self.tpx.setDac(TPX3_VTHRESH_FINE,th)
        v_thr=self.tpx.get_adc(32)
        self.tpx.openShutter()
        data=self.tpx.get_frame()
        num_pixels=0
        total_evnt=0
        total_itot=0
        for d in data:
           if d.type==0xB:
#           if evn[d.col][d.row]: print "!!"
             num_pixels+=1
#             total_evnt+=d.event_counter
#             total_itot+=d.itot
        line="%d %d %d %d %.2f\n"%(th,num_pixels,total_evnt,total_itot,1000*(v_fbk-v_thr)*20*3.62)
        print line,
        f.write(line)
      f.close()
    self.tpx.resetTimer()
    self.tpx.t0Sync()

    anim=['|','/','-','\\','|','/','-','\\']
    f=open(self.fname+'/cosmic_data.dat',"w")
    f.close()
    event_counter=0
#   for thr in range(1000,1210,5):
    for frame in range(120):
      thr =1150
      frame=0
      self.tpx.setSenseDac(TPX3_VTHRESH_FINE)
      self.SetThreshold(thr)
      v_thr=self.tpx.get_adc(8)
      self.tpx.openShutter_pc()
      data=self.tpx.get_frame()
      shutter_start=self.tpx.getShutterStart()
      shutter_stop=self.tpx.getShutterEnd()
      evn=np.zeros((256,256), int)
      itot=np.zeros((256,256), int)

      num_pixels=0
      total_evnt=0
      total_itot=0

      for pck in data:
        if pck.type==0xA:
          line="%d\t%d\t%d\t%d\t%d\t%d"%(event_counter,pck.col,pck.row,pck.toa,pck.tot,pck.ftoa)
          print line
          event_counter+=1
          evn[pck.col][pck.row]+=1
          #print "%c %s %d hits "%(13,anim[event_counter%len(anim)],event_counter),
          #sys.stdout.flush()
          f=open(self.fname+'/cosmic_data.dat',"a")
          f.write(line+"\n")
          f.close()
      temp=self.tpix3_temp()
      #print "%d Thr=%d (%.3f Ke-) pixels %d total count %d total itot %d temp %.1fC shutter %.3fms"%(frame,thr,(v_fbk-v_thr)*20,num_pixels,total_evnt,total_itot,temp,\
#                                                                                     (shutter_stop-shutter_start)*0.000025)
      f=open(self.fname+"/thscan.dat","a")
      line="%d %d %d %d %d %.2f %.3f\n"%(frame,thr,num_pixels,total_evnt,total_itot,(v_fbk-v_thr)*20,temp)
      f.write(line)
      f.close()
#      self.save_np_array(evn, fn=self.fname+'/evn_frame%d.map'%thr)#, info="  EVN to %s")
#      self.save_np_array(itot, fn=self.fname+'/itot_frame%d.map'%frame)#, info="  iTOT saved to %s")


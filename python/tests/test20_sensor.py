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



class test20_sensor(tpx3_test):
  """Threshold scan over noise data driven"""

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
    self.tpx.setPllConfig( (TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK | TPX3_PHASESHIFT_DIV_8 | TPX3_PHASESHIFT_NR_1 | 0x14<<TPX3_PLLOUT_CONFIG_SHIFT) )
    
    polarity=True
    genConfig_register=TPX3_ACQMODE_TOA_TOT | TPX3_GRAYCOUNT_ENA | TPX3_FASTLO_ENA 
    if not polarity: genConfig_register|=TPX3_POLARITY_EMIN
    shutter_length=1000000
    self.logging.info("Shutter length %d us"%shutter_length)
    self.tpx.setGenConfig( genConfig_register)
    self.tpx.setCtprBits(0)
    self.tpx.setCtpr()
    self.tpx.resetPixelConfig()

    self.tpx.load_equalization('calib/eq_codes_noiseFloor_ik5_w2l6_2.dat',\
                      maskname='calib/eq_mask_noiseFloor_ik5_w2l6_2.dat')
    self.tpx.setPixelMask(149,43,1)
    self.tpx.setPixelMask(222,56,1)
    self.tpx.setPixelMask(222,55,1)
    self.tpx.setPixelMask(223,56,1)
    self.tpx.setPixelMask(221,56,1)
    self.tpx.setPixelMask(222,57,1)
    self.tpx.setPixelMask(15,77,1)
    self.tpx.setPixelConfig()

    self.mkdir(self.fname)
    self.tpx.setShutterLen(shutter_length)
    self.tpx.datadrivenReadout()
    self.tpx.resetTimer()
    self.tpx.t0Sync()
    self.tpx.setSenseDac(TPX3_VFBK)
    v_fbk=self.tpx.get_adc(32)
    self.tpx.setSenseDac(TPX3_VTHRESH_FINE)
    v_thr=self.tpx.get_adc(32)

    anim=['|','/','-','\\','|','/','-','\\']

    TH_START=1130
    TH_STOP=1210
    TH_STEP=500

    link_mask=int(pow(2,8)-1)
    self.tpx.setOutputMask(link_mask)
    self.tpx.datadrivenReadout()


    for frame in range(1):
     f1=open(self.fname+'/dd_data_%d.dat'%frame,"w")

     for thr in range(TH_START,TH_STOP,TH_STEP):
        event_counter=0
        pileup=0
        self.tpx.setThreshold(thr)
        v_thr=self.tpx.get_adc(8)
        self.tpx.resetTimer()
        self.tpx.t0Sync()
        self.tpx.openShutter(sleep=False)
        evn=np.zeros((256,256), int)
        evn_total=np.zeros((256,256), int)

        dd_packets=0
        finish=0
        ev_total=0
        pre_pck=0
        not_1hit=0
        event_counter=0
        while not finish:
          data=self.tpx.get_N_packets(1000)
          #print data
          for pck in data:
            if pck.isData():
#              print pck
#              print "%3d %3d %.9f"%(pck.col,pck.row,pck.abs_toa)
              #if pre_pck:
              #   if abs(pre_pck.col-pck.col)<4 and abs(pre_pck.row-pck.row)<4 and abs(pre_pck.abs_toa-pck.abs_toa)*1000000000<1000: 
#                   print "%3d %3d %.9f %3d %3d %.9f | %d %d %.9f"%(pck.col,pck.row,pck.abs_toa,pre_pck.col,pre_pck.row,pre_pck.abs_toa,abs(pre_pck.col-pck.col)\
#                                                                   ,abs(pre_pck.row-pck.row),abs(pre_pck.abs_toa-pck.abs_toa))
              #     not_1hit+=1
              #   else: pre_pck=pck
              #else: pre_pck=pck
              print "%c %s %d hits frame %d"%(13,anim[event_counter%len(anim)],event_counter, frame),
              line="%d\t%d\t%d\t%d\t%d\n"%(pck.col,pck.row,pck.toa,pck.tot,pck.ftoa)
              event_counter+=1
              sys.stdout.flush()
              f1.write(line)


              dd_packets+=1
              evn_total[pck.col][pck.row]+=1
            else:
              if pck.isEoR():
                finish=1
        f1.flush()

#        dd_shutter_time=0
#        while dd_shutter_time==0:
#           shutter_start=self.tpx.getShutterStart()
#           shutter_stop=self.tpx.getShutterEnd()
#           dd_shutter_time=(shutter_stop-shutter_start)*0.025
#        dd_rate=float(dd_packets)/dd_shutter_time

#        l="%d (%.3f Ke-) %9d %9d %11.5f  "%(thr,(v_fbk-v_thr)*20,dd_packets,not_1hit,dd_rate)
#        print l
#        f.write("%s\n"%(l))
#        f.flush()
        #f.write("\n")
     f1.close()   #self.save_np_array(evn_total, fn=self.fname+'/evn_dd_%d.map'%thr, info="  EVN to %s")

    if 0:
     for frame in range(1):
      f=open(self.fname+'/dd_data%d.dat'%frame,"w")
      f.close()
      event_counter=0
      pileup=0
      print frame
      self.SetThreshold(1130)
      self.tpx.openShutter()
      evn=np.zeros((256,256), int)
      pileup_m=np.zeros((256,256), int)
      frame_c=0
      max_pileup=0
      while 1:
#  def get_N_packets(self,N):
#  def get_N_raw(self,N):
        data=self.tpx.get_N_packets(1)
        temp=1#self.tpix3_temp(resolution=1)
        #print data
        for pck in data:
          #print pck.tot
          line="%d\t%d\t%d\t%d\t%d\t%d\n"%(event_counter,pck.col,pck.row,pck.toa,pck.tot,pck.ftoa)
          #print line
          event_counter+=1
          evn[pck.col][pck.row]+=1
          if pileup_m[pck.col][pck.row] < pck.ftoa:
              pileup_m[pck.col][pck.row]=pck.ftoa
              if max_pileup<pck.ftoa:
                   max_pileup=pck.ftoa
          if pck.ftoa>1:
             pileup+=1
          print "%c %s %d hits %d pileup %d"%(13,anim[event_counter%len(anim)],event_counter,pileup, max_pileup),
          sys.stdout.flush()
          f=open(self.fname+'/dd_data%d.dat'%frame,"a")
          f.write(line)
          f.close()
        #self.save_np_array(evn, fn=self.fname+'/evn_frame%d.map'%frame_c, info="  EVN to %s")
        #self.save_np_array(pileup_m, fn=self.fname+'/pileup_frame%d.map'%frame_c, info="  iTOT saved to %s")
        frame_c+=1



      num_pixels=0
      total_evnt=0
      total_itot=0

      for d in data:
        if d.type==0xB:
           print d
#           if evn[d.col][d.row]: print "!!"
#           num_pixels+=1
#           total_evnt+=d.event_counter
#           total_itot+=d.itot
#           evn[d.col][d.row]=d.event_counter
#           itot[d.col][d.row]=d.itot
        elif d.type!=0x7:
           self.warning_detailed("Unexpected packet %s"%str(d))
      print "Thr=%d pixels %d total count %d total itot %d"%(thr,num_pixels,total_evnt,total_itot)
      
#      self.save_np_array(evn, fn=self.fname+'/evn_'+str(thr)+'.map', info="  EVN to %s")
#      self.save_np_array(pileup_m, fn=self.fname+'/itot_'+str(thr)+'.map', info="  iTOT saved to %s")
      self.save_np_array(evn, fn=self.fname+'/evn_frame%d.map'%frame, info="  EVN to %s")
      self.save_np_array(pileup_m, fn=self.fname+'/pileup_frame%d.map'%frame, info="  iTOT saved to %s")

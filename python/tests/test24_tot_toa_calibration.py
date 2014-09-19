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



class test24_tot_toa_calibration(tpx3_test):
  """TOT and TOA calibration in data driven"""

  def _execute(self,**keywords):
   self.tpx.resetPixels()
   dac_defaults(self.tpx)

   START_ENERGY=500
   STOP_ENERGY=10000
   STEP_ENERGY=100

   START_IKRUM=5
   STOP_IKRUM=30
   STEP_IKRUM=100

   for ik in range (START_IKRUM,STOP_IKRUM,STEP_IKRUM):
    self.tpx.setDecodersEna(True)    
    self.tpx.setPllConfig( (TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK | TPX3_PHASESHIFT_DIV_8 | TPX3_PHASESHIFT_NR_1 | 0x14<<TPX3_PLLOUT_CONFIG_SHIFT) )
    self.tpx.setDac(TPX3_IBIAS_IKRUM,ik)
    self.tpx.setDac(TPX3_IBIAS_DISCS1_ON,100)
    self.tpx.setDac(TPX3_IBIAS_DISCS2_ON,128)
    self.tpx.setDac(TPX3_IBIAS_PREAMP_ON,128)
    self.tpx.setDac(TPX3_IBIAS_PIXELDAC,128)
    self.tpx.setDac(TPX3_VFBK,164)
    polarity=True
    genConfig_register=TPX3_ACQMODE_TOA_TOT | TPX3_GRAYCOUNT_ENA | TPX3_FASTLO_ENA 
    if not polarity: genConfig_register|=TPX3_POLARITY_EMIN
    shutter_length=100000
    self.tpx.setGenConfig( genConfig_register)
    self.tpx.setCtprBits(0)
    self.tpx.setCtpr()
    self.tpx.resetPixelConfig()

    eq_codes=numpy.loadtxt('calib/eq_codes_noiseFloor_ik5_w2l6_2.dat', dtype=int)
    eq_mask=numpy.loadtxt('calib/eq_mask_noiseFloor_ik5_w2l6_2.dat', dtype=int)

    fbase="calib/"
    eq_codes=numpy.loadtxt(fbase+"eq_codes_noiseFloor_ik15_w0c0.dat", int)
    eq_mask=numpy.loadtxt(fbase+     "eq_mask_noiseFloor_ik15_w0c0.dat", int)

    self.tpx.resetPixelConfig()
#    self.tpx.load_equalization('calib/eq_codes_noiseFloor_ik1.dat',\
#                      maskname='calib/eq_mask_noiseFloor_ik1.dat')
#    for x in range(256):
#      for y in range(256):
#        self.tpx.setPixelTestEna(x,y, testbit=True)
#    self.tpx.setPixelConfig()

    link_mask=int(pow(2,8)-1)
    self.tpx.setOutputMask(link_mask)

    self.mkdir(self.fname)
    self.tpx.setShutterLen(shutter_length)

    anim=['|','/','-','\\','|','/','-','\\']

    TESTPULSES=100
    testpulses=True

    diagonal_tot={}
    diagonal_tot_noise={}

    for Energy_e in range(START_ENERGY,STOP_ENERGY,STEP_ENERGY):
     pos=int((Energy_e-START_ENERGY)/STEP_ENERGY)
     diagonal_tot[pos]={}
     diagonal_tot_noise[pos]={}

     if testpulses:   
        period=32
        self.tpx.setTpPeriodPhase(period,8)
        self.tpx.setTpNumber(TESTPULSES)
        pulse_period=2*(64*period+1)
        shutter_length=int(((pulse_period*TESTPULSES)/40) + 100)
        self.tpx.setShutterLen(shutter_length)
        self.tpx.setDac(TPX3_VTP_COARSE,64)
        self.tpx.setSenseDac(TPX3_VTP_COARSE)
        coarse=self.tpx.get_adc(32)
        self.tpx.setSenseDac(TPX3_VTP_FINE)
        time.sleep(0.001)
        best_vtpfine_diff=1e3
        best_vtpfine_code=0
        target=Energy_e
        for code in range(64,512):
          self.tpx.setDac(TPX3_VTP_FINE,code) # (0e-) slope 44.5e/LSB -> (112=1000e-)  (135=2000e-)
          time.sleep(0.001)
          fine=self.tpx.get_adc(8)
          electrons=1000.0*20*(fine-coarse)
          if abs(electrons-target)<abs(best_vtpfine_diff) and electrons>0:
              best_vtpfine_diff=electrons-target
              best_vtpfine_code=code
        self.tpx.setDac(TPX3_VTP_FINE,best_vtpfine_code) # (0e-) slope 44.5e/LSB -> (112=1000e-)  (135=2000e-)
        time.sleep(0.001)
        fine=self.tpx.get_adc(32)
        self.logging.info( "  TPX3_VTP_FINE code=%d %.2f v_fine=%.1f v_coarse=%.1f TP=%de-"\
                            %(best_vtpfine_code,best_vtpfine_diff,fine*1000.0,coarse*1000.0,target))
        self.tpx.setGenConfig(genConfig_register)
        spacing=2
     self.logging.info("Shutter length %d us"%shutter_length)
#    self.tpx.resetPixelConfig()
#    self.tpx.setPixelMask(ALL_PIXELS,ALL_PIXELS,1)
     self.tpx.setCtprBits(0)
     for x in range(256):
      for y in range(256):
#        self.tpx.setPixelTestEna(x,y, testbit=False)
#        self.tpx.setPixelThreshold(x,y,eq_codes[y][x])
#        if x%spacing==int(seq/spacing) and y%spacing==seq%spacing:
        if x==128 or y==128:
#        if x==128 and y==128:
#            self.tpx.setPixelMask(x,y,0)
            if (testpulses):
                self.tpx.setPixelTestEna(x,y, testbit=True)
                self.tpx.setCtprBit(x,1)
#    self.tpx.pauseReadout()
#    self.tpx.setPixelConfig()
     if testpulses:
      self.tpx.setCtpr()

     self.tpx.datadrivenReadout()



#    f1=open(self.fname+'/dd_data.dat',"w")
#    f=open(self.fname+'/Ths_Scan_dd.dat',"w")

     evn_cnt=np.zeros((256,256), int)
     tot_cnt=np.zeros((256,256), float)
     toa_cnt=np.zeros((256,256), float)
     tot_cnt2=np.zeros((256,256), float)
     toa_cnt2=np.zeros((256,256), float)
     stdv_toa=np.zeros((256,256), float)
     stdv_tot=np.zeros((256,256), float)
     toa_ref=np.zeros((256,256), float)
     resolution_tot=np.zeros((256,256), float)

     spacing=8
     seq_total=spacing*spacing/2
     seq_total=1
     for seq in range(seq_total):
      logging.info("  seq %0d/%d"%(seq,seq_total))

#      if ik==START_IKRUM and Energy_e==START_ENERGY:
      if 1:
          self.tpx.setPixelMask(ALL_PIXELS,ALL_PIXELS,1)
          self.tpx.setPixelThreshold(ALL_PIXELS,ALL_PIXELS,8)
          self.tpx.setCtprBits(0)
          for x in range(256):
              for y in range(256):
                self.tpx.setPixelTestEna(x,y, testbit=False)
#                if x==y:# and y==129:
                if y%spacing==int(seq/(spacing/2)) and x%(spacing/2)==seq%(spacing/2) and eq_mask[y][x]==0:
                        self.tpx.setPixelThreshold(x,y,eq_codes[y][x])
                        self.tpx.setPixelMask(x,y,eq_mask[y][x])
                        self.tpx.setPixelTestEna(x,y, testbit=True)
                        self.tpx.setCtprBit(x,1)
                        #if x<32 and y<32:
                        #  print seq,x,y

          self.tpx.pauseReadout()
          self.tpx.setPixelConfig()
          self.tpx.setCtpr()
#      f1=open(self.fname+'/dd_data_%d_%d.dat'%(Energy_e,seq),"w")
      self.tpx.datadrivenReadout()
      for frame in (1,2):
        if frame==1:
            genConfig_register=TPX3_ACQMODE_TOA_TOT | TPX3_TESTPULSE_ENA |TPX3_SELECTTP_DIGITAL | TPX3_GRAYCOUNT_ENA | TPX3_FASTLO_ENA 
            self.tpx.setTpNumber(1)
#            print"Digital Reference %d"%(seq)
        else:
            genConfig_register=TPX3_ACQMODE_TOA_TOT | TPX3_TESTPULSE_ENA | TPX3_GRAYCOUNT_ENA | TPX3_FASTLO_ENA
            self.tpx.setTpNumber(TESTPULSES)
#            print"Analog Measurement %d"%(seq)

        self.tpx.setGenConfig( genConfig_register)
        event_counter=0
        pileup=0
        self.tpx.setThreshold(1130)
        v_thr=self.tpx.get_adc(8)
        self.tpx.resetTimer()
        self.tpx.t0Sync()
        self.tpx.openShutter(sleep=False)
        shutter_start=self.tpx.getShutterStart()
        dd_packets=0
        finish=0
        ev_total=0
        pre_pck=0
        not_1hit=0
        bad_tot=0
        event_counter=0
        while not finish:
          data=self.tpx.get_N_packets(1024)
#         print data
          for pck in data:
            if pck.isData():
                hitTime=(pck.toa-shutter_start)&0x0003fff
                hitTimef=float(hitTime*25-float(pck.ftoa)*25/16)
                if frame==1:
                    toa_ref[pck.col][pck.row]=hitTimef
                timewalk=hitTimef-(toa_ref[pck.col][pck.row]+25*((pulse_period*(evn_cnt[pck.col][pck.row]))%16384))
#                    line="%d\t%d\t%d\t%d\t%d\t%d\t%d\t%3.4f\t%3.4f\t%d"\
#                        %(event_counter,pck.col,pck.row,pck.toa,pck.tot,pck.ftoa,shutter_start,hitTimef,timewalk,evn_cnt[pck.col][pck.row])
#                    print line
                if timewalk<300 and timewalk>0 and frame>1:
#                if frame>1:
                    tot_cnt[pck.col][pck.row]+=pck.tot
                    toa_cnt[pck.col][pck.row]+=timewalk
                    tot_cnt2[pck.col][pck.row]+=pow(pck.tot,2)
                    toa_cnt2[pck.col][pck.row]+=pow(timewalk,2)
                    evn_cnt[pck.col][pck.row]+=1
                    stdv_tot[pck.col][pck.row]=pow(tot_cnt2[pck.col][pck.row]/evn_cnt[pck.col][pck.row]-pow(tot_cnt[pck.col][pck.row]/evn_cnt[pck.col][pck.row],2),0.5)
                    stdv_toa[pck.col][pck.row]=pow(toa_cnt2[pck.col][pck.row]/evn_cnt[pck.col][pck.row]-pow(toa_cnt[pck.col][pck.row]/evn_cnt[pck.col][pck.row],2),0.5)
                    average_tot=tot_cnt[pck.col][pck.row]/evn_cnt[pck.col][pck.row]
                    resolution_e=stdv_tot[pck.col][pck.row]*Energy_e/average_tot
                    sys.stdout.flush()

                    if 0:
                        line="%d\t%d\t%d\n"%(pck.col,pck.row,pck.tot)
                        f1.write(line)
                        f1.flush()

                    if 0:
                      if pck.col==128:
                        line="%d\t%d\t%d\t%d\t%3.2f\t%3.4f\t%3.2f\t%3.4f\t%3.4f"\
                          %(evn_cnt[pck.col][pck.row],pck.col,pck.row,pck.tot,average_tot,stdv_tot[pck.col][pck.row],resolution_e,timewalk,stdv_toa[pck.col][pck.row])
                        print line
            else:
              if pck.isEoR():
                finish=1
        if frame>1:
            pixels_hit=0
            pixel_counter=0
            for x in range(256):
               for y in range(256):
                    if evn_cnt[x][y]>0 :
                        pixels_hit+=1;
                        pixel_counter+=evn_cnt[x][y]
            if pixels_hit>0:
                print"pixels %d average %3.2f"%(pixels_hit,float(pixel_counter/pixels_hit))
            else:
                print"pixels %d"%(pixels_hit)
        if frame==2:# and seq==1:
            print ik,Energy_e
            diagonal_tot[pos][0]=Energy_e
            diagonal_tot_noise[pos][0]=Energy_e
            for x in range(256):
                y=128
                if evn_cnt[x][y]:   diagonal_tot[pos][x+1]=tot_cnt[x][y]/evn_cnt[x][y]
                else:               diagonal_tot[pos][x+1]=0
                diagonal_tot_noise[pos][x+1]=stdv_tot[x][y]
#      f1.close()
     for x in range(256):
        for y in range(256):
            if evn_cnt[x][y]>0 :
                tot_cnt[x][y]=tot_cnt[x][y]/evn_cnt[x][y]
                toa_cnt[x][y]=toa_cnt[x][y]/evn_cnt[x][y]
                resolution_tot[x][y]=stdv_tot[x][y]*Energy_e/tot_cnt[x][y]
     print"Bad TOT pixels %d"%(bad_tot)
     if 1:

         self.save_np_array(evn_cnt, fn=self.fname+'/PC_%d.map'%Energy_e,fmt="%d")
         self.save_np_array(tot_cnt, fn=self.fname+'/TOT_%d.map'%Energy_e,fmt="%.3f")
         self.save_np_array(toa_cnt, fn=self.fname+'/TOA_%d.map'%Energy_e,fmt="%.3f")
         self.save_np_array(stdv_tot, fn=self.fname+'/Stdev_TOT_%d.map'%Energy_e,fmt="%.3f")
         self.save_np_array(stdv_toa, fn=self.fname+'/Stdev_TOA_%d.map'%Energy_e,fmt="%.3f")
         self.save_np_array(resolution_tot, fn=self.fname+'/Resolution_TOT_%d.map'%Energy_e,fmt="%.3f")
         self.save_np_array(toa_ref, fn=self.fname+'/TOA_ref_%d.map'%Energy_e,fmt="%.3f")

    dac_code_ikrum=self.tpx.getDac(TPX3_IBIAS_IKRUM)
    f=open(self.fname+"/TOT_IK%d.dat"%(dac_code_ikrum),"w")
    f1=open(self.fname+"/TOT_noise_IK%d.dat"%(dac_code_ikrum),"w")
    for Energy_e in range(START_ENERGY,STOP_ENERGY,STEP_ENERGY): 
      pos=int((Energy_e-START_ENERGY)/STEP_ENERGY)
      line=""
      line2=""
      for x in range (257):
        line+="%.3f "%(diagonal_tot[pos][x])
        line2+="%.3f "%(diagonal_tot_noise[pos][x])
#      print line
      f.write(line+"\n")
      f1.write(line2+"\n")
    f.close()
    f1.close()















from tpx3_test import *
import os
import random
import time
import logging
from SpidrTpx3_engine import *
import numpy
from scipy.optimize import curve_fit
import matplotlib.pyplot as plt
import pycallgraph

# Define model function to be used to fit to the data above:
def gauss(x, *p):
    A, mu, sigma = p
    return A*numpy.exp(-(x-mu)**2/(2.*sigma**2))

def mkdir(d):
  if not os.path.exists(d):
    os.makedirs(d)  
    
class test08_th_scan_noise(tpx3_test):
  """Threshold scan over noise"""

  def _execute(self):
   pycallgraph.start_trace()
   self.tpx.setDac(TPX3_VTP_COARSE,50)
   self.tpx.setDac(TPX3_VTP_FINE,90) # (0e-) slope 44.5e/LSB -> (112=1000e-)  (135=2000e-)
   self.tpx.setDac(TPX3_VTHRESH_COARSE,7) 
   self.tpx.setDac(TPX3_VFBK,143) 
   self.tpx.setGenConfig(0x208|4)
   mkdir(self.fname)

#    mod.add_enum('DAC_code', ['TPX3_IBIAS_PREAMP_ON','TPX3_IBIAS_PREAMP_OFF','TPX3_VPREAMP_NCAS','TPX3_IBIAS_IKRUM','TPX3_VFBK',
#                          'TPX3_VTHRESH_FINE','TPX3_VTHRESH_COARSE','TPX3_IBIAS_DISCS1_ON','TPX3_IBIAS_DISCS1_OFF',
#                          'TPX3_IBIAS_DISCS2_ON','TPX3_IBIAS_DISCS2_OFF','TPX3_IBIAS_PIXELDAC','TPX3_IBIAS_TPBUFIN',
#                          'TPX3_IBIAS_TPBUFOUT','TPX3_VTP_COARSE','TPX3_VTP_FINE','TPX3_IBIAS_CP_PLL'])
   for cdac in range(16):
    res={}
    
    for seq in range(1):
      self.tpx.resetPixelConfig()
#      self.tpx.maskPixel(ALL_PIXELS,ALL_PIXELS)
      for x in range(256):
        for y in range(256):
#          if x%2==seq/4 and y%4==seq%4:
#            self.tpx.unmaskPixel(x,y)
            self.tpx.configPixel(x,y,cdac)
      self.tpx.setPixelConfig()
    
      self.tpx.sequentialReadout()
    
      for i in range(0,512):
        self.tpx.setDac(TPX3_VTHRESH_FINE,i)
        self.tpx.openShutter(500)
      
        self.tpx.sequentialReadout()
        data=self.tpx.recv_mask(0xAF71,0xFFFF)
        for d in data:
          if d.type!=0xA:continue
          if not d.col in res:
            res[d.col]={}
          if not d.row in res[d.col]:
            res[d.col][d.row]={}
          res[d.col][d.row][i]=d.cnt


    if 0:
      self.tpx.resetPixelConfig()
      self.tpx.maskPixel(ALL_PIXELS,ALL_PIXELS)
      for x in range(256):
         self.tpx.unmaskPixel(x,x)
      self.tpx.setPixelConfig()
    
      self.tpx.sequentialReadout()
    
      for i in range(0,512):
        self.tpx.setDac(TPX3_VTHRESH_FINE,i)
        self.tpx.openShutter(100)
      
        self.tpx.sequentialReadout()
        data=self.tpx.recv_mask(0xAF71,0xFFFF)
        for d in data:
          if d.type!=0xA:continue
          if not d.col in res:
            res[d.col]={}
          if not d.row in res[d.col]:
            res[d.col][d.row]={}
          res[d.col][d.row][i]=d.cnt

    if 1: #fitting
      fit_res={}
      for col in range(256):
        fit_res[col]={}
        for row in range(256):
          if col in res and row in res[col]:
            print col,row
#            fn=self.fname+"_%d_%d.dat"%(col,row)
#            f=open(fn,"w")
            codes=[]
            vals=[]
            avr=0.0
            N=0
            for code in res[col][row]:
               val=res[col][row][code]
               if val>1:
#                 f.write("%d %d\n"%(code,val))
                 codes.append(code)
                 vals.append(val)
                 avr+=code*val
                 N+=val
#            f.close()
            if N>0:
              avr=avr/N

#            print codes
#            print vals
#            print avr
              try:
                hist, bin_edges = numpy.histogram(codes,bins=int(max(codes)-min(codes)+1),range=(-0.5+min(codes), 0.5+max(codes)), weights=vals)
                bin_centres = (bin_edges[:-1] + bin_edges[1:])/2
                # p0 is the initial guess for the fitting coefficients (A, mu and sigma above)
                p0 = [max(vals), avr, 5.]
                coeff, var_matrix = curve_fit(gauss, bin_centres, hist, p0=p0)
              except:
                coeff=[0.0,0.0,0.0]
             
              fit_res[col][row] =coeff
            else:
              fit_res[col][row] =[0.0,0.0,0.0]
              print vals


      fn=(self.fname+"/dacs%X_bl.dat"%cdac,self.fname+"/dacs%X_rms.dat"%cdac)
      f=(open(fn[0],"w"),open(fn[1],"w"))
      for row in range(256):
        for col in range(256):
          for i in range(2):
            if col in fit_res and row in fit_res[col]:
              f[i].write("%.3f "%fit_res[col][row][1+i])
            else:
              f[i].write("0 ")
        for i in range(2):
          f[i].write("\n")
      f[0].close()
      f[1].close()
   pycallgraph.make_dot_graph('test.png')

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
  
class test08_eq_scan(tpx3_test):
  """Threshold scan over noise"""

  def _execute(self):
  
   pycallgraph.start_trace()
   self.tpx.setDac(TPX3_VTP_COARSE,50)
   self.tpx.setDac(TPX3_VTP_FINE,90) # (0e-) slope 44.5e/LSB -> (112=1000e-)  (135=2000e-)
   self.tpx.setDac(TPX3_VTHRESH_COARSE,7) 
   self.tpx.setDac(TPX3_VFBK,143) 
   self.tpx.setGenConfig(0x208|4)
   mkdir(self.fname)

   eq=load('logs/thscan1/test08_th_scan_noise/eq.codes')
   self.tpx.resetPixelConfig()
   for x in range(256):
      for y in range(256):
          self.tpx.configPixel(x,y,eq[y][x])
   self.tpx.setPixelConfig()
    
   self.tpx.sequentialReadout()
   f=open(self.fname+".rate","w")
   res={}
   for i in range(0,350):
        self.tpx.setDac(TPX3_VTHRESH_FINE,i)
        self.tpx.openShutter(500)
      
        self.tpx.sequentialReadout()
        data=self.tpx.recv_mask(0xAF71,0xFFFF)
        pixels=0
        for d in data:
          if d.type==0xA:pixels+=1
        f.write("%d %d\n"% (i,pixels))
        print i,pixels
        f.flush()
        for d in data:
          if d.type!=0xA:continue
          if not d.col in res:
            res[d.col]={}
          if not d.row in res[d.col]:
            res[d.col][d.row]={}
          res[d.col][d.row][i]=d.cnt
   f.close()
   
   if 1: #fitting
      fit_res={}
      for col in range(256):
        fit_res[col]={}
        for row in range(256):
          if col in res and row in res[col]:
            print col,row
#            fn=self.fname+"_%d_%d.dat"%(col,row)
#            f=open(fn,"w")
            codes=[]
            vals=[]
            avr=0.0
            N=0
            for code in res[col][row]:
               val=res[col][row][code]
               if val>1:
#                 f.write("%d %d\n"%(code,val))
                 codes.append(code)
                 vals.append(val)
                 avr+=code*val
                 N+=val
#            f.close()
            if N>0:
              avr=avr/N

#            print codes
#            print vals
#            print avr
              try:
                hist, bin_edges = numpy.histogram(codes,bins=int(max(codes)-min(codes)+1),range=(-0.5+min(codes), 0.5+max(codes)), weights=vals)
                bin_centres = (bin_edges[:-1] + bin_edges[1:])/2
                # p0 is the initial guess for the fitting coefficients (A, mu and sigma above)
                p0 = [max(vals), avr, 5.]
                coeff, var_matrix = curve_fit(gauss, bin_centres, hist, p0=p0)
              except:
                coeff=[0.0,0.0,0.0]
             
              fit_res[col][row] =coeff
            else:
              fit_res[col][row] =[0.0,0.0,0.0]
              print vals


      fn=(self.fname+"/bl.dat",self.fname+"/rms.dat")
      f=(open(fn[0],"w"),open(fn[1],"w"))
      for row in range(256):
        for col in range(256):
          for i in range(2):
            if col in fit_res and row in fit_res[col]:
              f[i].write("%.3f "%fit_res[col][row][1+i])
            else:
              f[i].write("0 ")
        for i in range(2):
          f[i].write("\n")
      f[0].close()
      f[1].close()

   pycallgraph.make_dot_graph('test.png')

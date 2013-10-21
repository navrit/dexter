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

class test08_equalization(tpx3_test):
  """Threshold scan over noise"""

  def threshold_scan(self,res=None):
      peak=numpy.zeros((256,256), int)
      if res==None:
        res={}
        for x in range(256):
          res[x]={}
          for y in range(256):
            res[x][y]={}

  
      mask=0
      for i in range(0,512,1):
        self.tpx.setDac(TPX3_VTHRESH_FINE,i)
        self.tpx.openShutter(500)
#        time.sleep(0.001)
        self.tpx.sequentialReadout()
        data=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)
        logging.info("Packets received %d (to be masked %d)"%(len(data),mask))
        for d in data:
          if d.type==0xA:
            res[d.col][d.row][i]=d.cnt
            
            if  d.cnt>=200 and peak[d.col][d.row]==0: 
              peak[d.col][d.row]=1
            elif d.cnt<=2 and peak[d.col][d.row]==1:
              self.tpx.maskPixel(d.col,d.row)
              mask+=1
              peak[d.col][d.row]=2
          elif d.type!=0x7:
            logging.warning("Unexpected packet %s"%str(d))

        if  mask>8000:
          logging.info("Masking %d pixels"%mask)
          self.tpx.flushFifoIn()
          self.tpx.setPixelConfig()
          mask=0

      return res

  def _execute(self):
    self.tpx.resetPixels()
    self.tpx.setDacsDflt()
    self.tpx.setDac(TPX3_IBIAS_IKRUM,15)
    self.tpx.setDac(TPX3_VTP_COARSE,50)
    self.tpx.setDac(TPX3_VTP_FINE,112) # (0e-) slope 44.5e/LSB -> (112=1000e-)  (135=2000e-)
    self.tpx.setDac(TPX3_VTHRESH_COARSE,7) 
    self.tpx.setDac(TPX3_VFBK,143) 
    self.tpx.setDac(TPX3_IBIAS_PREAMP_ON,150)
    self.tpx.setDac(TPX3_IBIAS_DISCS1_ON,100)

#    self.tpx.setDac(TPX3_IBIAS_IKRUM,128)
#    self.tpx.setDac(TPX3_IBIAS_PREAMP_ON,128)
#    self.tpx.setDac(TPX3_IBIAS_DISCS1_ON,128)


    self.tpx.setGenConfig(0x04)
    self.tpx.setPllConfig(0x291E) 
#    self.tpx.setPllConfig(0x0E) 

    for c in range(256):
        self.tpx.configCtpr(c,0)
    self.tpx.setCtpr()


    mkdir(self.fname)
    self.tpx.flush_udp_fifo()

    

    for cdac in range(0,16,1):
      logdir=self.fname+"/0x%0X/"%cdac
      mkdir(logdir)

      res={}
      for x in range(256):
        res[x]={}
        for y in range(256):
          res[x][y]={}

      for seq in range(4):
        self.tpx.resetPixelConfig()
        self.tpx.maskPixel(ALL_PIXELS,ALL_PIXELS)
        on=0
        for x in range(256):
          for y in range(256):
            if x%2==int(seq/2) and y%2==seq%2:
              self.tpx.configPixel(x,y,cdac)
              self.tpx.unmaskPixel(x,y)

#            else:
#              self.tpx.configPixel(x,y,0)
#              self.tpx.maskPixel(x,y)
        self.tpx.setPixelConfig()
        res=self.threshold_scan(res)

      if 1: #fitting
        w2f=False
        fit_res={}
        for col in range(256):
          fit_res[col]={}
          if w2f:mkdir(logdir+"/%03d"%col)
          logging.info("Fitting col %d"%col)
          for row in range(256):
            if col in res and row in res[col]:
              if w2f:fn=logdir+"/%03d/%03d_%03d.dat"%(col,col,row)
              if w2f:f=open(fn,"w")
              codes=[]
              vals=[]
              avr=0.0
              N=0
              for code in res[col][row]:
                 val=res[col][row][code]
                 if w2f and val>0:
                   f.write("%d %d\n"%(code,val))
                 if val>2:
                   codes.append(code)
                   vals.append(val)
                   avr+=code*val
                   N+=val
              if N>2:
                avr=avr/N
                try:
                  hist, bin_edges = numpy.histogram(codes,bins=int(max(codes)-min(codes)+1),range=(-0.5+min(codes), 0.5+max(codes)), weights=vals)
                  bin_centres = (bin_edges[:-1] + bin_edges[1:])/2
                  # p0 is the initial guess for the fitting coefficients (A, mu and sigma above)
                  p0 = [max(vals), avr, 8.]
                  coeff, var_matrix = curve_fit(gauss, bin_centres, hist, p0=p0)
                except:
                  coeff=[0.0,0.0,0.0]
                fit_res[col][row] =coeff
              else:
                fit_res[col][row] =[0.0,0.0,0.0]
                logging.warning("No hits for pixel (%d,%d)"%(col,row))

              if w2f:
                f.write("# %.3f %.3f\n"%(fit_res[col][row][1],fit_res[col][row][2]))
                f.close()

        fn=(self.fname+"/dacs%X_bl.dat"%cdac,self.fname+"/dacs%X_rms.dat"%cdac)
        f=(open(fn[0],"w"),open(fn[1],"w"))
        for row in range(256):
          for col in range(256):
            for i in range(2):
              if col in fit_res and row in fit_res[col]:
                f[i].write("%.3f "%abs(fit_res[col][row][1+i]))
              else:
                f[i].write("0 ")
          for i in range(2):
            f[i].write("\n")
        f[0].close()
        f[1].close()




    if 1:#equalize
      low,la=load(self.fname+'/dacs0_bl.dat')
      high,ha=load(self.fname+'/dacsF_bl.dat')
  
      target=(la+ha)/2.0
      print la,ha,target
  
      f=open(self.fname+"/eq.codes","w")
      for y in range(256):
        for x in range(256):
          l=low[x][y]
          h=high[x][y]
          if target<l:
            code=0
          elif target>h:
            code=0xF
          else:
            dx=target-l
            fs=h-l
            code=int(dx/fs*15 +0.5)
          f.write("%2d "%code)
        f.write("\n")

#    if 1: # equalization
#      hsts=[]
#      rmss=[]
#      MC=16
#      maps=[]
#      for dc in range(MC):
#        try:
#          data,mmin,mmax=load('dacs%X_bl.dat'%dc)
#          maps.append(data)
#          hst=[0]*512
#          for x in range(256):
#            for y in range(56):
#              i=int(data[x][y])
#              hst[i]+=1
#            hsts.append(hst)

#          data,mmin,mmax=load('dacs%X_rms.dat'%dc)
#          hst=[0]*1024
#          for x in range(256):
#            for y in range(56):
#              i=int(100.0*data[x][y])
#              hst[i]+=1
#          rmss.append(hst)
#        except:
#          pass
#    
#    


class test08_equalization_test(tpx3_test):
  """Threshold scan over noise"""

  def threshold_scan(self,res=None):
      peak=numpy.zeros((256,256), int)
      if res==None:
        res={}
        for x in range(256):
          res[x]={}
          for y in range(256):
            res[x][y]={}

  
      mask=0
      for i in range(150,450,1):
        self.tpx.setDac(TPX3_VTHRESH_FINE,i)
        self.tpx.openShutter(500)
#        time.sleep(0.001)
        self.tpx.sequentialReadout()
        data=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)
        logging.info("Packets received %d (to be masked %d)"%(len(data),mask))
        for d in data:
          if d.type==0xA:
            res[d.col][d.row][i]=d.cnt
            
            if  d.cnt>=200 and peak[d.col][d.row]==0: 
              peak[d.col][d.row]=1
            elif d.cnt<=2 and peak[d.col][d.row]==1:
              self.tpx.maskPixel(d.col,d.row)
              mask+=1
              peak[d.col][d.row]=2
          elif d.type!=0x7:
            logging.warning("Unexpected packet %s"%str(d))

        if  mask>1200:
          logging.info("Masking %d pixels"%mask)
          self.tpx.flushFifoIn()
          self.tpx.setPixelConfig()
          mask=0

      return res

  def _execute(self):
    self.tpx.resetPixels()
    self.tpx.setDacsDflt()
    self.tpx.setDac(TPX3_IBIAS_IKRUM,15)
    self.tpx.setDac(TPX3_VTP_COARSE,50)
    self.tpx.setDac(TPX3_VTP_FINE,112) # (0e-) slope 44.5e/LSB -> (112=1000e-)  (135=2000e-)
    self.tpx.setDac(TPX3_VTHRESH_COARSE,7) 
    self.tpx.setDac(TPX3_VFBK,143) 
    self.tpx.setDac(TPX3_IBIAS_PREAMP_ON,150)
    self.tpx.setDac(TPX3_IBIAS_DISCS1_ON,100)

    self.tpx.setGenConfig(0x04)
    self.tpx.setPllConfig(0x291E) 

    for c in range(256):
        self.tpx.configCtpr(c,0)
    self.tpx.setCtpr()


    mkdir(self.fname)
    self.tpx.flush_udp_fifo()

    
    if 1:
      res={}
      for x in range(256):
        res[x]={}
        for y in range(256):
          res[x][y]={}

      for seq in range(16):
        self.tpx.resetPixelConfig()
        self.tpx.load_equalization("logs/F3_default_eq_bruteforce/test08_equalization/eq_codes.dat")
        self.tpx.maskPixel(ALL_PIXELS,ALL_PIXELS)
        on=0
        for x in range(256):
          for y in range(256):
            if x%4==int(seq/4) and y%4==seq%4:
              self.tpx.unmaskPixel(x,y)
        self.tpx.setPixelConfig()
        res=self.threshold_scan(res)

      if 1: #fitting
        w2f=False
        fit_res={}
        for col in range(256):
          fit_res[col]={}
          if w2f:mkdir(logdir+"/%03d"%col)
          logging.info("Fitting col %d"%col)
          for row in range(256):
            if col in res and row in res[col]:
              if w2f:fn=logdir+"/%03d/%03d_%03d.dat"%(col,col,row)
              if w2f:f=open(fn,"w")
              codes=[]
              vals=[]
              avr=0.0
              N=0
              for code in res[col][row]:
                 val=res[col][row][code]
                 if w2f and val>0:
                   f.write("%d %d\n"%(code,val))
                 if val>2:
                   codes.append(code)
                   vals.append(val)
                   avr+=code*val
                   N+=val
              if N>2:
                avr=avr/N
                try:
                  hist, bin_edges = numpy.histogram(codes,bins=int(max(codes)-min(codes)+1),range=(-0.5+min(codes), 0.5+max(codes)), weights=vals)
                  bin_centres = (bin_edges[:-1] + bin_edges[1:])/2
                  # p0 is the initial guess for the fitting coefficients (A, mu and sigma above)
                  p0 = [max(vals), avr, 8.]
                  coeff, var_matrix = curve_fit(gauss, bin_centres, hist, p0=p0)
                except:
                  coeff=[0.0,0.0,0.0]
                fit_res[col][row] =coeff
              else:
                fit_res[col][row] =[0.0,0.0,0.0]
                logging.warning("No hits for pixel (%d,%d)"%(col,row))

              if w2f:
                f.write("# %.3f %.3f\n"%(fit_res[col][row][1],fit_res[col][row][2]))
                f.close()

        fn=(self.fname+"/bl.dat",self.fname+"/rms.dat")
        f=(open(fn[0],"w"),open(fn[1],"w"))
        for row in range(256):
          for col in range(256):
            for i in range(2):
              if col in fit_res and row in fit_res[col]:
                f[i].write("%.3f "%abs(fit_res[col][row][1+i]))
              else:
                f[i].write("0 ")
          for i in range(2):
            f[i].write("\n")
        f[0].close()
        f[1].close()



class test08_equalization_hitrate_seqread(tpx3_test):
  """Threshold scan over noise and measture hit rate (sequential readout)"""

  def _execute(self):
    self.tpx.resetPixels()
    self.tpx.setDacsDflt()
    self.tpx.setDac(TPX3_IBIAS_IKRUM,15)
    self.tpx.setDac(TPX3_VTP_COARSE,50)
    self.tpx.setDac(TPX3_VTP_FINE,112) # (0e-) slope 44.5e/LSB -> (112=1000e-)  (135=2000e-)
    self.tpx.setDac(TPX3_VTHRESH_COARSE,7) 
    self.tpx.setDac(TPX3_VFBK,143) 
    self.tpx.setDac(TPX3_IBIAS_PREAMP_ON,150)
    self.tpx.setDac(TPX3_IBIAS_DISCS1_ON,100)

    self.tpx.setGenConfig(0x04)
    self.tpx.setPllConfig(0x291E) 
    self.tpx.getGenConfig()

    for c in range(256):
        self.tpx.configCtpr(c,0)
    self.tpx.setCtpr()


    mkdir(self.fname)
    self.tpx.flush_udp_fifo()

    
    res={}
    self.tpx.resetPixelConfig()
    self.tpx.load_equalization("logs/F3_default_eq_bruteforce/test08_equalization/eq_codes.dat")
    to_be_masked=[]# [(23L, 69L), (49L, 94L), (50L, 94L), (89L, 46L), (90L, 46L), (105L, 139L), (106L, 139L), (107L, 138L), (112L, 5L), (115L, 196L), (116L, 196L), (135L, 122L), (136L, 122L), (175L, 7L), (176L, 7L), (205L, 27L), (207L, 51L), (247L, 218L)]
    for x,y in to_be_masked:
       self.tpx.maskPixel(x,y)
    logging.info("Pixels masked %d)"%(len(to_be_masked)))
    self.tpx.setPixelConfig()
    
    to_be_masked=[]
    mask=0
    for i in range(0,350,1):
        self.tpx.setDac(TPX3_VTHRESH_FINE,i)
        self.tpx.openShutter(500)
        self.tpx.sequentialReadout()
        data=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)
        pixels=0
        for d in data:
          if d.type==0xA:
             pixels+=1
             if i==247:
                   to_be_masked.append( (d.col,d.row) )

          elif d.type!=0x7:
             logging.warning("Unexpected packet %s"%str(d))
        logging.info("Packets received %d (pixels %d)"%(len(data),pixels))
        res[i]=pixels
    f=open(self.fname+"/rate.dat","w")
    for x in sorted(res):
      f.write("%d %d\n"%(x,res[x]))
    f.close()
    print to_be_masked
    
    

class test08_equalization_hitrate_datadriven(tpx3_test):
  """Threshold scan over noise and measture hit rate (data driven)"""

  def _execute(self):
    self.tpx.resetPixels()
    self.tpx.setDacsDflt()
    self.tpx.setDac(TPX3_IBIAS_IKRUM,15)
    self.tpx.setDac(TPX3_VTP_COARSE,50)
    self.tpx.setDac(TPX3_VTP_FINE,112) # (0e-) slope 44.5e/LSB -> (112=1000e-)  (135=2000e-)
    self.tpx.setDac(TPX3_VTHRESH_COARSE,7) 
    self.tpx.setDac(TPX3_VFBK,143) 
    self.tpx.setDac(TPX3_IBIAS_PREAMP_ON,150)
    self.tpx.setDac(TPX3_IBIAS_DISCS1_ON,100)

    self.tpx.setGenConfig(0x00)
    self.tpx.setPllConfig(0x291E) 

    for c in range(256):
        self.tpx.configCtpr(c,0)
    self.tpx.setCtpr()


    mkdir(self.fname)
    self.tpx.flush_udp_fifo()
    self.tpx.getGenConfig()

    
    res={}
    self.tpx.resetPixelConfig()
    self.tpx.load_equalization("logs/F3_default_eq_bruteforce/test08_equalization/eq_codes.dat")
    to_be_masked= [(23L, 69L), (49L, 94L), (50L, 94L), (89L, 46L), (90L, 46L), (105L, 139L), (106L, 139L), (107L, 138L), (112L, 5L), (115L, 196L), (116L, 196L), (135L, 122L), (136L, 122L), (175L, 7L), (176L, 7L), (205L, 27L), (207L, 51L), (247L, 218L)]
    for x,y in to_be_masked:
       self.tpx.maskPixel(x,y)
    logging.info("Pixels masked %d)"%(len(to_be_masked)))
    self.tpx.setPixelConfig()
    
    to_be_masked=[]
    mask=0
    self.tpx.datadrivenReadout()

    for i in range(0,350,1):
        self.tpx.setDac(TPX3_VTHRESH_FINE,i)
        self.tpx.openShutter(500)
        data=self.tpx.recv_mask(0x71B0000000000000, 0xFFFF000000000000)
        pixels={}
        for d in data:
#          print d
          if d.type==0xB:
             addr=d.col*256+d.row
             if not addr in pixels:
               pixels[addr]=0
             pixels[addr]+=1
          elif d.type!=0x7:
             logging.warning("Unexpected packet %s"%str(d))
        logging.info("Packets received %d (pixels %d)"%(len(data),len(pixels)))
        res[i]=len(pixels)
    f=open(self.fname+"/rate.dat","w")
    for x in sorted(res):
      f.write("%d %d\n"%(x,res[x]))
    f.close()
    print to_be_masked

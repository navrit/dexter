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
      anim=['|','/','-','\\','|','/','-','\\']
      for i in range(0,512,1):

        self.tpx.setDac(TPX3_VTHRESH_FINE,i)
        data=self.tpx.recv_mask(0x7102000000000000, 0xFFFF000000000000)
        if 0 and len(data)>1:
          for d in data:
            
            logging.warning("after setdac %s"%(str(d)))

        self.tpx.openShutter()
#        self.tpx.sequentialReadout()
        
        tries=2
        data=[]
        while tries:
#        if 1:
          ndata=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)
          data+=ndata
          if len(data)==0 or data[-1].raw!=0x71A000000000:
             tries-=1
             time.sleep(0.002)
          else:
             break
          
        logging.debug("Packets received %d (to be masked %d)"%(len(data),mask))

        if len(data)==0:
             logging.error("No data !!")
             tries-=1
        elif data[-1].raw!=0x71A000000000:
             logging.error("Last packet %s"%(str(data[-1])))
             logging.error("Packets received %d (to be masked %d)"%(len(data),mask))


        if 1:
          print "%c %s %d/%d (packets %d)"%(13,anim[i%len(anim)],i,512,len(data)),
          sys.stdout.flush()

        
        for d in data:
#          logging.debug(str(d))
          if d.type==0xA:
            res[d.col][d.row][i]=d.event_counter
            
            if  d.event_counter>=200 and peak[d.col][d.row]==0: 
              peak[d.col][d.row]=1
            elif d.event_counter<=2 and peak[d.col][d.row]==1:
              self.tpx.setPixelMask(d.col,d.row,1)
              mask+=1
              peak[d.col][d.row]=2
          elif d.type!=0x7:
            logging.warning("Unexpected packet %s"%str(d))



        if mask>4000:
          logging.debug("Masking %d pixels"%mask)
#          self.tpx.flushFifoIn()
          self.tpx.setPixelConfig()
          data=self.tpx.recv_mask(0x718F000000000000, 0xFFFF000000000000)
          mask=0

      return res

  def _execute(self):
    self.tpx.resetPixels()
    dac_defaults(self.tpx)

#    self.tpx.setDac(TPX3_IBIAS_IKRUM,128)
#    self.tpx.setDac(TPX3_IBIAS_PREAMP_ON,128)
#    self.tpx.setDac(TPX3_IBIAS_DISCS1_ON,128)


    self.tpx.setGenConfig(0x04)
    self.tpx.getGenConfig()


    self.tpx.setPllConfig(  (TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK | TPX3_PHASESHIFT_DIV_8 | TPX3_PHASESHIFT_NR_16 | 0x14<<TPX3_PLLOUT_CONFIG_SHIFT) )
    self.tpx.getPllConfig()
    
#    self.tpx.setPllConfig(0x0E) 

    self.tpx.setCtprBits(0)
    self.tpx.setCtpr()


    mkdir(self.fname)

    self.tpx.setShutterLen(500)


    for cdac in range(0,16,1):
      logdir=self.fname+"/0x%0X/"%cdac
      logging.info("DACs %0x (logdir:%s)"%(cdac,logdir))
      mkdir(logdir)

      res={}
      for x in range(256):
        res[x]={}
        for y in range(256):
          res[x][y]={}

      for seq in range(4):
        logging.info("  seq %0x/4"%seq)
        self.tpx.resetPixelConfig()
        self.tpx.setPixelMask(ALL_PIXELS,ALL_PIXELS,1)
        self.tpx.setPixelThreshold(ALL_PIXELS,ALL_PIXELS,cdac)
        on=0
        for x in range(256):
          for y in range(256):
            if x%2==int(seq/2) and y%2==seq%2:
              self.tpx.setPixelMask(x,y,0)
              self.tpx.setPixelThreshold(x,y,cdac)

        self.tpx.setPixelConfig()
        self.tpx.sequentialReadout()
        self.tpx.flush_udp_fifo()
        res=self.threshold_scan(res)
        
        
      if 1: #fitting
        bad_pixels=[]
        w2f=True
        fit_res={}
        for col in range(256):
          fit_res[col]={}
          if w2f:mkdir(logdir+"/%03d"%col)
          if col in res:
            logging.info("Fitting col %d"%col)
          else:
            logging.info("No hits in col %d"%col)
            continue
          for row in range(256):
            if col in res and row in res[col]:
              if w2f:fn=logdir+"/%03d/%03d_%03d.dat"%(col,col,row)
              if w2f:f=open(fn,"w")
              codes=[]
              vals=[]
              avr=0.0
              N=0
              for code in sorted(res[col][row]):
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
#                  hist, bin_edges = numpy.histogram(codes,bins=int(max(codes)-min(codes)+1),range=(-0.5+min(codes), 0.5+max(codes)), weights=vals)
#                  bin_centres = (bin_edges[:-1] + bin_edges[1:])/2
                  # p0 is the initial guess for the fitting coefficients (A, mu and sigma above)
                  p0 = [max(vals), avr, 8.]
                  coeff, var_matrix = curve_fit(gauss, codes, vals, p0=p0)
                except:
                  coeff=[0.0,0.0,0.0]
                fit_res[col][row] =coeff
              else:
                fit_res[col][row] =[0.0,0.0,0.0]
                logging.warning("No hits for pixel (%d,%d)"%(col,row))
                bad_pixels.append( (col,row) )
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
        logging.info("Bad pixels for DAC=0x%X : %s"%(cdac,str(bad_pixels) ))
        if w2f:
          aname=logdir[:-1]+".zip"
          logging.info("Creating arhive %s"%aname)
          zipdir(aname,logdir)
          shutil.rmtree(logdir)



    if 0:#equalize
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


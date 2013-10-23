from tpx3_test import tpx3_test
import random
import time
import logging
from SpidrTpx3_engine import *
import os
import numpy

def mkdir(d):
  if not os.path.exists(d):
    os.makedirs(d)
    
class test11_pulse_map(tpx3_test):
  """Pixel matrix VCO and clock phasing in TOT&TOA mode"""

  def _execute(self):
    self.tpx.resetPixels()
    self.tpx.setDacsDflt()
    self.tpx.setDac(TPX3_IBIAS_IKRUM,15)
    self.tpx.setDac(TPX3_VTP_COARSE,50)
    self.tpx.setDac(TPX3_VTHRESH_COARSE,7) 
    self.tpx.setDac(TPX3_VFBK,143) 
    self.tpx.setDac(TPX3_IBIAS_PREAMP_ON,150)
    self.tpx.setDac(TPX3_IBIAS_DISCS1_ON,100)

    self.tpx.setDac(TPX3_VTHRESH_FINE,240)
    self.tpx.setGenConfig(0x20| 0x8 | 0x40)
    self.tpx.getGenConfig()
    self.tpx.setPllConfig(0x11E)
    self.tpx.getPllConfig()

    PULSES=1
    self.tpx.setTpPeriodPhase(0xF,0)
    self.tpx.setTpNumber(PULSES)

    mkdir(self.fname)

    self.tpx.resetPixelConfig()
    self.tpx.load_equalization("logs/F3_default_eq_bruteforce/test08_equalization/eq_codes.dat")
    self.tpx.maskPixel(ALL_PIXELS,ALL_PIXELS)
    pixlist=[(73, 92), (64, 26), (121, 86), (250, 104), (124, 128), (7, 33), (218, 148), (131, 179), (1, 181), (2, 132), (16, 180), (67, 247), (207, 233), (185, 83), (3, 242), (68, 105), (97, 27), (117, 233), (251, 26), (132, 230), (84, 153), (88, 227), (29, 113), (116, 244), (201, 86), (7, 210), (184, 67), (17, 135), (91, 209), (119, 154), (253, 39), (86, 15)]

    
    clist=[]
    for c,r in pixlist:
#      c=random.randint(0,255)
      clist.append(c)
#      r=random.randint(0,255)
#      pixlist.append( (c,r) )
#      print c,r
      self.tpx.unmaskPixel(c,r)
      self.tpx.configPixelTpEna(c,r, testbit=True)
#    print pixlist
    self.tpx.flushFifoIn()
    self.tpx.flush_udp_fifo()    
    self.tpx.setPixelConfig()

    for c in range(256):
      if c in clist:
        self.tpx.configCtpr(c,1)
      else:
        self.tpx.configCtpr(c,0)
    self.tpx.setCtpr()

    #testpulse amplitude
    self.tpx.setDac(TPX3_VTP_FINE,112+2*23)
    self.tpx.setSenseDac(TPX3_VTP_COARSE)
    coarse=self.tpx.get_adc(64)
    self.tpx.setSenseDac(TPX3_VTP_FINE)
    fine=self.tpx.get_adc(64)
    dv=1000.0*abs(fine-coarse)
    electrons=20.0*dv
    logging.info("Test pulse voltage %.4f mv (~ %.0f e-)"%(dv,electrons))

   
    result={}
    self.tpx.datadrivenReadout()
    for th in range(0,280):
      self.tpx.setDac(TPX3_VTHRESH_FINE,th)
      tot=[]
      toa=[]
      for loop in range(128):
        self.tpx.send(0x40,0,0)#reset timer
        self.tpx.send(0x4A,0,0)#t0sync

        self.tpx.openShutter(100)
        data=self.tpx.recv_mask(0x71B0000000000000,0xFFFF000000000000)
        self.tpx.flushFifoIn()
        shutter=self.tpx.getShutterStart()
        cnt=0

        for pck in data:
          if pck.type==0xB :
            if not pck.col in result:
              result[pck.col]={}
            if not pck.row in result[pck.col]:
              result[pck.col][pck.row]={}
            if not th  in result[pck.col][pck.row]:
              result[pck.col][pck.row][th]=[]

            cnt+=1
            v=16*(pck.toa-(shutter&0x3FFF)) 
#            if v<0: v+=0x4000
            v-=float(pck.ftoa)
#            print v
            tot.append(pck.tot)
            toa.append(v)
            result[pck.col][pck.row][th].append( (v,pck.tot) )
          elif pck.type!=0x7:
            logging.warning("Unexpected packet %s"%str(pck))
        print "Total pck count ",cnt

    for col,row in pixlist:
         print "(%d,%d)"%(col,row),
         f=open(self.fname+"/%03d_%03d.dat"%(col,row),"w")
         for th in result[col][row]:
           toal=[]
           totl=[]
           for toa,tot in result[col][row][th]:
             toal.append(float(toa))
             totl.append(float(tot))
           tota=numpy.array(totl)
           toaa=numpy.array(toal)

           f.write("%3d %.5e %.5e %.5e %.5e "%(th,numpy.mean(toaa), numpy.std(toaa),numpy.mean(tota), numpy.std(tota) ) )

           f.write("cnts: %3d "%len(result[col][row][th]) )

           f.write("toa:" )
           for toa in toal:
             f.write(" %5d "%toa )

           f.write("tot:" )
           for tot in totl:
             f.write(" %4d "%tot )
           f.write("\n")
            

           
         f.close()
#    f=open(self.fname+"/toa.dat","w")
#      f.write("%d %.5e %.5e %.5e %.5e\n"%(th))
#    fn=self.fname+'.map'



#(0x44) and High (0x45)

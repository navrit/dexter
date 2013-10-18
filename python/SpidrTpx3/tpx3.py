#!/usr/bin/env python
#import SpidrTpx3
import  SpidrTpx3
from SpidrTpx3_engine import SpidrController,SpidrDaq,UDPServer
import logging
import random
import Gnuplot, Gnuplot.funcutils
import time
import sys
from struct import *
from bitstring import BitArray
import cython
import numpy


def load_lut(fname):
      f=open(fname)
      lut={}

      for l in f.readlines()[3:]:
        if l.find("--")>=0:break
        ll=l.split("|")
        num_in=int(ll[1],16)
        num_out=int(ll[2],10)
        lut[num_in]=num_out
      return lut


evn4=load_lut("event_count_4b_LUT.txt")
itot14=load_lut("itot_14b_LUT.txt")
toa14=load_lut("toa_gray_count_14b_LUT.txt")
tote10=load_lut("tot_event_count_10b_LUT.txt")


class tpx3packet:
  def pack0(self):
    self.str="unimplemented"
  def pack1(self):
    self.str="unimplemented"
  def pack2(self):
    self.str="unimplemented"
  def pack3(self):
    self.str="unimplemented"
  def pack4(self):
    b0=(self.raw>>40)&0xFF
    if b0==0x44   :
      self.val= self.raw&0xFFFFFFFF ;
      self.str="Timer low %d"%self.val
    elif b0==0x45   :
      self.val= self.raw&0xFFFFFFFF ;
      self.str="Timer high %d"%self.val
    if b0==0x46   :
      self.val= self.raw&0xFFFFFFFF ;
      self.str="Timer shutter rise low %d"%self.val
    elif b0==0x47   :
      self.val= self.raw&0xFFFFFFFF ;
      self.str="Timer shutter rise high %d"%self.val
    else:
      self.str="unimplemented"
  def pack5(self):
    self.str="unimplemented"
  def pack6(self):
    self.str="unimplemented"
  def pack7(self):
    b0=(self.raw>>40)&0xFF
    b1=(self.raw>>32)&0xFF
    
    if b0==0x70   :  
      self.str="ACK"
    elif b0==0x71 :     
      if b1==0xDF:
        self.str="EoC"
      if b1==0x34:
        self.str="EoC SLVS Config"
      elif b1==0xD0:
        self.str="EoR"
      elif b1==0x44:
        self.str="EoC 0x44 (Timer Low)"
      elif b1==0x45:
        self.str="EoC 0x44 (Timer High)"
      elif b1==0x0d:
        self.str="EoC 0x0d (TP_PulseNumber)"
      elif b1==0x0c:
        self.str="EoC 0x0c (TP_Period)"
      elif b1==0xcf:
        self.str="EoC 0xcf (Set ctpr)"
      elif b1==0x31:
        self.str="EoC 0xcf (Read GeneralConfig)"
      elif b1==0x46:
        self.str="EoC 0x46 (Req Rising Shutter Low)"
      elif b1==0x47:
        self.str="EoC 0x47 (Req Rising Shutter High)"
      elif b1==0x30:
        self.str="EoC 0xcf (Set GeneralConfig)"
      elif b1==0x8F:
        self.str="EoC 0x8 (Load Matrix)"
      elif b1==0x0F:
        self.str="EoC 0x0F (Internal TestPulseFinished)"
      elif b1==0x9F:
        self.str="EoC 0x9 (Read Config Matrix)"
      elif b1==0xAF:
        self.str="EoC 0xA (Read Matrix Seq)"
      elif b1==0xEF:
        self.str="EoC 0xE (Reset Matrix Seq)"
      elif b1==0xFF:
        self.str="EoC 0xF (Stop Matrix Command)"
      elif b1==0xA0:
        self.str="EoR readout seq"
      elif b1==0xBF:
        self.str="EoC 0xB (Read Data Driven)"
      elif b1==0xB0:
        self.str="EoR Data driven"
    else :
      self.str="unimplemented"
  def pack8(self):
    self.str="unimplemented"
  def pack9(self):
     raw=self.raw>>14
     self.config=raw&0x3F
     raw>>=(6+8)
     self.pixel_address=raw&0x7
     raw>>=3
     self.sp_address  = raw&0x3F
     raw>>=6
     self.col_address = raw&0x7F
     
     self.col=self.col_address*2
     self.col+= (self.pixel_address&0x4)>>2
     self.row=self.sp_address*4
     self.row+= (self.pixel_address&0x3)
     
     self.str="ReadConfMatrix (%3d,%3d) dc=%3d sp=%3d pix=%3d config=0x%02X"%(self.col,self.row, self.col_address,self.sp_address,self.pixel_address,self.config)

    
  def packA(self):
     #event count and iTOT
     raw=self.raw>>4
     EventNounter=raw&0x3FF
     raw>>=10
     iTOT=raw&0x3FFF
     raw>>=14
     self.pixel_address=raw&0x7
     raw>>=3
     self.sp_address  = raw&0x3F
     raw>>=6
     self.col_address = raw&0x7F
     
     self.col=self.col_address*2
#     self.col+= (self.pixel_address&0x4)>>2
     if self.pixel_address&0x4 : self.col+=1
     self.row=self.sp_address*4
     self.row+= (self.pixel_address&0x3)
     
     self.cnt=tote10[(self.raw>>4)&0x3FF]
     self.str="DataSeq (%3d,%3d) dc=%3d sp=%3d pix=%3d cnt=%d"%(self.col,self.row, self.col_address,self.sp_address,self.pixel_address, self.cnt)

    
  def packB(self):
#     self.col_address=((self.b0<<3)&0x78) | ((self.b1>>5)&0x7) 
#     self.sp_address= ((self.b1<<1)&0x3E) | ((self.b2>>7)&0x1) 
#     self.pixel_address=((self.b2>>4)&0x07)
#     self.col=self.col_address*2
#     if self.pixel_address&0x4 : self.col+=1
#     self.row=self.sp_address*4
#     self.row+= (self.pixel_address&0x3)

#     self.ftoa=self.raw&0xF
#     self.tot=(self.raw>>4)&0x3FF

#     self.toa=(self.raw>>14)&0x3FFF
#     self.toa=toa14[self.toa]
#     self.tot=tote10[self.tot]
#     self.str="DataDriven (%3d,%3d) dc=%3d sp=%3d pix=%3d toa=%d tot=%d ftoa=%d"%(self.col,self.row, self.col_address,self.sp_address,self.pixel_address, self.toa,self.tot, self.ftoa)
     raw=self.raw>>4
     EventNounter=raw&0x3FF
     raw>>=10
     iTOT=raw&0x3FFF
     raw>>=14
     self.pixel_address=raw&0x7
     raw>>=3
     self.sp_address  = raw&0x3F
     raw>>=6
     self.col_address = raw&0x7F
     
     self.col=self.col_address*2
     #self.col+= (self.pixel_address&0x4)>>2
     if self.pixel_address&0x4 : self.col+=1
     self.row=self.sp_address*4
     self.row+= (self.pixel_address&0x3)
     
     self.cnt=tote10[(self.raw>>4)&0x3FF]

  def packC(self):
    self.str="unimplemented"
  def packD(self):
    raw=self.raw
    self.ctpr=raw&0x3
    raw>>=(2+8)
    self.eoc_fifo=raw&0xf
    raw>>(4+9)
    self.toa=raw&0x3FFF
    raw>>=14
    self.addr=raw&0x7F
    self.str="Read CTPR adr:%03d ctpr:%01x toa:%x  eoc_fifo:%x"%(self.addr, self.ctpr,self.toa, self.eoc_fifo)
  def packE(self):
    self.str="unimplemented"
  def packF(self):
    self.str="unimplemented"
    
  @cython.locals(raw=cython.long)
  def __init__(self,data):
    self.raw=data>>16
#    print "%016x"%data
#    self.ba= BitArray(uint=(data>>16)&0xffffffffffff, length=48)

#    self.b0=data&0xFF
#    self.b1=(data>>8)&0xFF
#    self.b2=(data>>16)&0xFF
#    self.b3=(data>>24)&0xFF
#    self.b4=(data>>32)&0xFF
#    self.b5=(data>>40)&0xFF
#    self.raw=self.b5 | (self.b4<<8) | (self.b3<<16)  | (self.b2<<24) | (self.b1<<32)  | (self.b0<<40)

    self.type=(self.raw>>44)&0xf
    self.str='-'
    self.pack_interpreter[self.type](self)
#    print self.str
  def __repr__(self):
    return "[%012X] %s"%(self.raw, self.str)
  pack_interpreter=[pack0,pack1,pack2,pack3,pack4,pack5,pack6,pack7,
                    pack8,pack9,packA,packB,packC,packD,packE,packF]


class TPX3:
  def __init__(self,ip):
    port=50000
    if len(ip.split(":")):
      port=int(ip.split(":")[1])
      ip=ip.split(":")[0]

    ip0,ip1,ip2,ip3=map(int,ip.split('.'))

    self.ctrl=SpidrController( ip0,ip1,ip2,ip3,port)
    if  not self.ctrl.isConnected() :
      logging.critical("Unable to connect %s (%s)"%(self.ctrl.ipAddressString(), self.ctrl.connectionErrString()))
      raise RuntimeError("Unable to connect")
    self.udp=UDPServer()
    self.udp.start(8192)
#    self.daq=SpidrDaq( self.ctrl )
#    self.daq.setFlush(False)
    self.id=0
    self.log_packets=False
    self.dacs=numpy.zeros((256,256), int)
    self.tpena=numpy.zeros((256,256), int)
    
  def stop(self):
#    self.daq.stop()
    pass
  def _vdd2str(self,meas):
    r,v,i,p=meas
    v=float(v)/1000
    i=float(i)/10
    p=float(p)/1000
    return "%.3f V %.1f mA %.3f W"%(v,i,p)

  def get_dvdd_v(self):
    return float(self.ctrl.getDvdd()[1])/1000
  def get_dvdd_i(self):
    return float(self.ctrl.getDvdd()[2])/10000
  def get_dvdd_p(self):
    return float(self.ctrl.getDvdd()[3])/1000

  def get_avdd_v(self):
    return float(self.ctrl.getAvdd()[1])/1000
  def get_avdd_i(self):
    return float(self.ctrl.getAvdd()[2])/10000
  def get_avdd_p(self):
    return float(self.ctrl.getAvdd()[3])/1000

  def get_avdd_str(self):
    return self._vdd2str(self.ctrl.getAvdd())

  
  def get_dvdd_str(self):
    return self._vdd2str(self.ctrl.getDvdd())
    
  def get_avdd_str(self):
    return self._vdd2str(self.ctrl.getAvdd())
    
  def get_adc(self,measurements=1):
    vv=[]
    ret=True
    for i in range(measurements):
      ret,val=self.ctrl.getAdc(self.id)
      if not ret:
        break
      vv.append(float(val))
    val=sum(vv)/len(vv)
    val=1.5*val/4096
#    self._log_ctrl_cmd("ADC measurement %.4f"%val,ret)
    return val

  def _temp2str(self,t):
    f=float(t)
    if t> 250000:
      return '-'
    else:
      return "%.2f"%(t/1000)
      
  def get_remote_temp(self):
    return self._temp2str(self.ctrl.getRemoteTemp()[1])
    
  def get_local_temp(self):
    return self._temp2str(self.ctrl.getLocalTemp()[1])
    
  def info(self):
    
    logging.info( "Controller IP       : %s"%self.ctrl.ipAddressString())
    if "daq" in self.__dict__:
      print "DAQ Class version   : %08x"%self.daq.classVersion()
      print "DAQ IP              : %s"%self.daq.ipAddressString()
    logging.info( "Digital Chip Supply : %s"%self.get_dvdd_str())
    logging.info( "Analog Chip Supply  : %s"%self.get_avdd_str())
    logging.info( "Chip temp.          : %s"%self.get_remote_temp())
    logging.info( "Chip card temp.     : %s"%self.get_local_temp())
 
  def _send_raw(self, buf):
    pck=[0xaa,0x00,0x00,0x00,0x00]
    for b in buf:
      pck.append(b&0xFF)
    l=len(pck)
    if l>256:
      print "Too long packet !! Not sending"
      return
    if self.log_packets:
      s='<['
      for i in range(l):
        s+="%02X"%pck[i]
      s+=']'
      logging.debug(s)
    
    for i in range(256-l):
      pck.append(0)
    pck=map(int, pck)
    self.ctrl.uploadPacket(self.id,list(pck),size=(l+2))
#    time.sleep(2)
  def send_byte_array(self,buf):
    self._send_raw(buf)

  def send(self,b0,b1,b2):
    buf=[b0,b1,b2]
    self._send_raw(buf)
    
  def recv(self,N):
    r=self.udp.getN(N,debug=0)
    ret=[]
    for pck_num in r:
      p=tpx3packet(pck_num)
      ret.append(p)
    return ret

  def flush_udp_fifo(self):
    return self.recv_mask(0x1234000000000000, 0xFFFF000000000000)

  def _log_ctrl_cmd(self,msg,result):
    if result:
      logging.info("%-80s [  OK  ]"%msg)
    else:
      logging.error("%-80s [FAILED] (%s)"%(msg,self.ctrl.errorString()))

  def setTpPeriodPhase(self,period, phase):
    r=self.ctrl.setTpPeriodPhase(self.id,period,phase)
    self._log_ctrl_cmd("setTpPeriodPhase(%d,%d) "%(period,phase),r)

  def setTpNumber(self,number):
    r=self.ctrl.setTpNumber(self.id,number)
    self._log_ctrl_cmd("setTpNumber(%d) "%(number),r)

  def configPixel(self,x,y,dac, testbit=False):
    self.dacs[x][y]=dac
    self.tpena[x][y]=testbit
    r=self.ctrl.configPixel(x,y,dac, testbit)

  def configPixelTpEna(self,x,y, testbit=False):
    self.tpena[x][y]=testbit
    r=self.ctrl.configPixel(x,y,self.dacs[x][y], testbit)

#    self._log_ctrl_cmd("configPixel(%d,%d,%d,%d) "%(x,y,threshold, testbit),r)

  def resetPixelConfig(self):
    r=self.ctrl.resetPixelConfig()
    self._log_ctrl_cmd("resetPixelConfig() ",True)

  def maskPixel(self,x,y):
    r=self.ctrl.maskPixel(x,y)
#    self._log_ctrl_cmd("maskPixel(%d,%d) "%(x,y),r)

  def unmaskPixel(self,x,y):
    r=self.ctrl.unmaskPixel(x,y)
#    self._log_ctrl_cmd("unmaskPixel(%d,%d) "%(x,y),r)

  def setPixelConfig(self):
    r=self.ctrl.setPixelConfig(self.id)
    self._log_ctrl_cmd("setPixelConfig() ",r)

  def getPixelConfig(self):
    r=self.ctrl.getPixelConfig(self.id)
    self._log_ctrl_cmd("getPixelConfig() ",r)

  def resetPixels(self):
    r=self.ctrl.resetPixels(self.id)
    self._log_ctrl_cmd("resetPixels() ",r)
    
  def configCtpr(self,column,val):
    r=self.ctrl.configCtpr(self.id,column,val)
    self._log_ctrl_cmd("configCtpr(%d,%d) "%(column,val),r)
  
  def setCtpr(self):
    r=self.ctrl.setCtpr(self.id)
    self._log_ctrl_cmd("setCtpr() ",r)

  def sequentialReadout(self):
    r=self.ctrl.sequentialReadout(self.id)
    self._log_ctrl_cmd("sequentialReadout() ",r)
    
    
  def datadrivenReadout(self):
    r=self.ctrl.datadrivenReadout(self.id)
    self._log_ctrl_cmd("datadrivenReadout() ",r)
    

  def openShutter(self,l):
    r=self.ctrl.openShutter(self.id,l)
    self._log_ctrl_cmd("openShutter(%d) "%(l),r)
    
  def setSenseDac(self,code):
    r=self.ctrl.setSenseDac(self.id,code)
    self._log_ctrl_cmd("setSenseDac(%d) "%(code),r)

  
  def setDac(self,code,val):
    r=self.ctrl.setDac(self.id,code,val)
    self._log_ctrl_cmd("setDac(%d,%d) "%(code,val),r)

  def setGenConfig(self,l):
    r=self.ctrl.setGenConfig(self.id,l)
    self._log_ctrl_cmd("setGenConfig(%04x) "%(l),r)
    
  def setDacsDflt(self):
    r=self.ctrl.setDacsDflt(self.id)
    self._log_ctrl_cmd("setDacsDflt()",r)
  
  def getGenConfig(self):
    r,val=self.ctrl.getGenConfig(self.id)
    self._log_ctrl_cmd("getGenConfig()=%02x"%(val),r)
    return val

  def setPllConfig(self,l):
    r=self.ctrl.setPllConfig(self.id,l)
    self._log_ctrl_cmd("setPllConfig(%04x) "%(l),r)

  def getPllConfig(self):
    r,val=self.ctrl.getPllConfig(self.id)
    self._log_ctrl_cmd("getPllConfig()=%02x"%(val),r)
    return val
    
  def getShutterStart(self):
#    r,lo,hi=self.ctrl.getShutterStart(self.id)
#    self._log_ctrl_cmd("getShutterStart()=%x %x"%(hi,lo),r)
    self.send(0x46,0,0)
    resp=self.recv_mask(0x4671,0xFFFF)
    for p in resp:
      if p.b0==0x46: low=p.val
    self.send(0x47,0,0)
    resp=self.recv_mask(0x4771,0xFFFF)
    for p in resp:
      if p.b0==0x47: high=p.val
    v=low+(high<<32)
    self._log_ctrl_cmd("getShutterStart()=%d"%(v),True)
    return v

  def flushFifoIn(self):
    r=self.ctrl.flushFifoIn(self.id)
    self._log_ctrl_cmd("flushFifoIn()",r)
    return 
    
#c2.add_method('getTpNumber',           'bool',        [param('int', 'dev_nr'),param('int*', 'number', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
#c2.add_method('setTpNumber',           'bool',        [param('int', 'dev_nr'),param('int', 'number')])


  def recv_mask(self,val,mask):
    r=self.udp.getH(val,mask,debug=0)
    ret=[]
    for pck_num in r:
      p=tpx3packet(pck_num)
      ret.append(p)
      if self.log_packets : logging.debug(p)
    return ret
    
  def __del__(self):
    pass

  def load_equalization(self,fname):
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
    eq=load('logs/thscan1/test08_th_scan_noise/eq.codes')
    self.resetPixelConfig()
    for x in range(256):
      for y in range(256):
          self.configPixel(x,y,eq[y][x],False)
#    self.setPixelConfig()
    
def main():
  tpx=TPX3()
  tpx.info()
  
if __name__=="__main__":
  main()


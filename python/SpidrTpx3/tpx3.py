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
    if self.b0==0x44   :
      val= self.b5 | self.b4<<8 | self.b3<<16 | self.b2<<24 ;
      self.str="Timer low %d"%val
    elif self.b0==0x45   :
      val= self.b5 | self.b4<<8 | self.b3<<16 | self.b2<<24 ;
      self.str="Timer high %d"%val
    else:
      self.str="unimplemented"
  def pack5(self):
    self.str="unimplemented"
  def pack6(self):
    self.str="unimplemented"
  def pack7(self):
    if self.b0==0x70   :  
      self.str="ACK"
    elif self.b0==0x71 :     
      if self.b1==0xDF:
        self.str="EoC"
      if self.b1==0x34:
        self.str="EoC SLVS Config"
      elif self.b1==0xD0:
        self.str="EoR"
      elif self.b1==0x44:
        self.str="EoC 0x44 (Timer Low)"
      elif self.b1==0x45:
        self.str="EoC 0x44 (Timer High)"
      elif self.b1==0x8F:
        self.str="EoC 0x8 (Load Matrix)"
      elif self.b1==0x9F:
        self.str="EoC 0x9 (Read Config Matrix)"
      elif self.b1==0xAF:
        self.str="EoC 0xA (Read Matrix Seq)"
      elif self.b1==0xEF:
        self.str="EoC 0xE (Reset Matrix Seq)"
      elif self.b1==0xFF:
        self.str="EoC 0xF (Stop Matrix Command)"
      elif self.b1==0xA0:
        self.str="EoR"
    else :
      self.str="unimplemented"
  def pack8(self):
    self.str="unimplemented"
  def pack9(self):
     self.col_address=((self.b0<<3)&0x78) | ((self.b1>>5)&0x7) 
     self.sp_address= ((self.b1<<1)&0x3E) | ((self.b2>>7)&0x1) 
     self.pixel_address=((self.b2>>4)&0x07)
     self.config= ((self.b3&0xF)<<2) | ((self.b4>>6)&0x03)

     self.col=self.col_address*2
     if self.pixel_address&0x4 : self.col+=1
        
     self.row=self.sp_address*4
     self.row+= (self.pixel_address&0x3)

     self.str="ReadConfMatrix (%3d,%3d) dc=%3d sp=%3d pix=%3d config=0x%02X"%(self.col,self.row, self.col_address,self.sp_address,self.pixel_address,self.config)

    
  def packA(self):
    self.str="unimplemented"
  def packB(self):
    self.str="unimplemented"
  def packC(self):
    self.str="unimplemented"
  def packD(self):
    self.addr=(self.b0&0xF)<<3 | (self.b1>>5)
    self.ctpr=self.b5&0x3

    self.toa=((self.b3&0xf) <<10) | (self.b3<<2) | (self.b4>>6)&0x3
    self.eoc_fifo=(self.b4>>2)&0xf

    self.str="Read CTPR adr:%03d ctpr:%01x toa:%x  eoc_fifo:%x"%(self.addr, self.ctpr,self.toa, self.eoc_fifo)
  def packE(self):
    self.str="unimplemented"
  def packF(self):
    self.str="unimplemented"
  def __init__(self,data):
    self.raw=data
    self.b0=data&0xFF
    self.b1=(data>>8)&0xFF
    self.b2=(data>>16)&0xFF
    self.b3=(data>>24)&0xFF
    self.b4=(data>>32)&0xFF
    self.b5=(data>>40)&0xFF
    self.type=self.b0>>4
    self.str='-'
    self.pack_interpreter[self.type](self)
  def __repr__(self):
    return "[%02X%02X%02X%02X%02X%02X] %s"%(self.b0,self.b1,self.b2,self.b3,self.b4,self.b5, self.str)
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
    for i in range(measurements):
      ret,val=self.ctrl.getAdc(self.id)
      vv.append(float(val))
    val=sum(vv)/len(vv)
    val=1.5*val/4096
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
    
def main():
  tpx=TPX3()
  tpx.info()
  
if __name__=="__main__":
  main()


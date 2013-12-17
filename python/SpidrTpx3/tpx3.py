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


evn4=load_lut("SpidrTpx3/luts/event_count_4b_LUT.txt")
itot14=load_lut("SpidrTpx3/luts/itot_14b_LUT.txt")
toa14=load_lut("SpidrTpx3/luts/toa_gray_count_14b_LUT.txt")
tote10=load_lut("SpidrTpx3/luts/tot_event_count_10b_LUT.txt")


class tpx3packet:
  mode=0
  def pack0(self):
    self.str="unimplemented"
  def pack1(self):
    self.str="unimplemented"
  def pack2(self):
    self.str="unimplemented"
  def pack3(self):
    self.reg= self.raw>>40&0xF
    if self.reg==1:
      raw=self.raw
      self.pol=raw&0x1
      raw>>=1
      self.mode=raw&0x3
      tpx3packet.mode=self.mode
      raw>>=2
      self.str="Read General Config (pol=%d, mode=%d)"%(self.pol,self.mode)
    else:
      self.str="unimplemented"
    
    
  def pack4(self):
    if self.b0==0x44   :
      self.val= self.raw&0xFFFFFFFF ;
      self.str="Timer low %d"%self.val
    elif self.b0==0x45   :
      self.val= self.raw&0xFFFFFFFF ;
      self.str="Timer high %d"%self.val
    if self.b0==0x46   :
      self.val= self.raw&0xFFFFFFFF ;
      self.str="Timer shutter rise low %d"%self.val
    elif self.b0==0x47   :
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
      elif b1==0x02:
        self.str="EoC 0x02 (SetDac)"
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
      elif b1==0xB0:
        self.str="EoR data driven"
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
     self.str="DataSeq "
     if tpx3packet.mode==0:
       self.mode0()
     elif tpx3packet.mode==2:
       self.mode2()
     else:
       self.str+="Not parsed"

    
  def packB(self):
     self.str="DataDriven "
     if tpx3packet.mode==0:
       self.mode0()
     elif tpx3packet.mode==2:
       self.mode2()
     else:
       self.str+="Not parsed"

  def mode2(self):
       #event count and iTOT
     raw=self.raw>>4
     self.event_counter=raw&0x3FF
     raw>>=10
     self.itot=raw&0x3FFF
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
     
     self.event_counter=tote10[self.event_counter]
     self.itot=itot14[self.itot]
     self.str+="(%3d,%3d) dc=%3d sp=%3d pix=%3d evn_cnt=%d itot=%d"%(self.col,self.row, self.col_address,self.sp_address,self.pixel_address, self.event_counter, self.itot)


  def mode0(self):
     raw=self.raw
     self.ftoa=raw&0xf
     raw>>=4
     self.tot=raw&0x3FF
     raw>>=10
     self.toa=raw&0x3FFF
     raw>>=14
     self.pixel_address=raw&0x7
     raw>>=3
     self.sp_address  = raw&0x3F
     raw>>=6
     self.col_address = raw&0x7F

     self.col=self.col_address*2
     if self.pixel_address&0x4 : self.col+=1
     self.row=self.sp_address*4
     self.row+= (self.pixel_address&0x3)
     
     self.tot=tote10[self.tot]
     self.toa=toa14[self.toa]
     
     self.str+="(%3d,%3d) dc=%3d sp=%3d pix=%3d toa=%d tot=%d ftoa=%d"%(self.col,self.row, self.col_address,self.sp_address,self.pixel_address,  self.toa,self.tot,self.ftoa)

  def packC(self):
    self.str="unimplemented"
  def packD(self):
    raw=self.raw
    self.ctpr=raw&0x3
    raw>>=(2+8)
    self.eoc_fifo=raw&0xf
    raw>>=(4)
    self.toa=toa14[raw&0x3FFF]
    raw>>=(14+9)
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
    self.b0=(self.raw>>40)&0xFF

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
#    for i in range(measurements):
    ret,val=self.ctrl.getAdc(self.id,measurements)
#      if not ret:
#        break
#      vv.append(float(val))
    val=float(val)/measurements
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
                                                       
  def flush_udp_fifo(self,val=0x1234000000000000, mask=0xFFFF000000000000):
    data = self.recv_mask(val,mask)
    for d in data:
      logging.debug("FLUSH : %s"%(str(d)))
    

  def _log_ctrl_cmd(self,msg,result):
    if result:
      logging.debug("%-80s [  OK  ]"%msg)
    else:
      logging.error("%-80s [FAILED] (%s)"%(msg,self.ctrl.errorString()))

  def setTpPeriodPhase(self,period, phase):
    r=self.ctrl.setTpPeriodPhase(self.id,period,phase)
    self._log_ctrl_cmd("setTpPeriodPhase(%d,%d) "%(period,phase),r)

  def setTpNumber(self,number):
    r=self.ctrl.setTpNumber(self.id,number)
    self._log_ctrl_cmd("setTpNumber(%d) "%(number),r)

  def setPixelThreshold(self,x,y,dac):
    r=self.ctrl.setPixelThreshold(x,y,dac)

  def setPixelTestEna(self,x,y, testbit=False):
    r=self.ctrl.setPixelTestEna(x,y, testbit)

#    self._log_ctrl_cmd("configPixel(%d,%d,%d,%d) "%(x,y,threshold, testbit),r)

  def resetPixelConfig(self):
    r=self.ctrl.resetPixelConfig()
    self._log_ctrl_cmd("resetPixelConfig() ",True)

  def pauseReadout(self):
    r=self.ctrl.pauseReadout()
    self._log_ctrl_cmd("pauseReadout() ",True)
  
  def setPixelMask(self,x,y,v):
    r=self.ctrl.setPixelMask(x,y,v)

  def setPixelConfig(self):
    r=self.ctrl.setPixelConfig(self.id, cols_per_packet=2)
    time.sleep(1)
    self._log_ctrl_cmd("setPixelConfig() ",r)

  def getPixelConfig(self):
    r=self.ctrl.getPixelConfig(self.id)
    self._log_ctrl_cmd("getPixelConfig() ",r)

  def resetPixels(self):
    r=self.ctrl.resetPixels(self.id)
    self._log_ctrl_cmd("resetPixels() ",r)
    
  def setCtprBit(self,column,val):
    r=self.ctrl.setCtprBit(column,val)
    self._log_ctrl_cmd("setCtprBit(%d,%d) "%(column,val),r)

  def setCtprBits(self,val):
    r=self.ctrl.setCtprBits(val)
    self._log_ctrl_cmd("setCtprBits(%d) "%(val),r)

  
  def setCtpr(self):
    r=self.ctrl.setCtpr(self.id)
    self._log_ctrl_cmd("setCtpr() ",r)

  def sequentialReadout(self,tokens=128):
    r=self.ctrl.sequentialReadout(tokens)
    self._log_ctrl_cmd("sequentialReadout(tokens=%d) "%tokens,r)
    
    
  def datadrivenReadout(self):
    r=self.ctrl.datadrivenReadout()
    self._log_ctrl_cmd("datadrivenReadout() ",r)

  def setShutterLen(self,l):
    r=self.ctrl.setTriggerConfig(4,l,1,1)
    self._log_ctrl_cmd("Config shutter (%d) "%(l),r)
    
  def resetTimer(self):
    r=self.ctrl.resetTimer(self.id)
    self._log_ctrl_cmd("resetTimer() ",r)


  def openShutter(self):
    r=self.ctrl.startAutoTrigger()
#c2.add_method('stopAutoTrigger',      'bool',       [])
#    r=self.ctrl.openShutter(self.id,l)
    self._log_ctrl_cmd("Start shutter() ",r)

  def t0Sync(self):
    r=self.ctrl.t0Sync(self.id)
    self._log_ctrl_cmd("t0Sync() ",r)
    
    
    
  def setSenseDac(self,code):
    r=self.ctrl.setSenseDac(self.id,code)
    self._log_ctrl_cmd("setSenseDac(%d) "%(code),r)

  
  def setDac(self,code,val):
    r=self.ctrl.setDac(self.id,code,val)
    self._log_ctrl_cmd("setDac(%d,%d) "%(code,val),r)

  def getDac(self,code):
    r,v=self.ctrl.getDac(self.id,code)
    self._log_ctrl_cmd("getDac(%d)=%d "%(code,v),r)
    return v
 
  def setGenConfig(self,l):
    r=self.ctrl.setGenConfig(self.id,l)
    tpx3packet.mode=(l>>1)&0x3

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

  def getTimer(self):
    r,lo,hi=self.ctrl.getTimer(self.id)
    v=lo + (hi<<32)
#    print r,lo,hi
    self._log_ctrl_cmd("getTimer()=%d"%(v),True)
    return v
  
  def getShutterStart(self):
#    r,lo,hi=self.ctrl.getShutterStart(self.id)
#    self._log_ctrl_cmd("getShutterStart()=%x %x"%(hi,lo),r)
#    self.send(0x46,0,0)
#    resp=self.recv_mask(0x7146000000000000,0xFFFF000000000000)

#    for p in resp:
#      print "ST",p,">> %02x"%p.b0
#      if p.b0==0x46: 
#        low=p.val
#    self.send(0x47,0,0)
#    resp=self.recv_mask(0x7147000000000000,0xFFFF000000000000)
#    for p in resp:
#      print "ST",p
#      if p.b0==0x47: high=p.val
#    v=low+(high<<32)
    r,lo,hi=self.ctrl.getShutterStart(self.id)
    v=lo + (hi<<32)
#    print r,lo,hi
    self._log_ctrl_cmd("getShutterStart()=%d"%(v),True)
    return v

  def getShutterEnd(self):
#    r,lo,hi=self.ctrl.getShutterStart(self.id)
#    self._log_ctrl_cmd("getShutterStart()=%x %x"%(hi,lo),r)
#    self.send(0x46,0,0)
#    resp=self.recv_mask(0x7146000000000000,0xFFFF000000000000)

#    for p in resp:
#      print "ST",p,">> %02x"%p.b0
#      if p.b0==0x46: 
#        low=p.val
#    self.send(0x47,0,0)
#    resp=self.recv_mask(0x7147000000000000,0xFFFF000000000000)
#    for p in resp:
#      print "ST",p
#      if p.b0==0x47: high=p.val
#    v=low+(high<<32)
    r,lo,hi=self.ctrl.getShutterEnd(self.id)
    v=lo + (hi<<32)
    self._log_ctrl_cmd("getShutterEnd()=%d"%(v),True)
    return v

  def getHeaderFilter(self):
    r,eth,cpu=self.ctrl.getHeaderFilter(self.id)
    self._log_ctrl_cmd("getHeaderFilter()=0x%04X,0x%04X"%(eth,cpu),r)
    return eth,cpu

  def setHeaderFilter(self,eth,cpu):
    r=self.ctrl.setHeaderFilter(self.id,eth,cpu)
    self._log_ctrl_cmd("setHeaderFilter(0x%04X,0x%04X)"%(eth,cpu),r)

  
#  def flushFifoIn(self):
#    r=self.ctrl.flushFifoIn(self.id)
#    self._log_ctrl_cmd("flushFifoIn()",r)
#    return 
    
#c2.add_method('getTpNumber',           'bool',        [param('int', 'dev_nr'),param('int*', 'number', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
#c2.add_method('setTpNumber',           'bool',        [param('int', 'dev_nr'),param('int', 'number')])


  def recv_mask(self,val,mask):
    ok=False
    cnt=2
    ret=[]
    last=0
    while cnt>0 and not ok:
      r=self.udp.getH(val,mask,debug=0)
      for pck_num in r:
        p=tpx3packet(pck_num)
        ret.append(p)
        if self.log_packets : logging.debug(p)
        last=pck_num
      cnt-=1
      if (last&mask)==val: ok=True
    if not ok:
      logging.warning("Timeout ;/ (last packet : %16X while expecting %016X)"%(last,val))
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


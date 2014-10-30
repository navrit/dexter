#!/usr/bin/env python
#import SpidrTpx3
import  SpidrTpx3
from SpidrTpx3_engine import *
#SpidrController,SpidrDaq,UDPServer
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

#d='../'
d=''
#print d,__file__
#evn4=load_lut(d+"SpidrTpx3/luts/event_count_4b_LUT.txt")
#itot14=load_lut(d+"SpidrTpx3/luts/itot_14b_LUT.txt")
#toa14=load_lut(d+"SpidrTpx3/luts/toa_gray_count_14b_LUT.txt")
#tote10=load_lut(d+"SpidrTpx3/luts/tot_event_count_10b_LUT.txt")



class tpx3packet:
  mode=0
  pileup_decode=0
  hw_dec_ena=0
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
     
     if not self.hw_dec_ena:
       if self.event_counter in tote10:
         self.event_counter=tote10[self.event_counter]
       else:
         self.event_counter=-1
     
       if self.itot in itot14:
         self.itot=itot14[self.itot]
       else:
         self.itot=-1
     self.str+="(%3d,%3d)  evn_cnt=%d itot=%d"%(self.col,self.row, self.col_address,self.sp_address,self.pixel_address, self.event_counter, self.itot)


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
     
     if not self.hw_dec_ena:
       if self.tot in tote10:
         self.tot=tote10[self.tot]
       else:
#         self.logging.warning("Packet decode: Invalid tot value = %0x [%012X]"%(self.tot,self.raw))
         self.tot=-1
     
       if self.toa in toa14:
         self.toa=toa14[self.toa]
       else:
#       self.logging.warning("Packet decode: Invalid toa value = %0x [%012X]"%(self.toa,self.raw))
         self.toa=-1
#     else: # self.hw_dec_ena:

     self.abs_toa= (self.ext_toa<<14) + self.toa - self.ftoa
     if not self.pileup_decode:
       self.abs_toa -= self.ftoa
     self.abs_toa *= 25e-9
     
     if self.pileup_decode:
         if not self.hw_dec_ena:
           if self.ftoa in evn4:
             self.pileup=evn4[self.ftoa]
           else:
             self.pileup=-1
         else:
           self.pileup=self.ftoa
         self.str+="(%3d,%3d) dc=%3d sp=%3d pix=%3d toa=%d tot=%d pileup=%d"%(self.col,self.row, self.col_address,self.sp_address,self.pixel_address,  self.toa,self.tot,self.pileup)
     else:
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
  def isData(self):
    if self.type in (0xA,0xB):
      return True
    return 
  def isEoR(self):
    if self.raw&0xFFFF00000000 in (0x71b000000000,0x71b000000000):
      return True 
    else:
      return False
  @cython.locals(raw=cython.long)

  def __init__(self,data):
    self.ext_toa=data&0xFFFF

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
    self.shutter_len=1e-6
#    print self.str
  def __repr__(self):
    return "[%012X] %s"%(self.raw, self.str)
  pack_interpreter=[pack0,pack1,pack2,pack3,pack4,pack5,pack6,pack7,
                    pack8,pack9,packA,packB,packC,packD,packE,packF]


#pixel lookup address
address_loopup_list=[]
for raw in range(256*256):
   pixel_address=raw&0x7
   raw>>=3
   sp_address  = raw&0x3F
   raw>>=6
   col_address = raw&0x7F
   col=col_address*2
   if pixel_address&0x4 : col+=1
   row=sp_address*4
   row+= (pixel_address&0x3)
   address_loopup_list.append( (col,row) )
     


#higher performance (no string generation online)
class tpx3packet_hp:
  mode=0
  pileup_decode=0
  hw_dec_ena=0

  def isData(self):
    if self.type in (0xA,0xB):
      return True
    return 
  def isEoR(self):
    if self.raw&0xFFFF00000000 in (0x71b000000000,0x71b000000000):
      return True 
    else:
      return False

  @cython.locals(raw=cython.long)

  def __init__(self,data):
    self.ext_toa=data&0xFFFF
    self.raw=data>>16
    self.type=(self.raw>>44)&0xf
    self.str='-'
    if self.isData():
      if tpx3packet_hp.mode==0:
        raw=self.raw
        self.ftoa=raw&0xf
        raw>>=4
        self.tot=raw&0x3FF
        raw>>=10
        self.toa=raw&0x3FFF
        raw>>=14
        self.pixel_address=raw&0xFFFF
        self.col,self.row=address_loopup_list[self.pixel_address]
     
        self.abs_toa= (self.ext_toa<<14) + self.toa - self.ftoa
        if not self.pileup_decode:
          self.abs_toa -= float(self.ftoa)/16
        self.abs_toa *= 25e-9
     
        if self.pileup_decode:
           self.pileup=self.ftoa

      elif tpx3packet_hp.mode==2:
        #event count and iTOT
        raw=self.raw>>4
        self.event_counter=raw&0x3FF
        raw>>=10
        self.itot=raw&0x3FFF
        raw>>=14
        self.pixel_address=raw&0xFFFF
        self.col,self.row=address_loopup_list[self.pixel_address]

      else:
        pass
    elif self.type==0x9:
     raw=self.raw>>14
     self.config=raw&0x3F
     raw>>=(6+8)
     self.pixel_address=raw&0xFFFF
     self.col,self.row=address_loopup_list[self.pixel_address]

  def __repr__(self):
    self.str="-"
    if self.isData():
      if tpx3packet_hp.mode==0:
        if self.pileup_decode:
            self.str+="(%3d,%3d) toa=%d tot=%d pileup=%d"%(self.col,self.row, self.toa,self.tot,self.pileup)
        else:
            self.str+="(%3d,%3d) toa=%d tot=%d ftoa=%d"%(self.col,self.row, self.toa,self.tot,self.ftoa)
      elif tpx3packet_hp.mode==2:
        self.str+="(%3d,%3d)  evn_cnt=%d itot=%d"%(self.col,self.row, self.event_counter, self.itot)
    return "[%012X] %s"%(self.raw, self.str)




#higher performance (no string generation online)
class Tpx3DataPacket:
  mode=0
  pileup_decode=0
  hw_dec_ena=0

  #@cython.locals(raw=cython.long)
  def __init__(self,col,row,data,ext_toa):
    self.ext_toa=ext_toa
    self.raw=data
    self.str='-'
    self.col=col
    self.row=row
    if Tpx3DataPacket.mode==0:
        raw=self.raw
        self.ftoa=raw&0xf
        raw>>=4
        self.tot=raw&0x3FF
        raw>>=10
        self.toa=raw&0x3FFF
        raw>>=14

        self.abs_toa= (self.ext_toa<<14) + self.toa - self.ftoa
        if not self.pileup_decode:
          self.abs_toa -= float(self.ftoa)/16
        self.abs_toa *= 25e-9

        if self.pileup_decode:
           self.pileup=self.ftoa

    elif Tpx3DataPacket.mode==2:
        #event count and iTOT
        raw=self.raw>>4
        self.event_counter=raw&0x3FF
        raw>>=10
        self.itot=raw&0x3FFF
        raw>>=14
        self.pixel_address=raw&0xFFFF
    else:
        pass

  def __repr__(self):
    self.str="-"
    if self.isData():
      if Tpx3DataPacket.mode==0:
        if self.pileup_decode:
            self.str+="(%3d,%3d) toa=%d tot=%d pileup=%d"%(self.col,self.row, self.toa,self.tot,self.pileup)
        else:
            self.str+="(%3d,%3d) toa=%d tot=%d ftoa=%d"%(self.col,self.row, self.toa,self.tot,self.ftoa)
      elif Tpx3DataPacket.mode==2:
        self.str+="(%3d,%3d)  evn_cnt=%d itot=%d"%(self.col,self.row, self.event_counter, self.itot)
    return "[%012X] %s"%(self.raw, self.str)


class TPX3:
  def __init__(self,ip):
    self.logging = logging.getLogger("tpx3")
    port=50000
    if len(ip.split(":")):
      port=int(ip.split(":")[1])
      ip=ip.split(":")[0]
    self.readout='seq'
    ip0,ip1,ip2,ip3=map(int,ip.split('.'))
    self.id=0
    self.ctrl=SpidrController( ip0,ip1,ip2,ip3,port)
    if  not self.ctrl.isConnected() :
      self.logging.critical("Unable to connect %s (%s)"%(self.ctrl.ipAddressString(), self.ctrl.connectionErrString()))
      raise RuntimeError("Unable to connect")
    #self.udp=UDPServer()
    #self.udp.start(8192)
    #if  not self.udp.isStarted():
#      raise RuntimeError("Problem with UDP server. Unable to connect")
    self.daq=SpidrDaq(self.ctrl, 1024*1024,self.id)
    time.sleep(0.01) #give some time for process scheduling !
    msg=str(self.daq.errorString())
    if msg!="":
         #self.connectrionMessage.setText(self.daq.errorString())
         self._isConected=False
         self._connectionErrString=self.daq.errorString()
         self.logging.critical("Unable to connect (%s)"%msg)
         raise RuntimeError("Unable to connect (%s)"%msg)
    self.daq.setSampling(True)
    self.daq.setSampleAll(True)


#    self.daq=SpidrDaq( self.ctrl )
#    self.daq.setFlush(False)
    self.log_packets=False
    self.dacs=numpy.zeros((256,256), int)
    self.tpena=numpy.zeros((256,256), int)
    self.flush_udp_fifo(val=0)
    self.timeouts=0
    self.bad_counters=0

    self.has_supply_monitors=False
    r,v,i,p=self.ctrl.getDvddNow()
    if r :
        self.has_supply_monitors=True

  def disconect(self):
      self.daq.stop
      del self.daq
      del self.ctrl

  def stop(self):
#    self.daq.stop()
    pass


  def _vdd2str(self,meas):
    r,v,i,p=meas
    v=float(v)/1000
    i=float(i)/10
    p=float(p)/1000
    #if p>1000:p=0
    return "%.3f V %.1f mA %.3f W"%(v,i,p)

  def get_dvdd_v(self):
    return float(self.ctrl.getDvdd()[1])/1000
  def get_dvdd_i(self):
    return float(self.ctrl.getDvdd()[2])/10000
  def get_dvdd_p(self):
    return float(self.ctrl.getDvdd()[3])/1000

  def get_avdd_v(self):
      r,vol=self.ctrl.getAvdd()
      return float(vol)/1000
  def get_avdd_i(self):
    return float(self.ctrl.getAvdd()[2])/10000
  def get_avdd_p(self):
    return float(self.ctrl.getAvdd()[3])/1000


  def get_dvdd_str(self):
    return self._vdd2str(self.ctrl.getDvddNow())
    
  def get_avdd_str(self):
    return self._vdd2str(self.ctrl.getAvddNow())
    
  def get_adc(self,measurements=1):
    vv=[]
    ret=True
#    for i in range(measurements):
    ret,val=self.ctrl.getAdc(measurements)
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

  def getClassVersion(self):
    v=self.ctrl.classVersion()
    return "%08X"%v

  def getSoftwVersion(self):
    r,v=self.ctrl.getSoftwVersion()
    return "%08X"%v

  def getFirmwVersion(self):
    r,v=self.ctrl.getFirmwVersion()
    return "%08X"%v

  def info(self):
      self.logging.info( "Controller IP       : %s"%self.ctrl.ipAddressString())
      ver=self.getSoftwVersion()
      self.logging.info( "Software Version    : 20%s/%s/%s (%s)"%(ver[0:2],ver[2:4],ver[4:6],ver[6:8]))
      ver=self.getFirmwVersion()
      self.logging.info( "Firmware Version    : 20%s/%s/%s (%s)"%(ver[0:2],ver[2:4],ver[4:6],ver[6:8]))
      ver=self.getClassVersion()
      self.logging.info( "Ctrl Class Version  : 20%s/%s/%s (%s)"%(ver[0:2],ver[2:4],ver[4:6],ver[6:8]))
      if "daq" in self.__dict__:
          ver="%08x"%self.daq.classVersion()
          self.logging.info( "DAQ Class version   : 20%s/%s/%s (%s)"%(ver[0:2],ver[2:4],ver[4:6],ver[6:8]))
          self.logging.info( "DAQ IP              : %s"%self.daq.ipAddressString())

      self.logging.info( "Chip temp.          : %s"%self.get_remote_temp())
      self.logging.info( "Chip card temp.     : %s"%self.get_local_temp())
      if self.has_supply_monitors:
          self.logging.info( "Digital Chip Supply : %s"%self.get_dvdd_str())
          self.logging.info( "Analog Chip Supply  : %s"%self.get_avdd_str())
      self.logging.info("Links speed         : %d Mbps"%self.getReadoutSpeed())
      self.logging.info("Links status        : 0x%02x "%self.getLinkStatus())

  def _send_raw(self, buf):
    pck=[0xaa,0x00,0x00,0x00,0x00]
    for b in buf:
      pck.append(b&0xFF)
    l=len(pck)
    if l>256:
      print "Too long packet !! Not sending"
      return
    if  self.log_packets:
      s='<['
      for i in range(l):
        s+="%02X"%pck[i]
      s+=']'
      self.logging.debug(s)
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
    # if val==0:
    #   self.udp.flush()
    # else:
    #   data = self.recv_mask(val,mask)
    #   for d in data:
    #     self.logging.debug("FLUSH : %s"%(str(d)))
    #
    pass

  def _log_ctrl_cmd(self,msg,result):
    if result:
      self.logging.debug("%-95s [  OK  ]"%msg)
    else:
      self.logging.error("%-95s [FAILED] (%s)"%(msg,self.ctrl.errorString()))

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

  def MaskPixels(self,l,mask=True):
    for c,r in l:
      self.setPixelMask(c,r,mask)

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

  def setGPIO(self, pin, state):
    r=self.ctrl.setGpioPin(pin,state)
    self._log_ctrl_cmd("setGPIO(%d,%d) "%(pin,state),r)


  def getGPIO(self, pin):
    r,state=self.ctrl.getGPIO(pin)
    self._log_ctrl_cmd("getGPIO(%d)=%d "%(pin,state),r)
    return state

  def sequentialReadout(self,tokens=2,now=False):
    self.readout='seq'
    r=self.ctrl.sequentialReadout(tokens,now=now)
#    self.presetFPGAFilters()
    self._log_ctrl_cmd("sequentialReadout(tokens=%d,now=%d) "%(tokens,now),r)
    
    
  def datadrivenReadout(self):
    self.readout='dd'
    r=self.ctrl.datadrivenReadout()
    self.presetFPGAFilters()
    self._log_ctrl_cmd("datadrivenReadout() ",r)

  def setShutterLen(self,l):
    self.shutter_len=float(l)/1e6
    r=self.ctrl.setShutterTriggerConfig(4,l,1,1)
    self._log_ctrl_cmd("Config shutter (%d) "%(l),r)
    
  def resetTimer(self):
    r=self.ctrl.resetTimer(self.id)
    self._log_ctrl_cmd("resetTimer() ",r)


  def resetDevice(self):
    r=self.ctrl.resetDevice(self.id)
    self._log_ctrl_cmd("resetDevice() ",r)

  def getLinkStatus(self):
    r,mask,links=self.ctrl.getLinkStatus(self.id)
    self._log_ctrl_cmd("getLinkStatus()=%02x,%02x"%(mask,links),r)
    return links

  def reinitDevice(self):
    mask=0
    links=1
    cnt=3
    while cnt>0 and mask!=links:
        r=self.ctrl.reinitDevice(self.id)
        self._log_ctrl_cmd("reinitDevice() ",r)
        r,mask,links=self.ctrl.getLinkStatus(self.id)
        if mask!=links:
            self.logging.warning("reinitDevice(): Links unlocked!")
            cnt-=1
    return cnt>0

  def setReadoutSpeed(self,speed):
    r=self.ctrl.setReadoutSpeed(self.id,speed)
    self._log_ctrl_cmd("setReadoutSpeed(%d) "%(speed),r)

  def getReadoutSpeed(self):
    r,OutBlockConfig=self.ctrl.getOutBlockConfig(self.id)
    clksrc=OutBlockConfig&TPX3_CLK_SRC_MASK
    clksrc>>=8
    r,PllConfig=self.ctrl.getPllConfig(self.id)
    DualEdgeClock=PllConfig&TPX3_DUALEDGE_CLK
    speed=0
    if clksrc==4:
        speed=40
    elif clksrc==3:
        speed=80
    elif clksrc==2:
        speed=160
    elif clksrc==1:
        speed=320
    if DualEdgeClock: speed*=2
    return speed


  def readEfuses(self):
    r,v=self.ctrl.getDeviceId(self.id)
    self._log_ctrl_cmd("readEfuses()=%08x"%v,r)
    return v

  def presetFPGAFilters(self):
    eth,cpu=self.getHeaderFilter()
    self.setHeaderFilter(eth|0x0C80,cpu)

  def readName(self):
    fuses=self.readEfuses()
    if fuses==0:
      return '-'
    x=fuses&0xF
    y=(fuses>>4)&0xF
    w=(fuses>>8)&0xFFF
    mod=(fuses>>20)&0x3
    mod_val=(fuses>>22)&0x3FF
    return self.generate_die_name(w,x,y,mod,mod_val)

  def chipID(self):
    def _generate_die_name( wno, x,y,mod=0,mod_val=0):
      if mod==1:
        x=mod_val&0xf
      elif mod==2:
        y=mod_val&0xf
      elif mod==3:
        wno=wno & ~(0x3FF)
        wno|=(mod_val&0x3FF)
      xs=chr(ord('A')+x-1)
      return "W%d_%s%d"%(wno,xs,y)

    r,fuses=self.ctrl.getDeviceId(self.id)
    if fuses==0:
      return '-'
    x=fuses&0xF
    y=(fuses>>4)&0xF
    w=(fuses>>8)&0xFFF
    mod=(fuses>>20)&0x3
    mod_val=(fuses>>22)&0x3FF
    return _generate_die_name(w,x,y,mod,mod_val)

  def decode_die_name(self, name):
     if len(name.split("_"))!=2 : 
       self.logging.error("Inforect die name")
       return 
     try:
       wnum=int(name.split("_")[0][1:])
       xs=name.split("_")[1][0]
       x=(ord(xs)-ord('A') +1)&0xf
       y=int(name.split("_")[1][1:])
       if (xs=='A' and y>=5 and y<=7) or\
           (xs=='B' and y>=4 and y<=9) or\
           (xs=='C' and y>=3 and y<=10) or\
           (xs=='D' and y>=2 and y<=10) or\
           (xs=='E' and y>=2 and y<=11) or\
           (xs=='F' and y>=1 and y<=11) or\
           (xs=='G' and y>=1 and y<=11) or\
           (xs=='H' and y>=1 and y<=11) or\
           (xs=='I' and y>=2 and y<=11) or\
           (xs=='J' and y>=2 and y<=10) or\
           (xs=='K' and y>=3 and y<=10) or\
           (xs=='L' and y>=4 and y<=9) or\
           (xs=='M' and y>=5 and y<=7) :
            return (wnum,x,y)
       else:
         self.logging.error("Inforect die name")
         return
     except:
       return None
       

  def generate_die_name(self, wno, x,y,mod=0,mod_val=0):
      if mod==1:
        x=mod_val&0xf
      elif mod==2:
        y=mod_val&0xf
      elif mod==3:
        wno=wno & ~(0x3FF)
        wno|=(mod_val&0x3FF)
      xs=chr(ord('A')+x-1)
      return "W%d_%s%d"%(wno,xs,y)

  def burnEfuse(self, selection,program_width=0):
    r=self.ctrl.burnEfuse(self.id,program_width,selection)
    self._log_ctrl_cmd("burnEfuse(%d,%d) "%(program_width,selection),r)

  def setOutputMask(self, mask=0xff):
    r=self.ctrl.setOutputMask(self.id,mask)
    self._log_ctrl_cmd("setOutputMask(0x%02x) "%(mask),r)

  def getOutputMask(self, mask=0xff):
    r,mask=self.ctrl.getOutputMask(self.id)
    self._log_ctrl_cmd("getOutputMask()=0x%02x"%(mask),r)
    return mask

  def openShutter(self,sleep=True):
    r=self.ctrl.startAutoTrigger()
#c2.add_method('stopAutoTrigger',      'bool',       [])
#    r=self.ctrl.openShutter(self.id,l)
    self._log_ctrl_cmd("Start shutter() ",r)
    if sleep:
      time.sleep(self.shutter_len)
    return r

  def shutterOn(self):
    r=self.ctrl.setShutterTriggerConfig(4,0,1,0)
    self._log_ctrl_cmd("setShutterTriggerConfig ",r)
    r=self.ctrl.startAutoTrigger()
    self._log_ctrl_cmd("Shutter on ",r)
    return r
    
  def shutterOff(self):
    r=self.ctrl.stopAutoTrigger()
    self._log_ctrl_cmd("Shutter off ",r)
    return r


  def t0Sync(self):
    r=self.ctrl.t0Sync(self.id)
    self._log_ctrl_cmd("t0Sync() ",r)
    
    
  def setLogLevel(self,lvl):
    r=self.ctrl.setLogLevel(lvl)
    self._log_ctrl_cmd("setLogLevel(%d) "%(lvl),r)

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
    tpx3packet_hp.mode=(l>>1)&0x3
    #if fast local oscilator is disabled we have to devode pileup counter
    if r :
      if l & TPX3_FASTLO_ENA :
        tpx3packet_hp.pileup_decode = False
      else:
        tpx3packet_hp.pileup_decode = True
    self.setDecodersEna(True)        
    self._log_ctrl_cmd("setGenConfig(%04x) "%(l),r)

  def setDecodersEna(self,enable=True):
    r=self.ctrl.setDecodersEna(enable)
    self._log_ctrl_cmd("setDecodersEna(%d) "%(enable),r)
    if r:
      tpx3packet.hw_dec_ena=enable
  
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


  def setSlvsConfig(self,l):
    r=self.ctrl.setSlvsConfig(self.id,l)
    self._log_ctrl_cmd("setSlvsConfig(%04x) "%(l),r)

  def getSlvsConfig(self):
    r,val=self.ctrl.getSlvsConfig(self.id)
    self._log_ctrl_cmd("getSlvsConfig()=%02x"%(val),r)
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

  def get_frame(self,timeout=10):
      res=self.daq.getFrame(timeout_ms=timeout)
      ret=[]
      while res:
          rawpacket=self.daq.nextPacket()
          if rawpacket==0: return ret
          #print col,row
          #if not res: break
          p=tpx3packet_hp(rawpacket)
          ret.append(p)
          #print "p",p.col

      return ret
    #if self.readout=='dd':
#      return self.recv_mask(0x71B0000000000000, 0xFFFF000000000000)
#    else:
#      return self.recv_mask(0x71A0000000000000, 0xFFFF000000000000)
  def recv_mask(self,val,mask,timeout=200):
      ret=[]
      res=1
      while res:
          res=self.daq.getSample(256*256*8*2,timeout_ms=timeout)
          while True:
            rawpacket=self.daq.nextPacket()
            if rawpacket==0:
                break
            p=tpx3packet_hp(rawpacket)
            ret.append(p)
            #yield p
            if (rawpacket&mask)==val:
              return ret
      return ret

  def recv_mask2(self,val,mask):
    ok=False
    cnt=2
    ret=[]
    last=0
    vomit=0
    ERRORS=(0,1)
    while cnt>0 and not ok:
      r=list(self.udp.getH(val,mask,debug=0))
      cnt-=1
      if len(r)==0: continue
      pck_num=r[-1]
      if pck_num in ERRORS:
        if pck_num ==0:
          self.logging.warning("Received 0x0 packet !")
          continue
        if pck_num==1:
          self.logging.warning("Chip is vomiting! (%d)"%len(ret))
          self.reinitDevice()
          print len(r), r[0], r[-1]
          vomit=1
          
      for pck_num in r:
        p=tpx3packet_hp(pck_num)
        ret.append(p)
        if self.log_packets : self.logging.info(p)
      last=r[-1]
      if (last&mask)==val: ok=True
      if vomit: break
    if not ok:
      self.timeouts+=1
      if self.timeouts<64:
        self.logging.warning("Timeout ;/ (last packet : %16X while expecting %016X)"%(last,val))
    return ret

  def get_N_packets(self,N):
     r=list(self.udp.getN(N,debug=0))
     ret=[]
     for pck_num in r:
       p=tpx3packet_hp(pck_num)
       ret.append(p)
     return ret

  def get_N_raw(self,N):
     r=list(self.udp.getN(N,debug=0))
     return r



  def __del__(self):
    pass

  def load_equalization(self,fname,maskname=""):
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
    eq=load(fname)
    if maskname : mask=load(maskname)
    self.resetPixelConfig()
    for x in range(256):
      for y in range(256):
          self.setPixelThreshold(x,y,eq[y][x])
          if maskname and mask[y][x]: 
            self.setPixelMask(x,y,mask[y][x])
            
  def getTpix3Temp(self):
    self.setSenseDac(TPX3_BANDGAP_TEMP)
    v_bg_temp=self.get_adc(64)
    self.setSenseDac(TPX3_BANDGAP_OUTPUT)
    v_bg=self.get_adc(64)
    return 88.75-607.3*(v_bg_temp-v_bg)     #Mpix3 extracted



  def setThreshold(self,dac_value=1000):
    """ Xavi's treshold """
    i=0
    coarse_found=0
    fine_found=352
    for coarse in range(16):
       for fine in range(352,512,1):
          if dac_value==i:
             coarse_found=coarse
             fine_found=fine
          i+=1
    self.setDac(TPX3_VTHRESH_COARSE,coarse_found)
    self.setDac(TPX3_VTHRESH_FINE,fine_found)

def main():
  tpx=TPX3()
  tpx.info()
  
if __name__=="__main__":
  main()


from PySide.QtCore import *
from PySide.QtGui import *
from SpidrTpx3_engine import *
import numpy as np
import time
from xml.etree import ElementTree
from xml.dom import minidom
from xml.etree.ElementTree import Element, SubElement, Comment
from kutils import *
import resources_rc


TPX3_VTHRESH = 32

class MySignal(QObject):
    sig = Signal(str)

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

SPIDR_DUMMYGEN_ENA=0x2
SPIDR_TPX_FE_CONFIG_I       = 0x0210

class Rate():
    def __init__(self,refresh=0.02,updateRateSignal=None,refreshDisplaySignal=None):
        self.refresh=refresh
        self.total_events=0
        self.new_events=0
        self.last_ref_time=time.time()
        self.last_s_time=self.last_ref_time
        self.updateRateSignal=updateRateSignal
        self.refreshDisplaySignal=refreshDisplaySignal

    def processed(self, events):
        now=time.time()
        self.new_events+=events

        # refresh
        dt=now-self.last_ref_time
        if dt>self.refresh and self.refreshDisplaySignal:
            self.last_ref_time=now
            self.refreshDisplaySignal.sig.emit("Now")
        # report rate
        dt=now-self.last_s_time
        if dt>1.0 and self.updateRateSignal:
            rate=self.new_events/dt
            self.total_events+=self.new_events
            self.last_s_time=now
            self.new_events=0
            self.updateRateSignal.sig.emit("%.3f"%rate)

DISMODE_DECAY=0
DISMODE_INT=1
DISMODE_OVERWRITE=2

class DaqThread(QThread):
    def __init__(self, parent=None):
        QThread.__init__(self)
        self.parent=parent
        self.abort = False
        self.updateRate = MySignal()
        self.refreshDisplay = MySignal()
        self.rate=Rate(refresh=0.05,updateRateSignal=self.updateRate, refreshDisplaySignal=self.refreshDisplay)
        self.displayMode=0
        self.decayVal=0.98
        self.__clear=False

    def stop(self):
        self.abort = True
        self.parent.pauseReadout()
        self.parent.resetPixels()
        print "Stoping DAQ thread"

    def __del__(self):
        print "Wating ..."
        self.wait()
    def clear(self):
        self.__clear=True
    def run(self):
        print "Starting DAQ thread"
        #prev_ref=0
        #msg=""
        self.abort=False
        self.parent.resetPixels()
        self.parent.datadrivenReadout()

        tot_frame =np.zeros( (256,256) , dtype =np.int)
        hits_frame=np.zeros( (256,256) , dtype =np.int)
        toa_frame =np.zeros( (256,256) , dtype =np.int)

        while True:
            if self.__clear:
                for x in range(self.parent.matrixTOT.shape[0]):
                    for y in range(self.parent.matrixTOT.shape[1]):
                        self.parent.matrixTOT[x,y]=0
                        self.parent.matrixTOA[x,y]=0
                        self.parent.matrixCounts[x,y]=0
                self.__clear=False

            if self.abort:
                return
            if self.displayMode==DISMODE_DECAY:
                self.parent.matrixTOT*=self.decayVal
                low_values_indices = self.parent.matrixTOT < 1.0  # Where values are low
                self.parent.matrixTOT[low_values_indices] = 0  # All low values set to 0xzcxzczxczxc

            next_frame=self.parent.getSample(1024*16,10)
            self.rate.processed(0)
            if next_frame:
               if 1:
                       hits_processed=self.parent.daq.getNumpyFrames(tot_frame,hits_frame,toa_frame)
                       #hits_processed=0
                       if self.displayMode==DISMODE_OVERWRITE:
                          nonzero_indices = tot_frame > 0
                          self.parent.matrixTOT[nonzero_indices] = 0
                          self.parent.matrixTOT += tot_frame
                       else:
                           self.parent.matrixTOT+=tot_frame

                       nonzero_indices = toa_frame > 0
                       self.parent.matrixTOA[nonzero_indices] = 0
                       self.parent.matrixTOA+=toa_frame
                       self.parent.matrixCounts+=hits_frame
                       hits_processed+=hits_processed

               else:
                   time.sleep(0.005)
                   hits_processed=0
                   while True:
                       r,x,y,d,tstp=self.parent.nextPixel()
                       if not r: break
                       tot=(d>>4)&0x3FF
                       toa=((d>>10)&0x3FFF0 - (d&0xF)) | (tstp<<18)
                       #print "%08x"%d,tot,toa
                       if self.displayMode==DISMODE_OVERWRITE:
                          self.parent.matrixTOT[x,y]=tot
                       else:
                           self.parent.matrixTOT[x,y]+=tot
                       self.parent.matrixTOA[x,y]=toa
                       self.parent.matrixCounts[x,y]+=1
                       hits_processed+=1
               self.rate.processed(hits_processed)

class MyUDPServer:
    def __init__(self):
        self.udp=UDPServer()
        self.udp.start(8192)
        if  not self.udp.isStarted():
            raise RuntimeError("Problem with UDP server. Unable to connect")

        self.data=[]

    def getSample2(self,size,timeout):
        r=list(self.udp.getN2(size,0))
        self.data.extend(r)
        return len(self.data)!=0
#        return r

    def nextPixel(self):
        while True:
            if len(self.data)==0:
                return (False,0,0,0,0)
            raw=self.data.pop(0)
            raw0=raw
            toa=raw&0xFFFF
            raw>>=16
            data=raw&0xFFFFFFF
            raw>>=28
            address=raw&0xFFFF
            x,y=address_loopup_list[address]
            raw>>=16
            hdr=raw&0xF
            #raw>>=4
            if hdr in (0xa,0xb):
                return True,x,y,data,toa
#            else:
#                print "%16X"%raw0, hdr


    def getFrame(self,timeoutms=10):
        tries=1000
        mask=0xFFFF000000000000
        val=0x71A0000000000000
        while tries>0:
            r=list(self.udp.getH(val, mask,debug=0))
            if len(r)==0:continue
            last=r[-1]
            self.data.extend(r)
            if (last&mask)==val:
#                print "%16X"%last, len(self.data)
                return True

        return False

    def flush_udp_fifo(self,val,mask):
        if val==0:
            self.udp.flush()
        else:
            self.udp.getH(val,mask,debug=0)

class TPX3:

  def __init__(self,ip="192.168.100.10",port=50000,daq="spidr"):
    port=int(port)
    if len(ip.split(":"))>1:
      port=int(ip.split(":")[1])
      ip=ip.split(":")[0]
    self.readout='seq'
    ip0,ip1,ip2,ip3=map(int,ip.split('.'))

    self.ctrl=SpidrController( ip0,ip1,ip2,ip3,port)
    self._isConected=True
    if  not self.ctrl.isConnected() :
      self._isConected=False
      print ("Unable to connect %s (%s)"%(self.ctrl.ipAddressString(), self.ctrl.connectionErrString()))
      raise RuntimeError("Unable to connect (%s)"%str(self.ctrl.connectionErrString()))
    self._connectionStateString = self.ctrl.connectionStateString()
    self._connectionErrString=self.ctrl.connectionErrString()
#    self.udp=UDPServer()
#    self.udp.start(8192)
#    if  not self.udp.isStarted():
#      raise RuntimeError("Problem with UDP server. Unable to connect")

    self.shutterOff()
    self.setDummyGen(0)

    self.id=0
    self.log_packets=False
    self.dacs=np.zeros((256,256), int)
    self.tpena=np.zeros((256,256), int)
    self.mask=np.zeros((256,256), int)
    if daq=="spidr":
        self.daq=SpidrDaq(self.ctrl, 1024*1024*512,self.id)
        msg=str(self.daq.errorString())
        if msg!="":
           #self.connectrionMessage.setText(self.daq.errorString())
           self._isConected=False
           self._connectionErrString=self.daq.errorString()
           raise RuntimeError("Unable to connect (%s)"%msg)
        self.daq.setSampling(True)
        self.daq.setSampleAll(True)
    elif daq=="custom":
        self.daq=MyUDPServer()
    else:
        self.daq=None

    self.matrixTOT    = np.zeros( shape=(256,256))
    self.matrixTOA    = np.zeros( shape=(256,256))
    self.matrixCounts = np.zeros( shape=(256,256))
    self.matrixMask   = np.zeros( shape=(256,256))
    self.matrixDACs   = np.zeros( shape=(256,256))

    self.matrixMaskNeedUpdate = False
    self.matrixDACsNeedUpdate = False

    if self.daq!=None:
        self.daqThread = DaqThread(self)

#    self.reinitDevice()
#    self.flush_udp_fifo(val=0)

  def isConnected(self):
      return  self._isConected

  def connectionStateString(self):
      return self.ctrl.connectionStateString()


  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
  # Shutter control
  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

  def setShutterConfig(self, len,freq,cnt):
      len=int(len)
      freq=int(freq)
      cnt=int(cnt)
      r=self.ctrl.setShutterTriggerConfig(4,len,freq,cnt)
      self._log_ctrl_cmd("setShutterConfig (%d,%d,%d) "%(len,freq,cnt),r)

  def setShutterLen(self,l):
    self.shutter_len=float(l)/1e6
    r=self.ctrl.setShutterTriggerConfig(4,l,1,1)
    self._log_ctrl_cmd("Config shutter (%d) "%(l),r)

  def openShutter(self,sleep=True):
    r=self.ctrl.startAutoTrigger()
    self._log_ctrl_cmd("Start shutter() ",r)
    if sleep:
      time.sleep(self.shutter_len)
    return r

  def shutterStart(self):
    r=self.ctrl.startAutoTrigger()
    self._log_ctrl_cmd("Shutter on ",r)
    return r


  def shutterOn(self):
    r=self.ctrl.setShutterTriggerConfig(4,0,1,0)
    self._log_ctrl_cmd("setTriggerConfig ",r)
    r=self.ctrl.startAutoTrigger()
    self._log_ctrl_cmd("Shutter on ",r)
    return r

  def shutterOff(self):
    r=self.ctrl.stopAutoTrigger()
    self._log_ctrl_cmd("Shutter off ",r)
    return r




  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
  # Registers
  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

  def setGenConfig(self,l):
    r=self.ctrl.setGenConfig(self.id,l)
    # tpx3packet_hp.mode=(l>>1)&0x3
    # #if fast local oscilator is disabled we have to devode pileup counter
    # if r :
    #   if l & TPX3_FASTLO_ENA :
    #     tpx3packet_hp.pileup_decode = False
    #   else:
    #     tpx3packet_hp.pileup_decode = True
    self._setDecodersEna()
    self._log_ctrl_cmd("setGenConfig(%04x) "%(l),r)

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

  def setOutBlockConfig(self,l):
    r=self.ctrl.setOutBlockConfig(self.id,l)
    self._log_ctrl_cmd("setOutBlockConfig(%04x) "%(l),r)

  def getOutBlockConfig(self):
    r,val=self.ctrl.getOutBlockConfig(self.id)
    self._log_ctrl_cmd("getOutBlockConfig()=%02x"%(val),r)
    return val

  def setOutputMask(self, mask=0xff):
    r=self.ctrl.setOutputMask(self.id,mask)
    self._log_ctrl_cmd("setOutputMask(0x%02x) "%(mask),r)

  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
  # DACS
  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

  def setSenseDac(self,code):
    r=self.ctrl.setSenseDac(self.id,code)
    self._log_ctrl_cmd("setSenseDac(%d) "%(code),r)

  def setDac(self,code,val):
    if code<32:
        r=self.ctrl.setDac(self.id,code,val)
        self._log_ctrl_cmd("setDac(%d,%d) "%(code,val),r)
    elif code==TPX3_VTHRESH:
        threshold=val
        coarse=int(threshold/160)
        fine=threshold-coarse*160+352
        r=self.ctrl.setDac(self.id,TPX3_VTHRESH_FINE,fine)
        r=self.ctrl.setDac(self.id,TPX3_VTHRESH_COARSE,coarse)
        self._log_ctrl_cmd("setDac(%d,%d) "%(code,val),r)
    else:
        pass

  def getDac(self,code):
    if code<32:
        r,v=self.ctrl.getDac(self.id,code)
        self._log_ctrl_cmd("getDac(%d)=%d "%(code,v),r)
    elif code==TPX3_VTHRESH:
        r,coarse=self.ctrl.getDac(self.id,TPX3_VTHRESH_COARSE)
        r,fine=self.ctrl.getDac(self.id,TPX3_VTHRESH_FINE)
        v=coarse*160 +(fine-352)
    else:
        v=0
        pass

    return v

  def setDacsDflt(self):
    r=self.ctrl.setDacsDflt(self.id)
    self._log_ctrl_cmd("setDacsDflt()",r)

  def dacMax(self,code):
    if code<32:
        return self.ctrl.dacMax(code)
    elif code==TPX3_VTHRESH:
        return 2559
    else:
        return 0

  def getAdcEx(self,measurements):
      ret,val=self.ctrl.getAdc(measurements)
      val=float(val)/measurements
      val=1.5*val/4096
      return val

  def getDacVoltage(self,code,loop=32):
        self.setSenseDac(code)
        v=self.getAdcEx(1)
        time.sleep(0.001)
        v=self.getAdcEx(32)
        return v

  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
  # DAQ
  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

  def getFrame(self,timeoutms=10):
    return self.daq.getFrame(timeoutms)

  def getSample(self,size,timeout=10):
    return self.daq.getSample2(size,timeout)

  def nextPixel(self):
    return self.daq.nextPixel()

  def pauseReadout(self):
    r=self.ctrl.pauseReadout()
    self._log_ctrl_cmd("pauseReadout() ",True)

  def getHeaderFilter(self):
    r,eth,cpu=self.ctrl.getHeaderFilter(self.id)
    self._log_ctrl_cmd("getHeaderFilter()=0x%04X,0x%04X"%(eth,cpu),r)
    return eth,cpu

  def setHeaderFilter(self,eth,cpu):
    r=self.ctrl.setHeaderFilter(self.id,eth,cpu)
    self._log_ctrl_cmd("setHeaderFilter(0x%04X,0x%04X)"%(eth,cpu),r)

  def _presetFPGAFilters(self):
    eth,cpu=self.getHeaderFilter()
    self.setHeaderFilter(eth|0x0C80,cpu)

  def sequentialReadout(self,tokens=2):
    self.readout='seq'
    r=self.ctrl.sequentialReadout(tokens)
    self._presetFPGAFilters()
    self._log_ctrl_cmd("sequentialReadout(tokens=%d) "%tokens,r)

  def datadrivenReadout(self):
    self.readout='dd'
    r=self.ctrl.datadrivenReadout()
    self._presetFPGAFilters()
    self._log_ctrl_cmd("datadrivenReadout() ",r)

  def flush_udp_fifo(self,val=0x1234000000000000, mask=0xFFFF000000000000):
    self.daq.flush_udp_fifo(val,mask)
  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
  # Test pulses
  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

  def setTpPeriodPhase(self,period, phase):
    r=self.ctrl.setTpPeriodPhase(self.id,period,phase)
    self._log_ctrl_cmd("setTpPeriodPhase(%d,%d) "%(period,phase),r)

  def setTpNumber(self,number):
    r=self.ctrl.setTpNumber(self.id,number)
    self._log_ctrl_cmd("setTpNumber(%d) "%(number),r)

  def setCtprBit(self,column,val):
    r=self.ctrl.setCtprBit(column,val)
    self._log_ctrl_cmd("setCtprBit(%d,%d) "%(column,val),r)

  def setCtprBits(self,val):
    r=self.ctrl.setCtprBits(val)
    self._log_ctrl_cmd("setCtprBits(%d) "%(val),r)


  def setCtpr(self):
    r=self.ctrl.setCtpr(self.id)
    self._log_ctrl_cmd("setCtpr() ",r)

  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
  # Dummy generator
  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

  def setDummyGen(self,enable,delay=0,frames=1,header=0xA):
      val=0
      if enable:
          val|=SPIDR_DUMMYGEN_ENA
      val|= delay<<2
      val|= frames<<(2+8)
      val|= header<<(2+8+8)
      r=self.ctrl.setSpidrReg( SPIDR_TPX_FE_CONFIG_I,val)

  def getDummyGen(self):
      r,v=self.ctrl.getSpidrReg( SPIDR_TPX_FE_CONFIG_I)
      enable =0
      delay=0
      frames=0
      if r:
        if val&SPIDR_DUMMYGEN_ENA : enable=True
        delay=(val>>2)&0xFF
        frames=(val>>(2+10))&0xFF
      return enable,delay,frames


  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
  # Matrix configuration
  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

  def setPixelThreshold(self,x,y,dac):
    r=self.ctrl.setPixelThreshold(x,y,dac)
    if x<ALL_PIXELS and y< ALL_PIXELS:
        self.matrixDACs[x][y]=dac
    else:
        for x in range(ALL_PIXELS):
          for y in range(ALL_PIXELS):
            self.matrixDACs[x][y]=dac

  def setPixelTestEna(self,x,y, testbit=False):
    r=self.ctrl.setPixelTestEna(x,y, testbit)

  def resetPixelConfig(self):
    r=self.ctrl.resetPixelConfig()
    self._log_ctrl_cmd("resetPixelConfig() ",True)
    for x in range(256):
        for y in range(256):
            self.matrixMask[x][y]=0
            self.matrixDACs[x][y]=0

  def setPixelMask(self,x,y,v):
    r=self.ctrl.setPixelMask(x,y,v)
    if x<ALL_PIXELS and y< ALL_PIXELS:
        self.matrixMask[x][y]=v
    else:
        for x in range(ALL_PIXELS):
          for y in range(ALL_PIXELS):
            self.matrixMask[x][y]=v
 

  def MaskPixels(self,l,mask=True):
    for c,r in l:
      self.setPixelMask(c,r,mask)

  def setPixelConfig(self):
    restart=0

    if self.daq!=None and self.daqThread.isRunning():
        self.daqThread.stop()
        self.daqThread.wait()
        restart=1
    r=self.ctrl.setPixelConfig(self.id, cols_per_packet=2)
    if restart:
        self.daqThread.start()

    self._log_ctrl_cmd("setPixelConfig() ",r)

  def getPixelConfig(self):
    r=self.ctrl.getPixelConfig(self.id)
    self._log_ctrl_cmd("getPixelConfig() ",r)

  def resetPixels(self):
    r=self.ctrl.resetPixels(self.id)
    self._log_ctrl_cmd("resetPixels() ",r)

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

  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
  # Misc
  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #


  def _setDecodersEna(self,enable=True):
    r=self.ctrl.setDecodersEna(enable)
    self._log_ctrl_cmd("setDecodersEna(%d) "%(enable),r)
    #if r:
    #  tpx3packet.hw_dec_ena=enable
  def getClassVersion(self):
    v=self.ctrl.classVersion()
    return "%08X"%v

  def getSoftwVersion(self):
    r,v=self.ctrl.getSoftwVersion()
    return "%08X"%v

  def getFirmwVersion(self):
    r,v=self.ctrl.getFirmwVersion()
    return "%08X"%v

  def getLinkSpeed(self):
    r,v=self.ctrl.getOutBlockConfig(self.id)
    v&=TPX3_CLK_SRC_MASK
    v>>=8
    speed="- Mb/s"
    if v==3:
        speed="160 Mb/s"
    elif v==1:
        speed="640 Mb/s"
    return speed

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
    print "%08x"%fuses
    x=fuses&0xF
    y=(fuses>>4)&0xF
    w=(fuses>>8)&0xFFF
    mod=(fuses>>20)&0x3
    mod_val=(fuses>>22)&0x3FF
    return _generate_die_name(w,x,y,mod,mod_val)


  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
  # Logs
  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

  def _log_ctrl_cmd(self,msg,result):
    if result:
#      logging.debug("%-80s [  OK  ]"%msg)
      #print("%-80s [  OK  ]"%msg)
      pass
    else:
#      logging.error("%-80s [FAILED] (%s)"%(msg,self.ctrl.errorString()))
      print("%-80s [FAILED] (%s)"%(msg,self.ctrl.errorString()))
      #  pass

  def connectionStateString(self):
      return self._connectionStateString

  def connectionErrString(self):
      return self._connectionErrString


  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
  # Loading saving conf
  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

  def loadConfiguration(self,fname):
    print("Load config from %s"%fname)
    xmldom=parse(fname)
    all_dacs=["TPX3_IBIAS_PREAMP_ON","TPX3_IBIAS_PREAMP_OFF","TPX3_VPREAMP_NCAS","TPX3_IBIAS_IKRUM","TPX3_VFBK",\
              "TPX3_VTHRESH","TPX3_IBIAS_DISCS1_ON","TPX3_IBIAS_DISCS1_OFF",\
              "TPX3_IBIAS_DISCS2_ON","TPX3_IBIAS_DISCS2_OFF","TPX3_IBIAS_PIXELDAC","TPX3_IBIAS_TPBUFIN",\
              "TPX3_IBIAS_TPBUFOUT","TPX3_VTP_COARSE","TPX3_VTP_FINE"]
    root=xmldom.getElementsByTagName('Timepix3')
    if root[0].getElementsByTagName('registers'):
        for n in root[0].getElementsByTagName('registers')[0].getElementsByTagName("reg"):
            reg_name=n.attributes['name'].value
            reg_val=int(n.attributes['value'].value,0)
#            print reg_name,reg_val
            dn=("TPX3_"+reg_name).upper()
            if dn in all_dacs:
                self.setDac(eval(dn),reg_val)
    codes=root[0].getElementsByTagName('codes')[0]
    if codes:
       self.dacsFromString(codes.firstChild.nodeValue)

    mask=root[0].getElementsByTagName('mask')[0]
    if codes:
       self.maskFromString(mask.firstChild.nodeValue)
    self.setPixelConfig()


  def saveConfiguration(self,fname):
    print("Save config to %s"%fname)
    root = Element("Timepix3")
    info = SubElement(root, "info")
    time_now = SubElement(info, "time")
    time_now.set("time", get_date_time())
    user = SubElement(info, "user")
    user.set("user", get_user_name())
    host = SubElement(info, "host")
    host.set("host", get_host_name())

    regs = SubElement(root, "registers")
    all_dacs=["TPX3_IBIAS_PREAMP_ON","TPX3_IBIAS_PREAMP_OFF","TPX3_VPREAMP_NCAS","TPX3_IBIAS_IKRUM","TPX3_VFBK",\
              "TPX3_VTHRESH","TPX3_IBIAS_DISCS1_ON","TPX3_IBIAS_DISCS1_OFF",\
              "TPX3_IBIAS_DISCS2_ON","TPX3_IBIAS_DISCS2_OFF","TPX3_IBIAS_PIXELDAC","TPX3_IBIAS_TPBUFIN",\
              "TPX3_IBIAS_TPBUFOUT","TPX3_VTP_COARSE","TPX3_VTP_FINE"]
    for dac in all_dacs:
      reg = SubElement(regs, "reg")
      reg.set("name", dac[5:])
      reg.set("value", "%d"%self.getDac(eval(dac)))

    reg = SubElement(regs, "reg")
    reg.set("name", "GeneralConfig")
    reg.set("value", "0x%08x"%self.getGenConfig())

    reg = SubElement(regs, "reg")
    reg.set("name", "PllConfig")
    reg.set("value", "0x%08x"%self.getPllConfig())

    reg = SubElement(regs, "reg")
    reg.set("name", "OutputBlockConfig")
    reg.set("value", "0x%08x"%self.getOutBlockConfig())

    codes_se = SubElement(root, "codes")
    mask_se  = SubElement(root, "mask")
    codes_se.text=self.dacsToString()
    mask_se.text=self.maskToString()

    #time_now.text = "some vale1"
    f=open(fname,"w")
    f.write(prettify(root))
    f.close()

  def _matrixToStr(self,m):
    m_str=""
    for y in range(m.shape[1]):
        for x in range(m.shape[0]):
            m_str+="%d "%m[x][y]
        m_str+="\n"
    return m_str

  def _strToMatrix(self,s,fn):
    x,y=0,0
    for v in s.split():
      v=int(v)
      fn(x,y,v)
      x+=1
      if x==256:
         y+=1
         x=0


  def maskToString(self):
    return self._matrixToStr(self.matrixMask)

  def dacsToString(self):
    return self._matrixToStr(self.matrixDACs)

  def dacsFromString(self,s):
    self._strToMatrix(s,self.setPixelThreshold)

  def maskFromString(self,s):
    self._strToMatrix(s,self.setPixelMask)


import random


A1=((0,-3,27), (1,-3,20), (2,-3,14), (-2,-2,34),
(-1,-2,40), (0,-2,46), (1,-2,43), (2,-2,44),
(3,-2,3), (-3,-1,20), (-2,-1,46), (-1,-1,55),
(0,-1,80), (1,-1,82), (2,-1,38), (3,-1,38),
(4,-1,13), (-3,0,35), (-2,0,41), (-1,0,90),
(0,0,114), (1,0,123), (2,0,87), (3,0,44),
(4,0,2), (-3,1,35), (-2,1,42), (-1,1,96),
(0,1,136), (1,1,140), (3,1,48), (4,1,9),
(-3,2,11), (-2,2,43), (-1,2,73), (0,2,97),
(1,2,99), (3,2,41), (-2,3,24), (-1,3,37),
(0,3,35), (1,3,34), (2,3,41), (3,3,20),
(-1,4,8), (0,4,24), (1,4,11),)
A2=((-1,-2,19), (0,-2,41), (1,-2,29), (2,-2,19),
(-2,-1,2), (-1,-1,42), (0,-1,45), (1,-1,39),
(2,-1,36), (-2,0,25), (-1,0,63), (0,0,107),
(1,0,95), (2,0,37), (-2,1,24), (-1,1,79),
(0,1,123), (1,1,98), (2,1,38), (-2,2,28),
(-1,2,40), (0,2,68), (1,2,61), (2,2,42),
(-1,3,17), (0,3,34), (1,3,34), (2,3,23),)
A3=((0,-2,11), (-2,-1,4), (-1,-1,44), (0,-1,82),
(1,-1,45), (-2,0,15), (-1,0,61), (0,0,96),
(1,0,60), (2,0,12), (-2,1,5), (-1,1,23),
(0,1,40), (1,1,24), (2,1,4), (-1,2,5),
(0,2,5),)

ALPHAS=[A1,A2,A3]



class DummyDaq:
    def __init__(self):
        self.name="Dummy"
        self.n=0
        self.x=0
        self.y=0
        self.data=[]
        self.rate=10
        self.n=1
        self.mode=0
        self.img=QPixmap(":/img/homer_simpson_xray.jpg").toImage()
        #self.img.

    def errorString(self):
        return ""

    def setSampling(self,b=False):
        pass

    def setSampleAll(self,b=False):
        pass

    def getSample2(self,n,timeout):
        return self.getSample(n,timeout)

    def getSample(self,n,timeout):

        def line(x0, y0, x1, y1):
            "Bresenham's line algorithm"
            dx = abs(x1 - x0)
            dy = abs(y1 - y0)
            x, y = x0, y0
            sx = -1 if x0 > x1 else 1
            sy = -1 if y0 > y1 else 1
            if dx > dy:
                err = dx / 2.0
                while x != x1:
                    yield (x, y)
                    err -= dy
                    if err < 0:
                        y += sy
                        err += dx
                    x += sx
            else:
                err = dy / 2.0
                while y != y1:
                    yield (x, y)
                    err -= dx
                    if err < 0:
                        x += sx
                        err += dy
                    y += sy
            yield (x, y)
        time.sleep(float(timeout)/1e3)
        self.n+=self.rate+random.randint(-self.rate/5,self.rate/5)


        while int(self.n)>0:

            if self.mode==0:
              if self.n>0:
                for i in range(int(self.n)):
                    px=random.randint(0,255)
                    py=random.randint(0,255)
                    v=self.img.pixel (px,py)
                    v&=0xff
                    r=random.randint(0,256)
                    if r>v:
                        self.data.append( (px,py,random.randint(10,50)))
                        self.n-=1
            elif self.mode==2:

                aid=random.randint(0,len(ALPHAS)-1)
                scale=random.random()*2
                if scale<0.5:scale=0.5
                x0=random.randint(0,255)
                y0=random.randint(0,255)
                for dx,dy,v in ALPHAS[aid]:
                  self.n-=1
                  x=x0+dx
                  if x<0 or x>255: continue
                  y=y0+dy
                  if y<0 or y>255: continue
                  amp=int(scale*v)
                  self.data.append( (x,y,amp))

            elif self.mode==1:
                px1=random.randint(2,253)
                py1=random.randint(2,253)
                py2=py1+60-random.randint(0,40)
                if py2>255:py2=255
                px2=px1+10-random.randint(0,20)
                px2=max((min((px2,255)) , 0))
                amp=random.randint(50,500)
                for x,y in line(px1,py1,px2,py2):
                  self.data.append( (x,y,amp))
                  amp+=random.randint(-10,40)
                  self.n-=1

        if len(self.data)>0:
            return True
        else:
            return False

    def sampleSize(self):

        return len(self.data)*8

    def nextPixel(self):
        if len(self.data)>0:
            x,y,d=self.data.pop(0)
            return True,x,y,d<<4,0
        else:
            return False,0,0,0,0



class DummyTPX3:
    def __init__(self):
        self.matrixTOT    = np.zeros( shape=(256,256))
        self.matrixCounts = np.zeros( shape=(256,256))
        self.matrixMask   = np.zeros( shape=(256,256))
        self.matrixDACs   = np.zeros( shape=(256,256))

        self.matrixMaskNeedUpdate = False
        self.matrixDACsNeedUpdate = False

        self.daq=DummyDaq()

        self.daqThread = DaqThread(self)
        self.daqThread.data=self.matrixTOT
    def resetPixels(self):
        pass
    def isConnected(self):
        return True
    def datadrivenReadout(self):
        pass
    def pauseReadout(self):
        pass
    def getFrame(self,timeoutms=1):
        r=self.daq.getFrame()
        self._log_ctrl_cmd("getFrame(%d) "%(timeoutms),r)
        return r

    def getSample(self,size,timeout=10):
        return self.daq.getSample2(size,timeout)
    def shutterOff(self):
        pass
    def nextPixel(self):
        return self.daq.nextPixel()

    def setMode(self,m):
        self.daq.mode=m
        self.matrixTOT [:,:]= 0
        self.matrixCounts[:,:]= 0

    def setRate(self,r):
        self.daq.rate=int(r)

from SpidrTpx3_engine import *
import numpy as np
import time

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


    def getFrame(self):
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
      logging.critical("Unable to connect %s (%s)"%(self.ctrl.ipAddressString(), self.ctrl.connectionErrString()))
      raise RuntimeError("Unable to connect (%s)"%str(self.ctrl.connectionErrString()))

    self._connectionStateString = self.ctrl.connectionStateString()
    self._connectionErrString=self.ctrl.connectionErrString()
#    self.udp=UDPServer()
#    self.udp.start(8192)
#    if  not self.udp.isStarted():
#      raise RuntimeError("Problem with UDP server. Unable to connect")

    self.id=0
    self.log_packets=False
    self.dacs=np.zeros((256,256), int)
    self.tpena=np.zeros((256,256), int)
    self.mask=np.zeros((256,256), int)
    if daq=="spidr":
        self.daq=SpidrDaq(self.ctrl, 1024*1024,self.id)
        msg=str(self.daq.errorString())

        if msg!="":
           #self.connectrionMessage.setText(self.daq.errorString())
           self._isConected=False
           self._connectionErrString=self.daq.errorString()
           raise RuntimeError("Unable to connect (%s)"%msg)
        self.daq.setSampling(True)
        self.daq.setSampleAll(True )
    else:
        self.daq=MyUDPServer()
#    self.reinitDevice()
#    self.flush_udp_fifo(val=0)

  def isConnected(self):
      return  self._isConected

  def connectionStateString(self):
      return self.ctrl.connectionStateString()


  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
  # Shutter control
  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

  def setShutterLen(self,l):
    self.shutter_len=float(l)/1e6
    r=self.ctrl.setTriggerConfig(4,l,1,1)
    self._log_ctrl_cmd("Config shutter (%d) "%(l),r)

  def openShutter(self,sleep=True):
    r=self.ctrl.startAutoTrigger()
    self._log_ctrl_cmd("Start shutter() ",r)
    if sleep:
      time.sleep(self.shutter_len)
    return r

  def shutterOn(self):
    r=self.ctrl.setTriggerConfig(4,0,1,0)
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
    r=self.ctrl.setDac(self.id,code,val)
    self._log_ctrl_cmd("setDac(%d,%d) "%(code,val),r)

  def getDac(self,code):
    r,v=self.ctrl.getDac(self.id,code)
    self._log_ctrl_cmd("getDac(%d)=%d "%(code,v),r)
    return v

  def setDacsDflt(self):
    r=self.ctrl.setDacsDflt(self.id)
    self._log_ctrl_cmd("setDacsDflt()",r)

  def dacMax(self,code):
    return self.ctrl.dacMax(code)

  def getAdcEx(self,measurements):
      ret,val=self.ctrl.getAdc(0,measurements)
      val=float(val)/measurements
      val=1.5*val/4096
      return val


  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
  # DAQ
  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

  def getFrame(self):
    return self.daq.getFrame()

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
  # Matrix configuration
  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

  def setPixelThreshold(self,x,y,dac):
    r=self.ctrl.setPixelThreshold(x,y,dac)

  def setPixelTestEna(self,x,y, testbit=False):
    r=self.ctrl.setPixelTestEna(x,y, testbit)

  def resetPixelConfig(self):
    r=self.ctrl.resetPixelConfig()
    self._log_ctrl_cmd("resetPixelConfig() ",True)

  def setPixelMask(self,x,y,v):
    r=self.ctrl.setPixelMask(x,y,v)

  def MaskPixels(self,l,mask=True):
    for c,r in l:
      self.setPixelMask(c,r,mask)

  def setPixelConfig(self):
    r=self.ctrl.setPixelConfig(self.id, cols_per_packet=2)
    self._log_ctrl_cmd("setPixelConfig() ",r)

  def getPixelConfig(self):
    r=self.ctrl.getPixelConfig(self.id)
    self._log_ctrl_cmd("getPixelConfig() ",r)

  def resetPixels(self):
    r=self.ctrl.resetPixels(self.id)
    self._log_ctrl_cmd("resetPixels() ",r)

  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
  # Misc
  # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #


  def _setDecodersEna(self,enable=True):
    r=self.ctrl.setDecodersEna(enable)
    self._log_ctrl_cmd("setDecodersEna(%d) "%(enable),r)
    #if r:
    #  tpx3packet.hw_dec_ena=enable

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
      #print("%-80s [FAILED] (%s)"%(msg,self.ctrl.errorString()))
        pass

  def connectionStateString(self):
      return self._connectionStateString

  def connectionErrString(self):
      return self._connectionErrString

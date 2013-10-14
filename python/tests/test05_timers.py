from tpx3_test import tpx3_test
import random
import time

class test05_timers(tpx3_test):
  """Data Links and periphery timers"""

  def _execute(self):
    #reset timer
    self.tpx.send_byte_array([0x40,0,0])
    resp=self.tpx.recv_mask(0x4071,0xFFFF)
    if len(resp)>1:
      for p in resp:
        print p

    #software t0 sync
    self.tpx.send_byte_array([0x4A,0,0])
    resp=self.tpx.recv_mask(0x4A71,0xFFFF)
    if len(resp)>1:
      for p in resp:
        print p
        
    time.sleep(0.1)
    #load ctpr
    buf=[0xC0]
    for i in range(256/8):
      buf.append(random.randint(0,0xFF))
    self.tpx.send_byte_array(buf)
    resp=self.tpx.recv_mask(0xCF71,0xFFFF)
    if len(resp)>1:
      for p in resp:
        print p

    #read ctpr
    self.tpx.send_byte_array([0xD0,0,0])
    resp=self.tpx.recv_mask(0xD071,0xFFFF)
    if len(resp)>1:
      for p in resp:
        print p
    for i in range(5):
      #read timer low
      self.tpx.send_byte_array([0x44,0,0])
      resp=self.tpx.recv_mask(0x4471,0xFFFF)
      if len(resp)>1:
        for p in resp:
          print p
    
      #read timer high
      self.tpx.send_byte_array([0x45,0,0])
      resp=self.tpx.recv_mask(0x4571,0xFFFF)
      if len(resp)>1:
        for p in resp:
          print p

#(0x44) and High (0x45)

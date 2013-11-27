from tpx3_test import tpx3_test
import random
import time
import logging

class test05_timers(tpx3_test):
  """Data Links and periphery timers"""

  def _execute(self,**keywords):
  
    #tot & toa mode + gray counters
    self.tpx.send_byte_array([0x30,0,0x08])
    resp=self.tpx.recv_mask(0x4071,0xFFFF)
    if len(resp)>1:
      for p in resp:
        print p


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
        
    #load ctpr
    buf=[0xC0]
    for i in range(256/8):
      buf.append(random.randint(0,0x00))
    self.tpx.send_byte_array(buf)
    resp=self.tpx.recv_mask(0xCF71,0xFFFF)
    if len(resp)>1:
      for p in resp:
        print p

    time.sleep(2)
    toa_vals={}
    dcolumns={}
    #read ctpr
    self.tpx.send_byte_array([0xD0,0,0])
    resp=self.tpx.recv_mask(0xD071,0xFFFF)
    print len(resp)
    if len(resp)!=125:
      logging.error("Load CTPR returned only %d packets (insted of 125)"%len(resp))
    for pck in resp:
      if pck.type==0xD:
        if not pck.toa in toa_vals:
          toa_vals[pck.toa]=0
        toa_vals[pck.toa]+=1
        if not pck.addr in dcolumns:
          dcolumns[pck.addr]=1
    for i in range(128):
      if not i in dcolumns.keys():
        logging.error("Missing packet for %d dcolumn"%i)
      
    print toa_vals
    
    return
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

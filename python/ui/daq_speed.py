#!/usr/bin/env python
import sys
import numpy as np
import scipy.ndimage as ndi
import random
import os
import time
import random
import os
import time
import os
from kutils import *
pdir=os.path.dirname(os.path.abspath(__file__))+"/.."
sys.path.append(pdir+"/tmp")
from tpx3 import *

class Rate():
    def __init__(self,refresh=0.02):
        self.refresh=refresh
        self.total_events=0
        self.new_events=0
        self.last_ref_time=time.time()
        self.last_s_time=self.last_ref_time

    def processed(self, events):
        now=time.time()
        self.new_events+=events
        # report rate
        dt=now-self.last_s_time
        if dt>1.0:
            rate=self.new_events/dt
            self.total_events+=self.new_events
            self.last_s_time=now
            self.new_events=0
            print "rate %s"%n2h(rate)

def test():
    rate=Rate()
    ip="192.168.100.10"
    port=50000
    tpx=TPX3(ip=ip,port=port,daq="spidr")
    s=""
    if tpx.isConnected():
        print tpx.chipID()
        print tpx.connectionStateString()
        #self.tpx.shutterOff()
        #self.shutter=0
        tot=np.zeros( (256,256) )
        hits=np.zeros( (256,256) )

        print "Starting DAQ thread"
        while True:
            next_frame=tpx.getSample(1024*16*16,1)
            rate.processed(0)

            #print next_frame
            if next_frame:
               x=tpx.daq.getNumpyFrames(tot,hits)
               #print x

               #hits=0
               #while True:
#                   r,x,y,data,tstp=tpx.nextPixel()
#                   if not r: break
#                   data>>=4
#                   data&=0x2FF
#                   hits+=1
               rate.processed(x)
class XXX:
    def __init__(self):
        pass

def test2():
    tpx=TPX3(ip="192.168.100.10",port=50000,daq="spidr")
    if tpx.isConnected():
        print tpx.chipID()
        print tpx.connectionStateString()
        tot=np.zeros( (256,256) )
        hits=np.zeros( (256,256) )
        x=tpx.daq.getNumpyFrames(tot,hits)

if __name__=="__main__":
    test()

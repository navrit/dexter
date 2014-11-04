#!/usr/bin/env python
import gpib
import time
import os

from optparse import OptionParser

class ProbeStationException(Exception):
    def __init__(self,msg,code=0):
        super(Exception, self).__init__(msg.rstrip())
        self.code=code


class ProbeStation:
  def __init__(self,address=22,logname=""):
    self.address=address
    self.logname=logname
    if self.logname!="":
        self.logfile=open(self.logname,"w")
    else:
        self.logfile=None
  def log(self,msg):
    if self.logfile:
      self.logfile.write(msg+"\n")
      self.logfile.flush()

  def connect(self):
    present=gpib.listener(0,self.address)
    if not present:
      self.log("connect:Probe Station not present.")
      raise (ProbeStationException("connect:Probe Station not present."))
    self.log("connect:Probe Station present.")
    self.dev=gpib.dev(0,self.address)
    return True

  def wr(self,m):
    m=m.rstrip()
    self.log("wr:%s"%m)
    gpib.write(self.dev,m+"\r\n")
    time.sleep(0.1)

  def rd(self,l=256):
    m=gpib.read(self.dev,l)
    self.log("rd:%s"%m)
    return m
 
  def qr(self,m,l=256):
    self.wr(m)
    return self.rd(l)

  def StepFirstDie(self):
    r=self.qr("StepFirstDie")
    rr=r.split()
    if int(rr[0])!=0:
      raise (ProbeStationException("StepFirstDie:"+r,code=int(rr[0])))
    status,index_x,index_y,message=rr
    index_x=int(index_x)
    index_y=int(index_y)
    return (index_x,index_y)

  def StepNextDie(self):
    r=self.qr("StepNextDie")
    rr=r.split()
    if int(rr[0])!=0:
      raise (ProbeStationException("StepNextDie:"+r,code=int(rr[0])))
    status,index_x,index_y,message=rr
    index_x=int(index_x)
    index_y=int(index_y)
    return (index_x,index_y)


  def GoToXY(self,x,y,):
    r=self.qr("StepNextDie %d %d"%(x,y))
    rr=r.split()
    if int(rr[0])!=0:
      raise (ProbeStationException("GoToXY:"+r,code=int(rr[0])))
    return (rr[1],rr[2],rr[3])


  def Contact(self):
    r=self.qr("MoveChuckContact")
    rr=r.split()
    if int(rr[0])!=0:
      raise (ProbeStationException("MoveChuckContact:"+r,code=int(rr[0])))
    return True

  def Align(self):
    r=self.qr("MoveChuckAlign")
    rr=r.split()
    if int(rr[0])!=0:
      raise (ProbeStationException("MoveChuckAlign:"+r,code=int(rr[0])))
    return True

  def ReadMapPosition(self):
    r=self.qr("ReadMapPosition")
    rr=r.split()
    if int(rr[0])!=0:
      raise (ProbeStationException("ReadMapPosition:"+r,code=int(rr[0])))
    status,index_x,index_y,x,y,message=rr
    index_x=int(index_x)
    index_y=int(index_y)
    return (index_x,index_y)

def main():

    parser = OptionParser()
    parser.add_option("-l", "--log",           dest="logname",    help="log file name", default="probestation.log")
    parser.add_option("-g", "--gpib",          dest="address",    help="GPIB address of probestation", type="int", default=22)
    parser.add_option("-r", "--read-position", dest="readpos",    help="Read current position", action="store_true", default=False)
    parser.add_option("-f", "--step-first-die",dest="stepfirst",  help="Step first die", action="store_true", default=False)
    parser.add_option("-n", "--step-next-die", dest="stepnext",   help="Step next die", action="store_true", default=False)
    parser.add_option("-a", "--align",         dest="align",      help="Align", action="store_true", default=False)
    parser.add_option("-c", "--concact",       dest="concact",    help="Concact", action="store_true", default=False)

    (options, args) = parser.parse_args()

    ps=ProbeStation(address=options.address,logname=options.logname)
    try:
      ps.connect()
      if options.stepfirst: 
          print "StepFirstDie",ps.StepFirstDie()
      if options.stepnext: 
          print "StepFirstDie",ps.StepNextDie()

      if options.readpos: 
        print "ReadMapPosition",ps.ReadMapPosition()
      if options.align: 
        print "Align",ps.Align()
      if options.concact: 
        print "Concact",ps.Contact()
#      ps.GoToXY(7,1)
    except ProbeStationException as ex:
      print "ERROR",ex,ex.code
       
if __name__=="__main__":
  main()

#!/usr/bin/env python
import gpib
import time
import time
class ProbeStation:
  def __init__(self,address=22):
    self.address=address

  def connect(self):
    present=gpib.listener(0,self.address)
    if not present:
      print "Probe Station not present"
      return False
    self.dev=gpib.dev(0,self.address)

  def wr(self,m):
    m=m.rstrip()
    print "WR",m
    gpib.write(self.dev,m+"\r\n")
    time.sleep(0.2)

  def rd(self,l=256):
    m=gpib.read(self.dev,l)
    print "RD:",m.rstrip()
    return m

  def qr(self,m,l=256):
    self.wr(m)
    return self.rd(l)

  def StepFirstDie(self):
    self.qr("StepFirstDie")

  def StepNextDie(self):
    self.qr("StepNextDie")

  def GoToXY(self,x,y,):
    r=self.qr("StepNextDie %d %d"%(x,y))
    rr=r.split()
    return (rr[0],rr[1],rr[2])
def test():
  ps=ProbeStation()
  ps.connect()
  ps.StepFirstDie()
  if 0:
   for die in range(105):
    print "## DIE %d ##"%die
    ps.StepNextDie()
  ps.GoToXY(5,5)
  ps.GoToXY(5,5)
  ps.GoToXY(4,4)
  ps.GoToXY(9,9)
  ps.GoToXY(5,5)
if __name__=="__main__":
  test()

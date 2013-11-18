#!/usr/bin/env python
import numpy


def eq_broute_force(fbase):
  scans=[]
  avr=[]
  for dacc in range(16):
    data=numpy.loadtxt(fbase+"dacs%X_bl.dat"%dacc)
    scans.append(data)
    a=numpy.average(data)
    avr.append(a)
    print "DAC=0x%X AVR=%.2f"%(dacc,a)
  target=(avr[0]+avr[15])/2
  print "Target %.2f"%target
  bestv=numpy.zeros(scans[0].shape)
  bestcode=numpy.zeros(scans[0].shape)
  X,Y=scans[0].shape
  for x in range(X):
    for y in range(Y):
      bc=0
      bv=scans[0][x][y]
      for i in range(1,16):
        if abs(bv-target)>(scans[i][x][y]-target):
          bc=i
          bv=scans[i][x][y]
      bestv[x][y]=bv
      bestcode[x][y]=bc
  numpy.savetxt(fbase+"eq_bl.dat",bestv,fmt="%.2f")
  numpy.savetxt(fbase+"eq_codes.dat",bestcode,fmt="%.0f")
def main():
   eq_broute_force("logs/f3_ikrum15/20131105/test08_equalization/")
if __name__=="__main__":
  main()

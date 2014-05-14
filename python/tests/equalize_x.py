#!/usr/bin/env python
import numpy


def eq_broute_force(fbase):
  scans=[]
  avr=[]
  for dacc in range(0,16,15):
    data=numpy.loadtxt(fbase+"/dacs%X_bl_thr.dat"%dacc)
    scans.append(data)
  bestv=numpy.zeros(scans[0].shape)
  bestcode=numpy.zeros(scans[0].shape)
  maskPixels=numpy.zeros(scans[0].shape)
  #print "%a %a"%(bestv,bestcode)
  avr0=[]
  avr1=[]
  X,Y=scans[0].shape
  for x in range(X):
    for y in range(Y):
	if scans[0][x][y]>0:
	   avr0.append(scans[0][x][y])
	if scans[1][x][y]>0:
	   avr1.append(scans[1][x][y])
  avrmean0=numpy.average(avr0)
  avrmean1=numpy.average(avr1)
  target=(avrmean0+avrmean1)/2	   
  print "DAC0 AVR=%.2f DAC15 AVR=%.2f -> TARGET=%.2f"%(avrmean0,avrmean1,target)
  X,Y=scans[0].shape
  for x in range(X):
    for y in range(Y):
      step=(abs(scans[0][x][y]-scans[1][x][y]))/15
      #print "(%d,%d) -> %.2f"%(x,y,step)
      bc=0
      bv=scans[0][x][y]
      for i in range(16):
        if abs(bv-target) > abs((step*i+scans[0][x][y])-target):
          bc=i
          bv=step*i+scans[0][x][y]
      bestv[x][y]=bv
      bestcode[x][y]=bc
      if abs(bestv[x][y]-target) > step :
        maskPixels[x][y]=1
	bestv[x][y]=0
	bestcode[x][y]=0
      else:
        maskPixels[x][y]=0		
  numpy.savetxt(fbase+"/eq_bl.dat",bestv,fmt="%.2f")
  numpy.savetxt(fbase+"/eq_codes.dat",bestcode,fmt="%.0f")
  numpy.savetxt(fbase+"/eq_mask.dat",maskPixels,fmt="%d")
def main():
   eq_broute_force("logs/xx/last/test08_equalization_x")
if __name__=="__main__":
  main()

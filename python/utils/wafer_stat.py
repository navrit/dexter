#!/usr/bin/env python
import sys
import glob
import numpy as np
def main():
  WAFER=1
  basedir='../logs'
  dies=[]
  keys=[]
  names=[]
  for wdir in sorted(glob.glob(basedir+"/W%d_*"%WAFER)):
    die={}
#    print wdir
    n=wdir.split("/")[-1]
    names.append(n)
    for test in glob.glob(wdir+"/*/*/results.txt"):
      f=open(test,"r")
      for l in f.readlines():
        k,v=l.split()
        if k in ['NOISE_RMS','NOISE_MEAN','BL_RMS','BL_MEAN', 'GAIN_MEAN', 'GAIN_RMS']:
          v="%.3f"%float(v)
          
        die[k]=v
        
        if not k in keys: keys.append(k)
      f.close()
    dies.append(die)
  

  print " "*21,

  for n in names:
      print "%8s "%n,
  print
  for k in sorted(keys):

    print "%20s "%k,
    vals=[]
    for die in dies:
      s=""
      if k in die: s=die[k]
      print "%8s "%s,
      try:
        v=float(die[k])
        vals.append(v)
      except:
        pass
    if len(vals)>0:
      print "| %7.3f +/- %6.3f"%(np.mean(vals),np.std(vals)),
    print 
    

if __name__=="__main__":
  main()

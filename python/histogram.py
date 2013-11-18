#!/usr/bin/env python
import numpy
import sys

def histogram(fbase):
  
  data=numpy.loadtxt(fbase)
  fout=fbase[:-4]+".hst"
  hist, bin_edges =numpy.histogram(data,200,(0,10))
  f=open(fout,"w")
  for i in range(len(hist)):
    f.write("%.3e %.3e \n"%(bin_edges[i],hist[i]))
    f.write("%.3e %.3e \n"%(bin_edges[i+1],hist[i]))
  f.close()
  print fout
  
def main():
   
   histogram(sys.argv[1])
if __name__=="__main__":
  main()

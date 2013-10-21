#!/usr/bin/env python
import numpy


def histogram(fbase):
  
  data=numpy.loadtxt(fbase)
  fout=fbase[:-4]+".hst"
  hist, bin_edges =numpy.histogram(data,1000,(0,1000))
  f=open(fout,"w")
  for i in range(len(hist)):
    f.write("%.3e %.3e \n"%(bin_edges[i],hist[i]))
    f.write("%.3e %.3e \n"%(bin_edges[i+1],hist[i]))
  f.close()
  print fout
def main():

   histogram('logs/F3_default_eq_bruteforce/test08_equalization_test/bl.dat')
if __name__=="__main__":
  main()

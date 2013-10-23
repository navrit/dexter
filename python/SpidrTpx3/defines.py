#!/usr/bin/env python

def load_defines(fname):
  f=open(fname,"r")
  r=[]
  for l in f.readlines():
    if l.find("#define")>=0 :
      if len(l.split())>2:
        c=l.split()[1]
        r.append(c)
  f.close()
  return r
if __name__=="__main__":
  defines=load_defines("../SpidrTpx3Lib/tpx3defs.h")
  for d in defines:
    print d

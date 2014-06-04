#!/usr/bin/env python

def load_defines(fname, defs=()):
  f=open(fname,"r")
  ifdefs=[""]
  ifdefs.extend(defs)
  ifdef=""
  r=[]
  for l in f.readlines():
    if l.find("#define")>=0 :
      if len(l.split())>2:
        c=l.split()[1]
        if ifdef in ifdefs : 
          r.append(c)
          

    if l.find("#ifdef")>=0 :
      if len(l.split())>=2:
        ifdef=l.split()[1]
        
    if l.find("#endif")>=0 :
      ifdef=""

  f.close()
  return r
if __name__=="__main__":
  defines=load_defines("../SpidrTpx3Lib/tpx3defs.h")
  for d in defines:
    print d

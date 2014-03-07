#!/usr/bin/env python
import sys
from SpidrTpx3 import tpx3packet
import numpy 

def main():
  fname=sys.argv[1]
  fin=open(fname,'r')
  
  fout=open(fname[:-4]+".dat",'w')
  resp=[]
  state=0
  msg=""
  for l in fin.readlines():
    t,v=l.rstrip().split(',')
    v=v[:-1]
    
    if len(v)==1:v="0"+v
    
    if v=='K28.5' or v=='K28.5':
      state=0
      msg=""
      t0=0.0
      continue
    else:
#      print ">",v
      msg=v+msg
      t0=t
      state+=1
      if state==6:
        state=0
#        print msg
        resp.append ( (t0,msg))
        msg=""
        
        
  valid=numpy.zeros((256,256))
  valid_pixels=0
            


  for t,v in resp:
    pck_num=int(v, 16)<<16
    d=tpx3packet(pck_num)
#    ret.append(p)
    fout.write(" %-14s %s\n"%(t,str(d)))
    if d.type==0x9:
        valid[d.col][d.row]+=1
        valid_pixels+=1
  fout.close()

  if valid_pixels==256*256:
     print "All pixels were correcly configured and readout"
  else:
     print "Only %d pixels were correcly configured and readout (problem with %d pixels)"%(valid_pixels,256*256-valid_pixels)
     displayed=0
     bad=""
#      fn=self.fname+".bad"
#      f=open(fn,"w")
#      for x in range(256):
#        for y in range(256):
#          if valid[x][y]==0:
#            f.write("1 ")
#            if displayed<20:
#              bad+="(%d,%d) "%(x,y)
#              displayed+=1
#          else:
#            f.write("0 ")
#        f.write("\n")
#      if displayed==20: bad+="..."
#      logging.info("Bad pixels: %s"%bad)
#      logging.info("Storring bad pixel map to %s"%fn)


  print len(resp)
if __name__=="__main__":
  main()

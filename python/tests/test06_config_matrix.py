from tpx3_test import tpx3_test
import random
import logging
import numpy
import time
import random

class test06_config_matrix(tpx3_test):
  """Configure / Verify matrix configuration"""

  def _execute(self,**keywords):
#    pycallgraph.start_trace()
    self.tpx.resetPixels()
    rnd = random.Random()
    self.mkdir(self.fname)

    rnd.seed(0)
    for pattern in ['zeros','ones','random',]:
      logging.info(" ")
      logging.info("Digital patern being used during the test '%s'"%pattern)
      self.tpx.resetPixelConfig()
      stimulus={}
      for x in range(256):
        stimulus[x]={}
        for y in range(256):
          if pattern=='ones':
            dac=0x0F
            tb=1
            mask=1
          elif pattern=='random':
            dac= rnd.randint(0, 0xF)
            tb=rnd.randint(0, 1)
            mask=rnd.randint(0, 1)
          else:
            dac=0
            tb=0
            mask=0 
          def flip_bits4b(v):
            return (v&1)<<3 | (v&2)<<1 | (v&4)>>1  | (v&8)>>3
          config=(tb<<5)+( flip_bits4b(dac)<<1 )+mask
          
          stimulus[x][y]={'config':config,'received':0, 'ok':False}
          self.tpx.setPixelThreshold(x,y,dac)
          self.tpx.setPixelTestEna(x,y, tb)
          self.tpx.setPixelMask(x,y,mask)
      self.tpx.setPixelConfig()
      eth_filter,cpu_filter=self.tpx.getHeaderFilter()
      self.tpx.setHeaderFilter(0xffff,cpu_filter&(~0x0200)) # cpu should not see 0x90 packets

      self.tpx.send_byte_array([0x90]+[0x00]*(256/8)) 
      if 0:
        print "sleep"
        time.sleep(50)
        print "read"

      self.tpx.setShutterLen(1)
#    self.tpx.openShutter()
      self.tpx.sequentialReadout(tokens=120)
      data=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)
#      valid=numpy.zeros((256,256))
      
      for d in data:
        if d.type==0x9:
          stimulus[d.col][d.row]['received']+=1
          if d.config==stimulus[d.col][d.row]['config']: 
            stimulus[d.col][d.row]['ok']=1
          else:
            stimulus[d.col][d.row]['err']=d.config


      valid_pixels=0
      for x in range(256):
        for y in range(256):
          if stimulus[x][y]['received']==1 and stimulus[x][y]['ok']==1:
            valid_pixels+=1
#          else:
#            print x,y,stimulus[x][y]['received'],stimulus[x][y]['ok']


      fn=self.fname+".bad"
      if valid_pixels==256*256:
        logging.info("All pixels were correcly configured and readout")
      else:
        logging.info("Only %d pixels were correcly configured and readout (problem with %d pixels)"%(valid_pixels,256*256-valid_pixels))
        fn=self.fname+"/%s.map"%pattern
        fmap=open(fn,"w")
        logging.info("Storring bad pixel map to %s"%fn)

        fn=self.fname+"/%s.details"%pattern
        fdet=open(fn,"w")
        logging.info("Storring bad details list to %s"%fn)

        missing=""
        missing_displayed=0

        bad=""
        bad_displayed=0
        for x in range(256):
          for y in range(256):
            if stimulus[x][y]['received']<1:
              fmap.write("1 ")
              fdet.write("(%3d,%3d) - missing\n"%(x,y))
              if missing_displayed<20:
                missing+="(%d,%d) "%(x,y)
                missing_displayed+=1
            elif stimulus[x][y]['ok']!=1:
              fmap.write("1 ")
              fdet.write("(%3d,%3d) - bad conf %02x insted of %02x\n"%(x,y,stimulus[d.col][d.row]['err'], stimulus[d.col][d.row]['config']))
              if bad_displayed<20:
                bad+="(%d,%d %02x!=%02x) "%(x,y, stimulus[d.col][d.row]['config'], stimulus[d.col][d.row]['err'])
                bad_displayed+=1
            else:
              fmap.write("0 ")
          fmap.write("\n")
        fmap.close()
        fdet.close()
        if bad_displayed==20: bad+="..."
        if missing_displayed==20: missing+="..."
        if bad_displayed>0: logging.info("Bad pixels: %s"%bad)
        if missing_displayed>0: logging.info("Missing pixels: %s"%missing)



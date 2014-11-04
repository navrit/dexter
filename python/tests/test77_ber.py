from tpx3_test import tpx3_test
import random
import logging
import numpy 
import time
import random

class test77_ber(tpx3_test):
  """Test Bit Error Rate as a function of link mask"""

  def _execute(self,**keywords):
    rnd = random.Random()
    rnd.seed(0)
    missing_pixels=set()
    multiple_pixels=set()
    bad_pixel_config=set()
    vomiting_pixels=set()


    eth_filter,cpu_filter=self.tpx.getHeaderFilter()
    self.tpx.setHeaderFilter(0xffff,cpu_filter&(~0x0200)) # cpu should not see 0x90 packets

    logname=self.fname+"/stats.txt"
    f=open(logname,"w")
    for links in range(255,256):
        self.tpx.reinitDevice()
        eth_filter,cpu_filter=self.tpx.getHeaderFilter()
        self.tpx.setHeaderFilter(0xffff,cpu_filter&(~0x0200)) # cpu should not see 0x90 packets
        self.logging.info(" ")
        self.logging.info("Active link '0x%02x'"%links)
        self.tpx.setOutputMask( mask=links)
        self.tpx.setSlvsConfig(0x10)
        pixel_errors=0

        for pattern in ['zeros','ones','random',]:
              _missing_pixels=set()
              _multiple_pixels=set()
              self.tpx.resetPixels()
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

              MAX=256
              for loop in range(MAX):
                self.warning_detailed_restart()
                self.logging.info("ACQ %s %d/%d"%(pattern,loop+1,MAX))
                for x in range(256):
                  for y in range(256):
                    stimulus[x][y]['received']=0
                    stimulus[x][y]['ok']=False

                _missing_pixels=set()
                _multiple_pixels=set()

                self.tpx.send_byte_array([0x90]+[0x00]*(256/8))

                self.tpx.sequentialReadout(tokens=128,now=True)

                received=0
                for d in self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000,timeout=100):
                  if d.type==0x9:
                    stimulus[d.col][d.row]['received']+=1
                    received+=1
                    if d.config==stimulus[d.col][d.row]['config']:
                      stimulus[d.col][d.row]['ok']=1
                    else:
                      stimulus[d.col][d.row]['err']=d.config
                valid_pixels=0
                for x in range(256):
                  for y in range(256):
                    if not stimulus[x][y]['received']:
                      _missing_pixels.add( (x,y) )
                      self.warning_detailed(" Pixel (%d,%d) not received!"%(x,y))
                    elif stimulus[x][y]['received']==1 :
                      if stimulus[x][y]['ok']==1:
                        valid_pixels+=1
                      else:
                        self.warning_detailed(" Pixel (%d,%d) received %d times, received value 0x%2x, expected value 0x%2x"%(x,y,stimulus[x][y]['received'],stimulus[x][y]['err'],stimulus[x][y]['config']))
                        bad_pixel_config.add( (x,y) )
                    else:
                        self.warning_detailed(" Pixel (%d,%d) received %d times!"%(x,y,stimulus[x][y]['received']))
                        _multiple_pixels.add( (x,y) )
                self.warning_detailed_summary()

                if valid_pixels==256*256:
#                  self.logging.info("OK")
                   pass
                else:
                    er=256*256-valid_pixels
                    self.logging.warning("Received %d pixels (missing %d)"%(received,er))
                    pixel_errors+=er
        self.logging.info("Active link '0x%02x' -> problems %d"%(links,pixel_errors))
        f.write("%d %d\n"%(links,pixel_errors))
        f.flush()
    f.close()



#    if len(missing_pixels)>0:
#       self.update_category('Kcnf')
    return


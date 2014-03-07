from tpx3_test import tpx3_test
import random
import logging
import numpy 
import time
import random

class test06_config_matrix(tpx3_test):
  """Configure / Verify matrix configuration"""

  def _execute(self,**keywords):
    rnd = random.Random()
    rnd.seed(0)

    missing_pixels=set()
    multiple_pixels=set()
    bad_pixel_config=set()
    vomiting_pixels=set()
    for pattern in ['zeros','ones','random',]:
      self.warning_detailed_restart()
      _missing_pixels=set()
      _multiple_pixels=set()
      self.tpx.reinitDevice()
      self.tpx.resetPixels()
      self.logging.info(" ")
      self.logging.info("Digital pattern being used during the test '%s'"%pattern)
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

      self.tpx.sequentialReadout(tokens=120)
      data=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)
      if len(data)>256*(256+1):
        print len(data)
        self.logging.error("Vomiting chip.")
        self.logging.error("Last packets")
        for d in data[-15:]:
          if d.type==0x9:
            self.logging.error(" %s"%str(d))
            vomiting_pixels.add( (d.col, d.row) )
        self.update_category("V")
        break

      received=0
      for d in data:
        if d.type==0x9:
          stimulus[d.col][d.row]['received']+=1
          received+=1
          if d.config==stimulus[d.col][d.row]['config']: 
            stimulus[d.col][d.row]['ok']=1
          else:
            stimulus[d.col][d.row]['err']=d.config
          if d.col==203 and d.row==244:
            print d
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



      fn=self.fname+".bad"
      if valid_pixels==256*256:
        self.logging.info("All pixels were correctly configured and readout")
      else:
        fn=self.fname+"/%s.map"%pattern
        fmap=open(fn,"w")
        self.logging.warning("Received %d pixels (missing %d)"%(received,256*256-received))
        self.logging.warning("Storing bad pixel map to %s"%fn)
        for y in range(256):
          for x in range(256):
            if (x,y) in _missing_pixels:
              fmap.write("1 ")
            elif (x,y) in _multiple_pixels:
              fmap.write("2 ")
            elif (x,y) in bad_pixel_config:
              fmap.write("3 ")
            else:
              fmap.write("0 ")
          fmap.write("\n")
        fmap.close()
        for pix in _missing_pixels:
          missing_pixels.add(pix)
        for pix in _multiple_pixels:
          multiple_pixels.add(pix)

    bc=0
    for col in range(256):
       bp=0
       for row in range(256):
          if (col,row) in bad_pixel_config:
            bp+=1
       if bp>8:
          self.logging.warning("Bad column: %d. Problem with %d pixels"%(col,bp))
#          self.bad_columns.add(col)
          bc+=1

    self.add_bad_pixels(bad_pixel_config)

    self.results["CNF_BAD_PIXELS"]=len(bad_pixel_config)
    self.results["CNF_MISSING_PIXELS"]=len(missing_pixels)
    self.results["CNF_VOM_PIXELS"]=len(vomiting_pixels)
    self.results["CNF_BAD_COLUMNS"]=bc

    self.logging.info("")
    self.warn_info("Bad pixels: %s"%len(bad_pixel_config), len(bad_pixel_config)>0 )
    self.warn_info("Bad columns: %s"%bc, bc>0)
    self.warn_info_pixel_list("Missing pixels: %s"%len(missing_pixels), missing_pixels)
    self.warn_info_pixel_list("Pixels received multiple times: %s"%len(multiple_pixels), multiple_pixels)

    if len(bad_pixel_config) or len(missing_pixels):
        fn=self.fname+"/results.map"
        fmap=open(fn,"w")
        self.logging.warning("Storing bad pixel map to %s"%fn)
        for y in range(256):
          for x in range(256):
            if (x,y) in _missing_pixels:
              fmap.write("1 ")
            elif (x,y) in bad_pixel_config:
              fmap.write("2 ")
            else:
              fmap.write("0 ")
          fmap.write("\n")
        fmap.close()

    if len(multiple_pixels)>0:
       self.update_category('M')

#    if len(missing_pixels)>0:
#       self.update_category('Kcnf')
    return


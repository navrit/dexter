from tpx3_test import tpx3_test
import random
import logging
import numpy

class test06_config_matrix(tpx3_test):
  """Configure / Verify matrix configuration"""

  def _execute(self,**keywords):
#    pycallgraph.start_trace()
    self.tpx.resetPixels()

    self.tpx.resetPixelConfig()
    for x in range(256):
      for y in range(256):
        dac=0
        tb=0
        mask=0
        self.tpx.setPixelThreshold(x,y,dac)
        self.tpx.setPixelTestEna(x,y, tb)
        self.tpx.setPixelMask(x,y,mask)
    self.tpx.setPixelConfig()

    eth_filter,cpu_filter=self.tpx.getHeaderFilter()
    self.tpx.setHeaderFilter(eth_filter,cpu_filter|0x0200)

    self.tpx.send_byte_array([0x90]+[0x00]*(256/8)) 
    self.tpx.sequentialReadout()
#    self.tpx.getPixelConfig()
    pix=0
    data=self.tpx.recv_mask(0x11D0000000000000, 0xFFFF000000000000)
    print len(data)
    valid=numpy.zeros((256,256))
    for d in data:
      if d.type==0x9:
        valid[d.col][d.row]+=1
        pix+=1
    for x in range(256):
      for y in range(256):
        if valid[x][y]==0:
           print "Missing:",x,y
    print pix
#        if dcol[d.addr]: 
#          logging.warning("Multiple packets for double column %d"%(d.addr))
#        else:
#          expected=ctpr[2*d.addr+1]*2+ctpr[2*d.addr]
#          if expected!=d.ctpr:
#            logging.warning("Wrong CTPR value for double column %d (received:%d, expected:%d)"%(d.addr, d.ctpr, expected))
#          else:
#            #looks fine
#            dcol[d.addr]=1
#            if not d.toa in toa:
#              toa[d.toa]=0
#            toa[d.toa]+=1
#              
#          
#      elif d.type!=0x7:
#        logging.warning("Unexpected packet %s"%str(d))
#    err=0
#    if sum(dcol)!=128:
#      logging.warning("Received data for only %d double columns (should be 128)"%(sum(dcol)) )
#      missing=""
#      for d in range(128):
#        if dcol[d]==0:
#           missing+="%d "%d
#      logging.warning("Missing double columns %s"%missing )
#        
#      logging.warning("Received data:")
#      for d in data:
#        logging.warning("  %s"%str(d) )

#      data=self.tpx.recv_mask(0x71D0000000000000, 0xFFFF000000000000)
#      for d in data:
#        logging.warning("  >> %s"%str(d) )
#      err=1
#    if len(toa.keys())>2:
#      logging.warning("TOA spread to high (values:%s)"%(str(toa.keys())) )
#      err=1
#    if not err:
#      logging.info("CTPR OK")
    
#        config=[]
#        for sub_column in range(column,column+cstep):
#          logging.debug("LoadMatrixConfig for column %d"%sub_column)
#          LoadMatrixConsig=0x80
#          pixel_config=[]
#          for p in range(256):
#            pixel_config.append(random.randint(0,0x3F))
#          config.append(pixel_config)
#          buf=[LoadMatrixConsig]+gen_col_mask(sub_column)+gen_config_vector(pixel_config)
#          self.tpx.send_byte_array(buf)
#          resp=self.tpx.recv_mask(0x8f71,0xFFFF)
#          if len(resp)>1:
#            for p in resp:
#              print p

#        logging.debug("Read Config Matrix")
#        ReadConfigMatrix=0x90
#        buf=[ReadConfigMatrix]+gen_col_mask(range(column,column+cstep))
#        self.tpx.send_byte_array(buf)
#        resp=self.tpx.recv_mask(0x9f71,0xFFFF)
#        if len(resp)>1:
#          for p in resp:
#            print p


#        logging.debug("Read sequential")
#        ReadMatrixSeq=0xA0
#        buf=[ReadMatrixSeq]+[0]*28+[0x00,0x0,0x1,0x0]
#        self.tpx.send_byte_array(buf)
#        resp=self.tpx.recv_mask(0xa071,0xFFFF)
#        
#        matrix=dict()

#        errors=0
##        if resp[-1].type!=7: 
##          errors=255
##          self.info( "Timeout !! (%d packets received)"%(len(resp)))
##        elif len(resp)<256:
##          errors+=256-len(resp)
##          self.info( "Too less packets !! (%d packets received)"%(len(resp)))
#        
#        for p in resp:
#          if p.type==0x9:
#            if not p.col in matrix : 
#              matrix[p.col]={}
#            if not p.row in matrix[p.col] : 
#              matrix[p.col][p.row]=p.config
#              cc=p.col-column
#              if p.config!=config[cc][p.row]:
#                print "  Pixel configuration (%d,%d) failed !!"%(p.col,p.row)
#                errors+=1
#            else:
#              print "  Pixel configuration (%d,%d) received multiple time"%(p.col,p.row)
#              errors+=1
#        missing=0
#        logging.info("Total packets received %d"%len(resp))
#        for k in matrix:
#          if len(matrix[k])<256: 
#            l="%d missing pixels in column %d ( "%(256-len(matrix[k]),k)
#            for i in range(256):
#              if i not in matrix[k]: 
#                l+="%d "%i
#                missing+=1
#              
#            logging.error(l+")")
#        self._assert_true((missing==0),"Missing pixels %d"%(missing))
#        self._assert_true((errors==0),"Failing pixels %d"%(errors))
##    pycallgraph.make_dot_graph('test.png')



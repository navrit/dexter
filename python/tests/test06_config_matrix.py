from tpx3_test import tpx3_test
import random
import logging

class test06_config_matrix(tpx3_test):
  """Configure / Verify matrix configuration"""

  def _execute(self):
#    pycallgraph.start_trace()

       
    def gen_col_mask(cols):
      r=[0xFF]*(256/8)
      if isinstance(cols, int):
        cols=[cols]
      for col_enabled in cols:
        col=255-col_enabled
        byte=int(col/8)
        bit=7-col%8
        r[byte]&=~(1<<bit)
      return r
      
    def gen_config_vector(pixel_config):
       r=[0x00]*((6*256)/8)

       for p in range(256):
          pp=p%4
          pof=((6*256)/8) - 3 - int(p/4)*3
#          print p,len(r),pof
          if pp==3:
            r[pof+0]|=pixel_config[p]<<2
          elif pp==2:
            r[pof+0]|=(pixel_config[p]>>4)&0x3
            r[pof+1]|=(pixel_config[p]&0xf)<<4
          elif pp==1:
            r[pof+1]|=(pixel_config[p]>>2)&0xf
            r[pof+2]|=(pixel_config[p]&0x3)<<6
          elif pp==0:
            r[pof+2]|=(pixel_config[p]&0x3f)

       return r

    dbg=1
    cstep=256
    
    for column in range(0,1,cstep):
      for test_patern in range(1):

        logging.debug("Stop Matrix Command")
        StopMatrixCommand=0xF0
        buf=[StopMatrixCommand]+[0,0]
        self.tpx.send_byte_array(buf)
        resp=self.tpx.recv_mask(0xFF71,0xFFFF)
#        if len(resp)>1:
#          for p in resp:
#            print p
            
        logging.debug("Reset seq")
        ReadMatrixSeq=0xE0
        buf=[ReadMatrixSeq]
        for i in range(144/8):
          buf.append(0)
        self.tpx.send_byte_array(buf)
        resp=self.tpx.recv_mask(0xEF71,0xFFFF)
        if len(resp)>1:
          for p in resp:
            print p

        config=[]
        for sub_column in range(column,column+cstep):
          logging.debug("LoadMatrixConfig for column %d"%sub_column)
          LoadMatrixConsig=0x80
          pixel_config=[]
          for p in range(256):
            pixel_config.append(random.randint(0,0x3F))
          config.append(pixel_config)
          buf=[LoadMatrixConsig]+gen_col_mask(sub_column)+gen_config_vector(pixel_config)
          self.tpx.send_byte_array(buf)
          resp=self.tpx.recv_mask(0x8f71,0xFFFF)
          if len(resp)>1:
            for p in resp:
              print p

        logging.debug("Read Config Matrix")
        ReadConfigMatrix=0x90
        buf=[ReadConfigMatrix]+gen_col_mask(range(column,column+cstep))
        self.tpx.send_byte_array(buf)
        resp=self.tpx.recv_mask(0x9f71,0xFFFF)
        if len(resp)>1:
          for p in resp:
            print p


        logging.debug("Read sequential")
        ReadMatrixSeq=0xA0
        buf=[ReadMatrixSeq]+[0]*28+[0x00,0x0,0x1,0x0]
        self.tpx.send_byte_array(buf)
        resp=self.tpx.recv_mask(0xa071,0xFFFF)
        
        matrix=dict()

        errors=0
#        if resp[-1].type!=7: 
#          errors=255
#          self.info( "Timeout !! (%d packets received)"%(len(resp)))
#        elif len(resp)<256:
#          errors+=256-len(resp)
#          self.info( "Too less packets !! (%d packets received)"%(len(resp)))
        
        for p in resp:
          if p.type==0x9:
            if not p.col in matrix : 
              matrix[p.col]={}
            if not p.row in matrix[p.col] : 
              matrix[p.col][p.row]=p.config
              cc=p.col-column
              if p.config!=config[cc][p.row]:
                print "  Pixel configuration (%d,%d) failed !!"%(p.col,p.row)
                errors+=1
            else:
              print "  Pixel configuration (%d,%d) received multiple time"%(p.col,p.row)
              errors+=1
        missing=0
        logging.info("Total packets received %d"%len(resp))
        for k in matrix:
          if len(matrix[k])<256: 
            l="%d missing pixels in column %d ( "%(256-len(matrix[k]),k)
            for i in range(256):
              if i not in matrix[k]: 
                l+="%d "%i
                missing+=1
              
            logging.error(l+")")
        self._assert_true((missing==0),"Missing pixels %d"%(missing))
        self._assert_true((errors==0),"Failing pixels %d"%(errors))
#    pycallgraph.make_dot_graph('test.png')


class test06_xxxxxxx():
  """Configure / Verify matrix configuration"""

  def _execute(self):
    pycallgraph.start_trace()

       
    def gen_col_mask(cols):
      r=[0xFF]*(256/8)
      if isinstance(cols, int):
        cols=[cols]
      for col_enabled in cols:
        col=255-col_enabled
        byte=int(col/8)
        bit=7-col%8
        r[byte]&=~(1<<bit)
      return r
      
    def gen_config_vector(pixel_config):
       r=[0x00]*((6*256)/8)

       for p in range(256):
          pp=p%4
          pof=((6*256)/8) - 3 - int(p/4)*3
#          print p,len(r),pof
          if pp==3:
            r[pof+0]|=pixel_config[p]<<2
          elif pp==2:
            r[pof+0]|=(pixel_config[p]>>4)&0x3
            r[pof+1]|=(pixel_config[p]&0xf)<<4
          elif pp==1:
            r[pof+1]|=(pixel_config[p]>>2)&0xf
            r[pof+2]|=(pixel_config[p]&0x3)<<6
          elif pp==0:
            r[pof+2]|=(pixel_config[p]&0x3f)

       return r
       
    dbg=1
    
    if 1:
        if dbg: print "\nStop Matrix Command"
        StopMatrixCommand=0xF0
        buf=[StopMatrixCommand]+[0,0]
        self.tpx.send_byte_array(buf)
        resp=self.tpx.recv_mask(0xFF71,0xFFFF)
        if len(resp)>1:
          for p in resp:
            print p
            
        if dbg: print "\nReset seq"
        ReadMatrixSeq=0xE0
        buf=[ReadMatrixSeq]
        for i in range(144/8):
          buf.append(0)
        self.tpx.send_byte_array(buf)
        resp=self.tpx.recv_mask(0xEF71,0xFFFF)
        if len(resp)>1:
          for p in resp:
            print p

        if dbg: print "\nSLVSConsig"
        SLVSConfig=0x34
        lvsc=0x0
        buf=[SLVSConfig]+[0,lvsc|0x10]
        self.tpx.send_byte_array(buf)
        resp=self.tpx.recv_mask(0x3471,0xFFFF)
        if len(resp)>1:
          for p in resp:
            print p



    cstep=1
    hitmap={}
    
    if 1:
        for sub_column in range(0,1):
          if dbg: print "\nColumn %d"%sub_column
          if dbg: print "  Load Matrix"
          LoadMatrixConsig=0x80
          pixel_config=[]
          for p in range(256):
            pixel_config.append(0x3f)
          buf=[LoadMatrixConsig]+gen_col_mask(sub_column)+gen_config_vector(pixel_config)
          self.tpx.send_byte_array(buf)
          resp=self.tpx.recv_mask(0x8f71,0xFFFF)
          if len(resp)>1:
            for p in resp:
              print p


    for frame in range(1):
     print "FRAME%d"%frame
     for column in range(0,1,cstep):
      for test_patern in range(1):
        resp=self.tpx.recv(1000000)
        print "FLUSHING %d"%len(resp)
        
        if dbg: print "  Read Config Matrix"
        ReadConfigMatrix=0x90
        buf=[ReadConfigMatrix]+gen_col_mask(range(column,column+cstep))
        self.tpx.send_byte_array(buf)
        resp=self.tpx.recv_mask(0x9f71,0xFFFF)
        if len(resp)>1:
          for p in resp:
            print p

        time.sleep(10)
        if dbg: print "  Read sequential"
        ReadMatrixSeq=0xA0
        buf=[ReadMatrixSeq]+[0]*28+[0x00,0x0,0xF,0x0]
        self.tpx.send_byte_array(buf)
        resp=self.tpx.recv_mask(0xa071,0xFFFF)
        
        matrix=dict()

        errors=0
#        if resp[-1].type!=7: 
#          errors=255
#          self.info( "Timeout !! (%d packets received)"%(len(resp)))
#        elif len(resp)<256:
#          errors+=256-len(resp)
#          self.info( "Too less packets !! (%d packets received)"%(len(resp)))
        for p in resp:
          if p.type==0x9:
            if not p.col in matrix : 
              matrix[p.col]={}
            if not p.row in matrix[p.col] : 
              matrix[p.col][p.row]=p.config
#              cc=p.col-column
#              if p.config!=config[cc][p.row]:
#                print "  Pixel configuration (%d,%d) failed !!"%(p.col,p.row)
#                errors+=1
            else:
              print "  Pixel configuration (%d,%d) received multiple time"%(p.col,p.row)
              errors+=1

        f=open("data/missing/frame_%04d.dat"%frame,"w")
        print "Total packets received %d"%len(resp)
        for k in range(column,column+cstep):
          if k in matrix:
            print "  Packets received in column %d : %d"%(k,len(matrix[k])),
            if len(matrix[k])<256: 
              print "\n      Missing pixels (%d): "%(256-len(matrix[k])),
              for i in range(256):
                if i not in matrix[k]: 
                  print i,
                  f.write("%d %d\n"%(k,i))
                  if not k in hitmap:
                    hitmap[k]={}
                  if not i in hitmap[k]:
                    hitmap[k][i]=0
                  hitmap[k][i]+=1
                    
          else:
            print "  Packets received in column %d : 0 !!!!!!!!!!!!"%k
#            for i in range(256): 
#               f.write("%d %d\n"%(k,i))
          print
        f.close()
        g = Gnuplot.Gnuplot(debug=0)
        g('set output "data/missing/frame_%04d.png"'%frame)
        g('set terminal png')
        g('set xlabel "Column')
        g('set ylabel "Row"')
        g("set xtic 32")
        g("set ytic 32")
        g("set mxtic 4")
        g("set mytic 4")
        g("set size sq")
        g("set yr[0:256]")
        g("set xr[0:256]")
        g("set grid xti yt mxti myti")
        g("plot 'data/missing/frame_%04d.dat' u 1:2 w p pt 4 ps 0.3 t ''"%frame)
        self._assert_true((errors==0),"Column configuration test (column %d, failing pixels %d)"%(column,errors))

    f=open("data/missing/hitmap.dat","w")
    for row in range(0,256):
      for col in range(0,256):
        if col in hitmap and row in hitmap[col]: f.write("%d "%hitmap[col][row])
        else: f.write("0 ")
      f.write("\n")
    f.close()

#!/usr/bin/env python
import Gnuplot, Gnuplot.funcutils
from SpidrTpx3 import TPX3
import random
import time
import sys
import socket
import os
import getpass
import random
import pycallgraph

class TPX_tests:
  def get_time(self):
    return time.strftime("%H:%M:%S", time.localtime())

  def log2(self,msg):
    s="[%s] %-70s"%(self.get_time(),msg)
    print s

  def log(self,msg,r):
    if r:
      s="  OK  "
    else:
      s="FAILED"
    s="[%s] %-70s [%s]"%(self.get_time(),msg,s)
    self._log_file.write(s+"\n")
    print s
    
  def info(self,msg=""):
    self._log_file.write(msg+"\n")

    
  def get_date_time(self):
    return time.strftime("%Y/%m/%d %H:%M:%S", time.localtime())

  def get_user_name(self):
    return getpass.getuser()

  def get_host_name(self):
    return socket.gethostname()

  def _in_range(val,min,max):
    if val>=min and val<=max:
      return True
    else:
      return false

  def _assert_true(self,val,msg):
    self.log(msg,val)
    if not val:
      self.errors.append(msg)


  def _assert_in_range(self,val,min,max,msg):
    ok=False
    if val>=min and val<=max:
      ok=True
    self.log(msg,ok)
    if not ok:
      self.errors.append(msg)

  def _assert_equal(self,val,min,max,msg):
    ok=False
    if val>=min and val<=max:
      ok=True
    self.log(msg,ok)
    if not ok:
      self.errors.append(msg)
  def mkdir(self,d):
    if not os.path.exists(d):
      os.makedirs(d)  

  def __init__(self,name):
    self.dlogdir="logs/%s/"%name
    self.name=name
    self.mkdir(self.dlogdir)
    self._log_file=open("logs/%s.txt"%name,"w")
    
  def __del__(self):
   if self._log_file:
    self._log_file.close()
    
  def test00_power_consumption(self):
    def conv(meas):
      r,v,i,p=meas
      v=float(v)/1000
      i=float(i)/10000
      p=float(p)/1000
      return (r,v,i,p)
    ret_values={}

    r,v,i,p=conv(self.tpx.ctrl.getDvdd())
    self._assert_true(r,"Reading digial power consumption")
    self._assert_in_range(v,1.4,1.6,"Digital supply voltage %.3f V"%v)
    self._assert_in_range(i,0.1,0.2,"Digital supply current %.3f A"%i)
    self._assert_in_range(p,0.2,0.3,"Digital power consumption %.3f W"%p)
    
    ret_values['VDD']="%.3f"%v
    ret_values['IDD']="%.3f"%i
    ret_values['PDD']="%.3f"%p

    r,v,i,p=conv(self.tpx.ctrl.getAvdd())
    self._assert_true(r,"Reading analog power consumption")
    self._assert_in_range(v,1.4,1.6,"Analog supply voltage %.3f V"%v)
    self._assert_in_range(i,0.4,0.6,"Analog supply current %.3f A"%i)
    self._assert_in_range(p,0.6,0.8,"Analog power consumption %.3f W"%p)
    ret_values['VDDA']="%.3f"%v
    ret_values['IDDA']="%.3f"%i
    ret_values['PDDA']="%.3f"%p


  def test02_reset_values(self):
    r,v=self.tpx.ctrl.getDeviceId(self.tpx.id)
    self._assert_true(r,"Reading device ID")
    DEF_DEVIDE_ID=0
    self._assert_true((v==DEF_DEVIDE_ID),"Device ID 0x%0X"%v)

    r,v=self.tpx.ctrl.getGenConfig(self.tpx.id)
    self._assert_true(r,"Reading General Config")
    GEN_CONFIG_DEFAULT_VALUE=1
    self._assert_true((v==GEN_CONFIG_DEFAULT_VALUE),"General Config value 0x%0X"%v)

    r,v=self.tpx.ctrl.getPllConfig(self.tpx.id)
    self._assert_true(r,"Reading PLL Config")
    PLL_CONFIG_DEFAULT_VALUE=0xE
    self._assert_true((v==PLL_CONFIG_DEFAULT_VALUE),"PLL Config value 0x%0X"%v)

    r,v=self.tpx.ctrl.getOutBlockConfig(self.tpx.id)
    self._assert_true(r,"Reading Output Block Config")
    OUT_BLOCK_CONFIG_DEFAULT_VALUE=0x9FF
    self._assert_true((v==OUT_BLOCK_CONFIG_DEFAULT_VALUE),"Output Block value 0x%0X"%v)

    r,v1,v2=self.tpx.ctrl.getTpPeriodPhase(self.tpx.id)
    self._assert_true(r,"Reading TP Period & Phase")
    TP_PERIOD_DEFAULT_VALUE=0x0
    TP_PHASE_DEFAULT_VALUE=0x0
    self._assert_true((v1==TP_PERIOD_DEFAULT_VALUE),"TP period value 0x%0X"%v1)
    self._assert_true((v2==TP_PHASE_DEFAULT_VALUE),"TP phase value 0x%0X"%v2)

    r,v=self.tpx.ctrl.getTpNumber(self.tpx.id)
    self._assert_true(r,"Reading TP Number")
    TP_NUMBER_DEFAULT_VALUE=0x0
    self._assert_true((v==TP_NUMBER_DEFAULT_VALUE),"TP number value 0x%0X"%v)

#c2.add_method('getDac',                'bool',        [param('int', 'dev_nr'),param('int', 'dac_code'),param('int*', 'dac_val', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
#c2.add_method('getGenConfig',          'bool',        [param('int', 'dev_nr'),param('int*', 'config', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
#c2.add_method('getPllConfig',          'bool',        [param('int', 'dev_nr'),param('int*', 'config', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
#c2.add_method('getOutBlockConfig',     'bool',        [param('int', 'dev_nr'),param('int*', 'config', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
#c2.add_method('getTpPeriodPhase',      'bool',        [param('int', 'dev_nr'),param('int*', 'period', transfer_ownership=False,direction = Parameter.DIRECTION_OUT),param('int*', 'phase', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])
#c2.add_method('getTpNumber',           'bool',        [param('int', 'dev_nr'),param('int*', 'number', transfer_ownership=False,direction = Parameter.DIRECTION_OUT)])

  def test_list(self):
    r=[]
    TEST_PREFIX="test"
    for fn in dir(self):
      if fn.find(TEST_PREFIX)==0:
        #check if following chars are numbers
        if fn[len(TEST_PREFIX)].isdigit():
          r.append(fn)
    return sorted(r)
        
  def prepare(self):
    self.start=self.get_date_time()
    self.tpx=TPX3()
    self.tpx.info()
    self.data={}
    self.data['WNAME']="W1"
    self.data['CHIP_NAME']="TIMEPIX3"
    self.data['CHIPID']="B6"
    self.data['STATUS']="[B]"
    self.data['SYSTEM']="SPIDR"
    self.data['SYSTEM_REV']="1.01a"
    self.data['START']=self.start
    self.data['USER']=self.get_user_name()
    self.data['HOST']=self.get_host_name()

  def test01_bias(self):

    self.tpx.ctrl.setSenseDac(0,0x1C)
    v=self.tpx.get_adc(4)
    self._assert_in_range(v,0.6,0.7,"Bandgap voltage %.3f V"%v)
    self.data['VBDG']="%.3f"%v
    
    self.tpx.ctrl.setSenseDac(0,0x1D)
    v=self.tpx.get_adc(4)
    self._assert_in_range(v,0.6,0.7,"Temperature voltage %.3f V"%v)
    self.data['VTEMP']="%.3f"%v
    
    self.tpx.ctrl.setSenseDac(0,0x1E)
    v=self.tpx.get_adc(4)
    self._assert_in_range(v,1.1,1.2,"IBias DAC voltage %.3f V"%v)
    self.data['IB_DAC']="%.3f"%v

    self.tpx.ctrl.setSenseDac(0,0x1F)
    v=self.tpx.get_adc(4)
    self._assert_in_range(v,0.9,1.1,"IBias DAC cascode voltage %.3f V"%v)
    self.data['DAC_CAS']=v


  def test03_dac_scan(self):
#    tpx.ctrl.resetDevices()
    results=[]
    def_step=8
    meas_per_code=1
    self.info("\nDAC scan settings:")
    self.info("  default step size: %d"%def_step)
    self.info("  measurements per code: %d"%meas_per_code)
    self.info()
    
    g = Gnuplot.Gnuplot(debug=0)
    g('set xlabel "Code / Max Code')
    g('set ylabel "Voltage [V]"')
    g("set xtic 1.0/4")
    g("set ytic 0.2")
    g("set yr[0:1.79]")
    g("set xr[0:1]")
    g("set grid")
    g("set key  horiz top samp 1")

    g._clear_queue()
    dac_name=  ['none','IB_PRE_ON',  'IB_PRE_OFF','VPRE_NCAS',     'IB_IKRUM',
                       'VFBK',       'VTHR_FIN',  'VTHR_COA',      'IB_DIS1_ON',
                       'IB_DIS1_OFF','IB_DIS2_ON','IB_DIS2_OFF',   'IB_PIXDAC',
                       'IB_TPBIN',   'IB_TPBOUT', 'VTP_COA',       'VTP_FINE',
                       'IB_CP_PLL']
    fsr=  [(0,0),    (0.3,0.4),(0.1,0.2),  (0.9,1.2),  (0.3,0.4),
                     (0.9,1.2),(0.2,0.3),  (1.0,1.2),  (0.5,0.6),
                     (0.15,0.25),(0.25,0.35),(0.1,0.2),  (0.35,0.45),   
                     (0.3,0.4),(0.4,0.5),  (0.9,1.2),  (0.9,1.2),
                     (0.4,0.5)]

    retdict={}
    for dac_id in range(1,18):
      self.tpx.ctrl.setSenseDac(self.tpx.id,dac_id)
      name=dac_name[dac_id]
      max_code=self.tpx.ctrl.dacMax(dac_id)
      x=[]
      y=[]
      val=0
      self.tpx.ctrl.setDacsDflt(self.tpx.id)
      f=open(self.dlogdir+"%s.txt"%name,"w")
      step=def_step
      if max_code<255:
        step=1
      steps=[]
      for code in range(0,max_code+1,step):
        steps.append(code)
      if steps[-1]!=max_code:steps.append(max_code)
      self.info("\nDAC %s scan details:"%name)
      for code in steps:
        r=self.tpx.ctrl.setDac(self.tpx.id,dac_id,code)
#        print r
        val=self.tpx.get_adc(meas_per_code)


#        print "%c%20s [%03d/%03d] %0.3f"%(13,name,code,max_code, val),
#        sys.stdout.flush()
        x.append(float(code)/max_code)
        y.append(val)
        self.info("  %03d %6.4f"%(code,val))
        f.write("%03d %6.4f\n"%(code,val))

      f.close()
      fs=y[-1]-y[0]

      mono="YES"
      sigma=0.002
      for i in range(3,len(y)-3):
        if (y[i]>y[i-1]+sigma) and fs<0:
          mono="NO"
#          print '\n',name,i,y[i-1],y[i]
        if (y[i]<y[i-1]-sigma) and fs>0:
          mono="NO"
#          print '\n',name,i,y[i-1],y[i]
      
#      print "FS: %.3f Mono: %s"%(abs(fs),mono)
      fsstr=name+"_FS"
      retdict[fsstr]="%.3f"%abs(fs)
      monostr=name+"_MONO"
      retdict[monostr]=mono
      d = Gnuplot.Data(x, y,   title=name,    with_='lp pt 5 ps 0.2')
      g._add_to_queue([d])
      self.info("  Measured full scale %.3f"%(abs(fs)))
      self.info("  Acceptance range [%.3f,%3f]"%fsr[dac_id])

      self._assert_in_range(abs(fs),fsr[dac_id][0],fsr[dac_id][1],"DAC %s FS range %.3f V"%(name,abs(fs)))
      self._assert_true((mono=="YES"),"DAC %s monotonicity %s"%(name,mono))
      
    g("set terminal png size 450,370 small")
    g("set output 'dac2.png'")
    g.refresh() 
    return retdict
    
  def atest05_read_ctpr(self):
#    self.tpx.send(0x2,0x1,0)
    self.tpx.send_byte_array([0xd0,0,0])
    resp=self.tpx.recv(130)
    for p in resp:
      print p

  def test06_read_config(self):
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

        if dbg: print "\Stop Matrix Command"
        StopMatrixCommand=0xF0
        buf=[StopMatrixCommand]+[0,0]
        self.tpx.send_byte_array(buf)
        resp=self.tpx.recv_mask(0xFF71,0xFFFF)
#        if len(resp)>1:
#          for p in resp:
#            print p
            
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

        config=[]
        for sub_column in range(column,column+cstep):
          if dbg: print "\nColumn %d"%sub_column
          if dbg: print "  Load Matrix"
          LoadMatrixConsig=0x80
          pixel_config=[]
          for p in range(256):
            pixel_config.append(random.randint(0,0x3F))
          config.append(pixel_config)
          buf=[LoadMatrixConsig]+gen_col_mask(sub_column)+gen_config_vector(pixel_config)
          if 0:
            for i,b in enumerate(buf):
              print "%02x"%b,
              if i%64==63: print
            print
          self.tpx.send_byte_array(buf)
          resp=self.tpx.recv_mask(0x8f71,0xFFFF)
          if len(resp)>1:
            for p in resp:
              print p

        if dbg: print "  Read Config Matrix"
        ReadConfigMatrix=0x90
        buf=[ReadConfigMatrix]+gen_col_mask(range(column,column+cstep))
        self.tpx.send_byte_array(buf)
        resp=self.tpx.recv_mask(0x9f71,0xFFFF)
        if len(resp)>1:
          for p in resp:
            print p


        if dbg: print "  Read sequential"
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

        print "Total packets received %d"%len(resp)
        for k in matrix:
          print "  Packets received in column %d : %d"%(k,len(matrix[k])),
          if len(matrix[k])<256: 
            print "\n      Missing pixels (%d): "%(256-len(matrix[k])),
            for i in range(256):
              if i not in matrix[k]: print i,
          print
        self._assert_true((errors==0),"Column configuration test (column %d, failing pixels %d)"%(column,errors))
#    pycallgraph.make_dot_graph('test.png')


  def atest07_read_config_failing_pixels(self):
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

  def execute(self):
   self.prepare()
#    test0_power_consumption(tpx)
    
#  dac_scan_data = dac_scan(tpx,def_step=8)
#  for k in dac_scan_data:
#    data[k]=dac_scan_data[k]
   if 0:
    for t in self.test_list():
      self.errors=[]
      methodToCall = getattr(self, t)
      l=( 71+8 - (len(t)+3))/2
      msg="%s %s %s"%("#"*l, t,"#"*l)
      msg+="#"*(79-len(msg))
      
      self.log2(msg)
      result = methodToCall()
    self.stop=self.get_date_time()
#    test2_reset_values(tpx)
    self.data['STOP']=self.stop

def main():
  name="00"
  if len(sys.argv)>1:
    name=sys.argv[1]
  tests=TPX_tests(name)
  tests.execute()

#  gen_report(data,"report.pdf")
if __name__=="__main__":
  main()

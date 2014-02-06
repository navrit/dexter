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

  

  def execute(self):
   self.prepare()
   print 1
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

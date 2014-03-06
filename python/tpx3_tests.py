#!/usr/bin/env python
import Gnuplot, Gnuplot.funcutils
from SpidrTpx3 import TPX3
import random
import time
import sys
import socket
import os
import getpass
import tests
#import pycallgraph
import logging
from optparse import OptionParser
import datetime
import shlex

def dict2file(fname,d):
    f=open(fname,"w")
    for k in sorted(d.keys()):
      f.write("%s %s\n"%(k.upper(),d[k]))
    f.close()



def mkdir(d):
  if not os.path.exists(d):
    os.makedirs(d)  

today=datetime.datetime.now().strftime("%Y%m%d")

class TPX_tests:
  def __init__(self,name):
    self.dlogdir="logs/%s/%s/"%(name,today)
    self.name=name
    mkdir(self.dlogdir)
    
  def connect(self,ip):
      self.tpx=TPX3(ip)

  def get_time(self):
    return time.strftime("%H:%M:%S", time.localtime())

  def get_date_time(self):
    return time.strftime("%Y/%m/%d %H:%M:%S", time.localtime())

  def get_user_name(self):
    return getpass.getuser()

  def get_host_name(self):
    return socket.gethostname()

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
    logging.info("Start date          : %s"%self.start)
    logging.info("User                : %s"%self.get_user_name())
    logging.info("Host                : %s"%self.get_host_name())
    self.tpx.info()

  def execute(self,test_list=[],wiki=False):
    self.prepare()
    avaliable_tests=tests.get_tests()
    if len(test_list)==0:
      for test_name in sorted(avaliable_tests):
        test_list.append(test_name)
      test_list=sorted(test_list)

    params={'wiki':wiki}
    result={'category':'A','bad_pixels':set(),'timeouts':0}
    
    for test_name in test_list:
      if test_name.find("(")>=0:
        if test_name.find(")")>=0:
          args=test_name[test_name.find("(")+1:test_name.find(")")]
          test_name=test_name[:test_name.find("(")]
          params_new=dict(token.split('=') for token in shlex.split(args))    
          for k in params_new:
            params[k]=params_new[k]

      if test_name in avaliable_tests:
        test=avaliable_tests[test_name](tpx=self.tpx,fname=self.dlogdir+test_name)

        for k in result:
          test.set_atribute(k, result[k])

        test.execute(**params)

        for k in result:
          result[k]= test.get_atribute(k)

        for k in test.results:
          result[k]= test.results[k]

        if not test.cont : 
          break
      else:
        logging.warning( "Unknown test %s"%test_name)
        

    logging.info("")
    logging.info("Total number of timeouts : %d"%result["timeouts"])
    logging.info("")

    logging.info("Final categorization")
    bad_pix=len(result["bad_pixels"])

    bad_col=0
    for c in range(256):
      bp=0
      for r in range(256):
        if (c,r) in result["bad_pixels"]:
          bp+=1
      if bp>8:
        logging.info("  Found %d bad pixels in column %d"%(bp,c))
        bad_col+=1

    bad_sp=0
    DOUBLE_COLUMNS=256/2
    SP_PER_COL=256/4
    
    for dc in range(DOUBLE_COLUMNS):
      for sp in range(SP_PER_COL):
        bp=0
        for c in range(2*dc,2*(dc+1)):
          for r in range(sp*4,(sp+1)*4):
            if (c,r) in result["bad_pixels"]:
              bp+=1
        if bp>4:
           logging.info("  Found %d bad pixels in super pixel dc=%d sp=%d"%(bp,dc,sp))
           bad_sp+=1

    logging.info("Bad pixels : %d"%bad_pix)
    logging.info("Bad columns : %d"%bad_col)
    logging.info("Bad super pixels : %d"%bad_sp)
    
    new_cat='D'
    if (bad_sp==0) and (bad_col==0) and bad_pix<=64:
      new_cat='A'
    elif (bad_sp==1 or bad_col==1) and bad_pix<=256:
      new_cat='B'
    elif (bad_sp<=2 or bad_col<=2) and bad_pix<=1024:
      new_cat='C'
    else:
      new_cat='D'
      
    if result["category"]=='A':
      result["category"]=new_cat
    else:
      logging.info("Based on missing pixels, chip category would be %s"%new_cat)
    logging.info("Chip category %s"%result["category"])

    fn=self.dlogdir+"result.bad"
    logging.info("Map of bad pixels stored to %s"%fn)
    f=open(fn,"w")
    for r in range(256):
      for c in range(256):
        if (c,r) in result["bad_pixels"]:
          f.write("1 ")
        else:
          f.write("0 ")
      f.write("\n")
    f.close()


#    for k in ('bad_pixels','bad_columns','bad_superpixels'):
    result['bad_pixels']=bad_pix
    result['bad_columns']=bad_col
    result['bad_superpixels']=bad_sp

      
    fn=self.dlogdir+"result.dat"
    dict2file(fn,result)
    logging.info("Results stored to %s"%fn)


    return 




  def list(self):
   avaliable_tests=tests.get_tests()
   print "%-40s | %s"%("Test name", "Test description")
   print "-"*80
   for test_name in sorted(avaliable_tests):
     print "%-40s |"%(test_name),
     for i,l in enumerate(avaliable_tests[test_name].__doc__.split('\n')):
       if i>0:print "%-40s |"%(""),
       print l

def env_check():
  import netifaces as ni
  good_iface=None
  for iface  in ni.interfaces():
    if ni.AF_INET in ni.ifaddresses(iface):
      for va in range(len(ni.ifaddresses(iface)[ni.AF_INET])):
        if not 'broadcast' in ni.ifaddresses(iface)[ni.AF_INET][va]: continue
        if ni.ifaddresses(iface)[ni.AF_INET][va]['broadcast']=='192.168.100.255':
          good_iface=iface
  if good_iface==None:
    logging.error("Unable to find properly configured ethernet interface")
    return False
  
  def get_mtu(iface):
    #linux specific code !!
    import subprocess
    proc = subprocess.Popen(["ifconfig %s"%good_iface], stdout=subprocess.PIPE, shell=True)
    (out, err) = proc.communicate()
    if out.find("MTU:")>=0:
        return int(out[out.find("MTU:")+4:].split()[0])
        
  if get_mtu(good_iface)!=9000:
    logging.error("Wrong MTU for interface %s"%good_iface)
    logging.error("sudo ifconfig %s mtu 9000"%good_iface)
    return False
  return True
  

def start_pcap(iface,fname):  
  import threading
  import pcap

  class pcapThread (threading.Thread):
    def __init__(self, iface,fname):
        threading.Thread.__init__(self)
        self.iface=iface
        self.exitFlag=False
        
        self.p = pcap.pcapObject()
        self.p.open_live(iface, 100, 0, 100)
        self.p.dump_open(fname)
        self.p.setnonblock(1)

    def exit(self):
       self.exitFlag=True

       return  self.p.stats()
       
    def run(self):
        while not self.exitFlag:
           self.p.dispatch(0, None)
           time.sleep(0.0001)
  user = os.getuid()
  if user != 0:
    print "This program requires root privileges. Run as root using 'sudo'."
    print "Rerunning the script with sudo."
    cmd="sudo "+" ".join(sys.argv)
    os.system( cmd)
    sys.exit()
  CaptureThread=pcapThread(iface,fname)
  CaptureThread.start()
  return CaptureThread
           

def main():
  usage = "usage: %prog [options] assembly_name [test[(parameter=value parameter2=value)]"
  parser = OptionParser(usage=usage,version="%prog 0.01")
  parser.add_option("-i", "--ip",                              dest="ip",         default="192.168.100.10:50000", help="IP address of SPIDR TPX3 Controller")
  parser.add_option("-p", "--prefix",                          dest="prefix",     default="", help="prefix")
  parser.add_option("-d", "--dump-all",   action="store_true", dest="dump_all",   default=False,   help="Dump all Timepix3 messages")
  parser.add_option("-l", "--list-tests", action="store_true", dest="list_tests", default=False,  help="List all avaliable tests")
  parser.add_option("-v", "--verbose",    action="store_true", dest="verbose",    default=False,  help="Verbose output in console (debug log level)")
  parser.add_option("-w", "--wiki",       action="store_true", dest="wiki",       default=False,  help="Add wiki banner (and log file)")
  parser.add_option("",   "--pcap",       action="store_true", dest="pcap",       default=False,  help="Store all Ethernet comunication in *.pcap file")

  (options, args) = parser.parse_args()

  if options.list_tests:
    tests=TPX_tests("null")
    tests.list()
    return
    
  if len(args)<1:
    parser.error("You have to specify assembly name")
  name=args[0]
    #set up logger
  run_name="%s"%today
  if options.prefix!="":
    run_name+="_"+options.prefix
  mkdir("logs/%s/%s/"%(name,run_name))
  logname='logs/%s/%s/log.txt'%(name,run_name)
  logging.basicConfig(level=logging.INFO,
                    format='[%(levelname)7s] [%(relativeCreated)5d] %(message)s',
                    datefmt='%M:%S',filename=logname, filemode='w')
  formatter = logging.Formatter('[%(levelname)7s] [%(relativeCreated)5d] %(message)s')
  consoleHandler = logging.StreamHandler()
  consoleHandler.setFormatter(formatter)
  if options.verbose:
    consoleHandler.setLevel(logging.DEBUG)
  else:
    consoleHandler.setLevel(logging.INFO)

  logging.getLogger('').addHandler(consoleHandler)
  logging.info("Log will be stored to %s"%logname)

  if options.pcap:
    pcapname='logs/%s/%s/log.pcap'%(name,run_name)
    logging.info("Storring pcap log to %s"%pcapname)
    CaptureThread=start_pcap("eth3",pcapname)


  test_list=[]
  if len(args)>1:
    test_list=args[1:]

  if 1 : #env_check():
    try:
      tests=TPX_tests(name)
      tests.connect(options.ip)
      if options.dump_all:
        tests.tpx.log_packets=True
      tests.execute(test_list,wiki=options.wiki)
    except RuntimeError as e:
      logging.critical(e)
  if options.pcap:
    stat=CaptureThread.exit()
    logging.info('PCAP log : %d packets received, %d packets dropped, %d packets dropped by interface' % stat)
if __name__=="__main__":
  main()

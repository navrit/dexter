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
import pycallgraph
import logging
from optparse import OptionParser
import datetime

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

  def execute(self,test_list=[]):
  
   self.prepare()
   avaliable_tests=tests.get_tests()
   if len(test_list)==0:
     for test_name in sorted(avaliable_tests):
       test_list.append(test_name)
     test_list=sorted(test_list)

   for test_name in test_list:
     if test_name in avaliable_tests:
       test=avaliable_tests[test_name](tpx=self.tpx,fname=self.dlogdir+test_name)
       test.execute()
     else:
       print "Unknown test %s"%test_name

  def list(self):
   avaliable_tests=tests.get_tests()
   print "%-40s | %s"%("Test name", "Test description")
   print "-"*80
   for test_name in sorted(avaliable_tests):
     print "%-40s | %s"%(test_name, avaliable_tests[test_name].__doc__)


def main():
  usage = "usage: %prog [options] assembly_name [test]"
  parser = OptionParser(usage=usage,version="%prog 0.01")
  parser.add_option("-i", "--ip",                              dest="ip",         default="192.168.100.10:50000", help="IP address of SPIDR TPX3 Controller")
  parser.add_option("-p", "--prefix",                          dest="prefix",     default="", help="prefix")
  parser.add_option("-d", "--dump-all",   action="store_true", dest="dump_all",   default=False,   help="Dump all Timepix3 messages")
  parser.add_option("-l", "--list-tests", action="store_true", dest="list_tests", default=False,  help="List all avaliable tests")
  parser.add_option("-v", "--verbose",    action="store_true", dest="verbose",    default=False,  help="Verbose output in console (debug log level)")

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
  logging.basicConfig(level=logging.DEBUG,
                    format='[%(levelname)6s] [%(relativeCreated)5d] %(message)s',
                    datefmt='%M:%S',filename=logname, filemode='w')
  formatter = logging.Formatter('[%(levelname)6s] [%(relativeCreated)5d] %(message)s')
  consoleHandler = logging.StreamHandler()
  consoleHandler.setFormatter(formatter)
  consoleHandler.setLevel(logging.INFO)
  if options.verbose:
    consoleHandler.setLevel(logging.DEBUG)
  
  logging.getLogger('').addHandler(consoleHandler)
  logging.info("Log will be stored to %s"%logname)

  test_list=[]
  if len(args)>1:
    test_list=args[1:]
    
  try:
    tests=TPX_tests(name)
    tests.connect(options.ip)
  
    if options.dump_all:
      tests.tpx.log_packets=True
    tests.execute(test_list)
  except RuntimeError as e:
    logging.critical(e)

if __name__=="__main__":
  main()

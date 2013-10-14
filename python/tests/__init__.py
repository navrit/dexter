import os
import sys
import glob
import inspect
from tpx3_test import tpx3_test
import importlib


for f in glob.glob(os.path.dirname( __file__)+"/test*.py"):
 bn=os.path.basename(f)[:-3]
 x=__import__("tests."+bn)

 
#for mname, mod in inspect.getmembers(x):
#  if mname.find("test")>=0 and inspect.ismodule(mod):
#    for cname, obj in inspect.getmembers(mod):
#      if inspect.isclass(obj): 
#        print cname,obj
#        ii="tests."+mname
#        print ii
#        __import__(ii, globals(), locals(), ['cname'], -1)


def get_tests():
  d={}
  for cls in tpx3_test.__subclasses__():
    name=cls(None).__class__.__name__
    d[name]=cls
  return d

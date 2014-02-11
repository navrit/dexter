#!/usr/bin/env python
import shlex
import sys

s = 'IBIAS=12 THR="string with space" key3=[1,2]'

fnc=sys.argv[1]
params={}
if fnc.find("(")>=0:
  if fnc.find(")")>=0:
    args=fnc[fnc.find("(")+1:fnc.find(")")]
    fnc=fnc[:fnc.find("(")]
    params=dict(token.split('=') for token in shlex.split(args))    
print fnc
for p in params:
  print p,params[p]


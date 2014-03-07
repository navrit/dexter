#!/usr/bin/env python
import os
import sys
from probestation import ProbeStation
import time

def query_yes_no(question, default="yes"):
    """Ask a yes/no question via raw_input() and return their answer.

    "question" is a string that is presented to the user.
    "default" is the presumed answer if the user just hits <Enter>.
        It must be "yes" (the default), "no" or None (meaning
        an answer is required of the user).

    The "answer" return value is one of "yes" or "no".
    """
    valid = {"yes":True,   "y":True,  "ye":True,
             "no":False,     "n":False}
    if default == None:
        prompt = " [y/n] "
    elif default == "yes":
        prompt = " [Y/n] "
    elif default == "no":
        prompt = " [y/N] "
    else:
        raise ValueError("invalid default answer: '%s'" % default)

    while True:
        sys.stdout.write(question + prompt)
        choice = raw_input().lower()
        if default is not None and choice == '':
            return valid[default]
        elif choice in valid:
            return valid[choice]
        else:
            sys.stdout.write("Please respond with 'yes' or 'no' \
                             (or 'y' or 'n').\n")

def main():
  if len(sys.argv)!=2:
    print "You have to specify wafer number! Ending ..."
    return

  WNUMBER=int(sys.argv[1])
  go=query_yes_no("Start burning wafer %d"%WNUMBER,default="no")
  if not go: return

  ps=ProbeStation()
  ps.connect()
  x,y,z=ps.StepFirstDie()
  ps.Contact()
  XLUT=('-','A','B','C','D','E','F','G','H','I','J','K','L','M')
  ask=True
  for i in range(105):
    print "\n\n"
    print "-"*80
    WNAME="W%d_%s%d"%(WNUMBER,XLUT[x],y)
    print "step ",i, WNAME, z
    print
    force=""
    if not ask:
      force=" force='True'"
    cmd="./tpx3_tests.py %s_efuse test01_supply \"test17_efuse(name='%s' %s)\""%(WNAME,WNAME,force)
    print cmd
    print "-"*80
    os.system(cmd)
    if ask :
      ask=query_yes_no("Ask about next die?",default="yes")
    x,y,z=ps.StepNextDie()


if __name__=="__main__":
  main()

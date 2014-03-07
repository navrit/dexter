#!/usr/bin/env python
import os
import sys
from probestation import ProbeStation
import time

def main():
  if len(sys.argv)!=3:
    print "You have to specify wafer number and the wafer name! Ending ..."
    return

  WNUMBER=int(sys.argv[1])
  IBMNAME=sys.argv[2]
  ps=ProbeStation()
  ps.connect()
  x,y,z=ps.StepFirstDie()
  XLUT=('-','A','B','C','D','E','F','G','H','I','J','K','L','M')
  for i in range(105):
    print "\n\n"
    print "#"*160
    WNAME="W%d_%s%d"%(WNUMBER,XLUT[x],y)
    print "# Step : %d"%i
    print "# Name : %s"%WNAME
    print "#"*160
    print
    cmd="./tpx3_tests.py %s test01_supply test17_efuse  test01_bias test02_registers test03_dac_scan test06_config_matrix test04_pixel_kidnapper  test07_clock_phasing test08_noise_scan test04_diagonal_scurves  "%(WNAME)
    print "#",cmd
    print "#"*160
    os.system(cmd)
    x,y,z=ps.StepNextDie()

    cmd="./utils/wafer_maps.py %d %s logs/ CATEGORY"%(WNUMBER,IBMNAME)
    print "#"*160
    print "#",cmd
    print "#"*160
    os.system(cmd)

    cmd="./utils/zip_latest.py logs/W%d_RESULTS.zip logs/"%(WNUMBER)
    print "#"*160
    print "#",cmd
    print "#"*160
    os.system(cmd)
    

if __name__=="__main__":
  main()

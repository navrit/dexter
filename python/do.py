#!/usr/bin/env python
import os
import sys

if len(sys.argv)!=2:
  print "%s die_name"%sys.argv[0] 
else:
  os.system("./tpx3_tests.py %s test17_efuse test01_supply test01_bias test02 test03_dac_scan test04_diagonal_scurves test07_clock_phasing test06_config_matrix test08_noise_scan "%sys.argv[1] )

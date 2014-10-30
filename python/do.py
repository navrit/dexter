#!/usr/bin/env python
import os
import sys

if len(sys.argv)==2:
  name=sys.argv[1] 
else:
  name='-a'
#test01_supply 
#  os.system("./tpx3_tests.py %s  test17_efuse test01_bias test02 test03_dac_scan test06_config_matrix test04_pixel_kidnapper  test07_clock_phasing test08_noise_scan test04_diagonal_scurves  test99_summary"%sys.argv[1] )
os.system("./tpx3_tests.py %s  test01_supply  test17_efuse test01_bias test02_registers test03_dac_scan test06_config_matrix test04_pixel_kidnapper  test07_clock_phasing test08_noise_scan test04_diagonal_scurves "%name )
#  os.system("./tpx3_tests.py %s test17_efuse test01_bias test02 test03_dac_scan test04_diagonal_scurves test07_clock_phasing test06_config_matrix test08_noise_scan "%sys.argv[1] )
#  os.system("./tpx3_tests.py %s test17_efuse test01_bias test02 test04_pixel_kidnapper  test08_noise_scan "%sys.argv[1] )

#!/usr/bin/env python
from SpidrTpx3_engine import *
def dac_defaults(tpx):
   tpx.setDacsDflt()
   tpx.setDac(TPX3_IBIAS_IKRUM,15)
   tpx.setDac(TPX3_VTP_COARSE,50)
   tpx.setDac(TPX3_VTP_FINE,112) # (0e-) slope 44.5e/LSB -> (112=1000e-)  (135=2000e-)
   tpx.setDac(TPX3_VTHRESH_COARSE,7) 
   tpx.setDac(TPX3_VFBK,143) 
#   tpx.setDac(TPX3_IBIAS_PIXELDAC,100)
   tpx.setDac(TPX3_IBIAS_PREAMP_ON,150)
   tpx.setDac(TPX3_IBIAS_DISCS1_ON,100)
#   tpx.setDac(TPX3_IBIAS_DISCS2_ON,150)


#   tpx.setDac(TPX3_IBIAS_IKRUM,9)
#   tpx.setDac(TPX3_VTP_COARSE,50)
#   tpx.setDac(TPX3_VTP_FINE,112) # (0e-) slope 44.5e/LSB -> (112=1000e-)  (135=2000e-)
#   tpx.setDac(TPX3_VTHRESH_COARSE,7) 
#   tpx.setDac(TPX3_VFBK,143) 
#   tpx.setDac(TPX3_IBIAS_PREAMP_ON,150)
#   tpx.setDac(TPX3_IBIAS_DISCS1_ON,100)


from tpx3_test import tpx3_test
from SpidrTpx3_engine import *
import time
import os
import sys
class test01_supply(tpx3_test):
  """Power supply test"""

  def _execute(self):
    def conv(meas):
      r,v,i,p=meas
      v=float(v)/1000
      i=float(i)/10000
      p=float(p)/1000
      return (r,v,i,p)
    ret_values={}

    r,v,i,p=conv(self.tpx.ctrl.getDvdd())
    self._assert_true(r,"Reading digial power consumption")
    self._assert_in_range(v,1.4,1.6,"Digital supply voltage %.3f V"%v)
    self._assert_in_range(i,0.1,0.2,"Digital supply current %.3f A"%i)
    self._assert_in_range(p,0.2,0.3,"Digital power consumption %.3f W"%p)
    
    ret_values['VDD']="%.3f"%v
    ret_values['IDD']="%.3f"%i
    ret_values['PDD']="%.3f"%p

    r,v,i,p=conv(self.tpx.ctrl.getAvdd())
    self._assert_true(r,"Reading analog power consumption")
    self._assert_in_range(v,1.4,1.6,"Analog supply voltage %.3f V"%v)
    self._assert_in_range(i,0.4,0.6,"Analog supply current %.3f A"%i)
    self._assert_in_range(p,0.6,0.8,"Analog power consumption %.3f W"%p)
    ret_values['VDDA']="%.3f"%v
    ret_values['IDDA']="%.3f"%i
    ret_values['PDDA']="%.3f"%p


class test01_bias(tpx3_test):
  """Biasing values test"""

  def _execute(self):

    self.tpx.ctrl.setSenseDac(0,0x1B)
    v=self.tpx.get_adc(4)
    self._assert_in_range(v,0.2,1.0,"PLL Control Voltage %.3f V"%v)

    
    self.tpx.ctrl.setSenseDac(0,0x1C)
    v=self.tpx.get_adc(4)
    self._assert_in_range(v,0.6,0.7,"Bandgap voltage %.3f V"%v)
#    self.data['VBDG']="%.3f"%v
    
    self.tpx.ctrl.setSenseDac(0,0x1D)
    v=self.tpx.get_adc(4)
    self._assert_in_range(v,0.6,0.7,"Temperature voltage %.3f V"%v)
#    self.data['VTEMP']="%.3f"%v
    
    self.tpx.ctrl.setSenseDac(0,0x1E)
    v=self.tpx.get_adc(4)
    self._assert_in_range(v,1.1,1.2,"IBias DAC voltage %.3f V"%v)
#    self.data['IB_DAC']="%.3f"%v

    self.tpx.ctrl.setSenseDac(0,0x1F)
    v=self.tpx.get_adc(4)
    self._assert_in_range(v,0.9,1.1,"IBias DAC cascode voltage %.3f V"%v)
#    self.data['DAC_CAS']=v

def mkdir(d):
  if not os.path.exists(d):
    os.makedirs(d)  

import datetime

class test01_supply_different_modes(tpx3_test):
  """Power supply test"""

  def _execute(self):
    def conv(meas):
      r,v,i,p=meas
      v=float(v)/1000
      i=float(i)/10000
      p=float(p)/1000
      return (r,v,i,p)
    ret_values={}


    self.tpx.resetPixels()
    self.tpx.setDacsDflt()
    self.tpx.setDac(TPX3_IBIAS_IKRUM,15)
    self.tpx.setDac(TPX3_VTP_COARSE,50)
    self.tpx.setDac(TPX3_VTHRESH_COARSE,7) 
    self.tpx.setDac(TPX3_VFBK,143) 
    self.tpx.setDac(TPX3_IBIAS_PREAMP_ON,150)
    self.tpx.setDac(TPX3_IBIAS_DISCS1_ON,100)

    self.tpx.setGenConfig(8)
    

    self.tpx.setPllConfig(TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK | TPX3_PHASESHIFT_DIV_8 | 0x14<<TPX3_PLLOUT_CONFIG_SHIFT)
    self.tpx.setTpPeriodPhase(0x0,0)

    self.tpx.resetPixelConfig()
    self.tpx.load_equalization("logs/F3_default_eq_bruteforce/test08_equalization/eq_codes.dat")
    self.tpx.setPixelConfig()

    self.tpx.datadrivenReadout()
    mkdir(self.fname)
    for gc in [0,8,8|0x40, 0x4, 8|0x40|0x4]:
      for th in range(0,300,280):
        self.tpx.resetPixels()
        self.tpx.setGenConfig(0)
        self.tpx.setDac(TPX3_VTHRESH_FINE,0)
        time.sleep(0.5)


        self.tpx.setGenConfig(gc)
        self.tpx.setDac(TPX3_VTHRESH_FINE,th)
        time.sleep(0.1)

        f=open(self.fname+"/gc%04x_th%03d.dat"%(gc,th),"w")

        start = datetime.datetime.now()

        for i in range(2000):
          now = datetime.datetime.now()
          dt=now-start
           # microseconds
          if i==100:         self.tpx.openShutter(500000)

          r,v,i,p=conv(self.tpx.ctrl.getDvdd())
          f.write("%.10f %.4f %.4f %.4f \n"%(dt.total_seconds(),v,i,p))
#          r,v,i,p=conv(self.tpx.ctrl.getAvdd())
#          f.write("%.4f %.4f %.4f \n"%(v,i,p))
          time.sleep(0.002)



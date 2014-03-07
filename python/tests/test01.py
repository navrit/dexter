from tpx3_test import tpx3_test
from SpidrTpx3_engine import *
import time
import os
import sys


class test01_supply(tpx3_test):
  """Power supply test"""

  def _execute(self,**keywords):
    def conv(meas):
      r,v,i,p=meas
      v=float(v)/1000
      i=float(i)/10000
      p=float(p)/1000
      return (r,v,i,p)
    r,v,i,p=conv(self.tpx.ctrl.getDvddNow())
    r,v,i,p=conv(self.tpx.ctrl.getAvddNow())
    time.sleep(0.5)
    r,v,i,p=conv(self.tpx.ctrl.getDvddNow())
    self._assert_true(r,"Reading digial power consumption")
    r1=self._assert_in_range(v,1.3,1.7,"Digital supply voltage %.3f V"%v)
    r2=self._assert_in_range(i,0.0,1.0,"Digital supply current %.3f A"%i)
    self._assert_in_range(p,0.0,1.5,"Digital power consumption %.3f W"%p)
    
    if not r1 or not r2:
      self.logging.error("Digital power consumption out of spec")
      self.update_category('F_d')
    self.results['VDD']="%.3f"%v
    self.results['IDD']="%.3f"%i
    self.results['PDD']="%.3f"%p

    r,v,i,p=conv(self.tpx.ctrl.getAvddNow())
    self._assert_true(r,"Reading analog power consumption")
    r1=self._assert_in_range(v,1.3,1.7,"Analog supply voltage %.3f V"%v)
    r2=self._assert_in_range(i,0.0,1.0,"Analog supply current %.3f A"%i)
    self._assert_in_range(p,0.0,1.5,"Analog power consumption %.3f W"%p)
    if not r1 or not r2:
      self.logging.error("Analog power consumption out of spec")
      self.update_category('F_a')

    self.results['VDDA']="%.3f"%v
    self.results['IDDA']="%.3f"%i
    self.results['PDDA']="%.3f"%p
    
    return 
    
class test01_bias(tpx3_test):
  """Biasing values test"""

  def _execute(self,**keywords):
    self.tpx.ctrl.setSenseDac(0,0x1B)
    v=self.tpx.get_adc(4)
    r=self._assert_in_range(v,0.2,1.2,"PLL Control Voltage %.3f V"%v)
    self.results['VOL_PLL_CONTROL']="%.4f"%v
    if not r:
      self.logging.error("PLL control voltage out of spec")
      self.update_category('E')
      
    self.tpx.ctrl.setSenseDac(0,0x1C)
    v=self.tpx.get_adc(4)
    self._assert_in_range(v,0.5,0.9,"Bandgap voltage %.3f V"%v)
    self.results['VOL_BANDGAP']="%.4f"%v
    bgv=v
    if not r:
      self.logging.error("Bandgap voltage out of spec")
      self.update_category('E')
    
    self.tpx.ctrl.setSenseDac(0,0x1D)
    v=self.tpx.get_adc(4)
    self._assert_in_range(v,0.5,0.9,"Temperature voltage %.3f V"%v)
    self.results['VOL_TEMPERATURE']="%.4f"%v
    tempv=v

    if not r:
      self.logging.error("Temperature voltage out of spec")
      self.update_category('E')

    temp=88.75-607.3*(tempv-bgv)
    self.results['TEMPERATURE']="%.4f"%temp

    self.logging.info("Temperature %.2f C"%temp)
    
    self.tpx.ctrl.setSenseDac(0,0x1E)
    v=self.tpx.get_adc(4)
    self._assert_in_range(v,.5,1.5,"IBias DAC voltage %.3f V"%v)
    self.results['VOL_IBIAS_DAC']="%.4f"%v
    if not r:
      self.logging.error("IBIAS dac voltage out of spec")
      self.update_category('E')
      
    self.tpx.ctrl.setSenseDac(0,0x1F)
    v=self.tpx.get_adc(4)
    self._assert_in_range(v,0.5,1.5,"IBias DAC cascode voltage %.3f V"%v)
    self.results['VOL_IBIAS_DAC_CASCODE']="%.4f"%v
    if not r:
      self.logging.error("IBias DAC cascode voltage out of spec")
      self.update_category('E')

    return 


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
    cat=keywords['category']
    cont=True
    self.tpx.reinitDevice()
    ret_values={}
    r,v,i,p=conv(self.tpx.ctrl.getDvdd())
    self._assert_true(r,"Reading digial power consumption")
    r1=self._assert_in_range(v,1.4,1.6,"Digital supply voltage %.3f V"%v)
    r2=self._assert_in_range(i,0.1,0.3,"Digital supply current %.3f A"%i)
    self._assert_in_range(p,0.1,0.5,"Digital power consumption %.3f W"%p)
    
    if not r1 or not r2:
      self.logging.error("Digital power consumption out of spec")
      cat='F_d'
      cont=False
    ret_values['VDD']="%.3f"%v
    ret_values['IDD']="%.3f"%i
    ret_values['PDD']="%.3f"%p

    r,v,i,p=conv(self.tpx.ctrl.getAvdd())
    self._assert_true(r,"Reading analog power consumption")
    r1=self._assert_in_range(v,1.4,1.6,"Analog supply voltage %.3f V"%v)
    r2=self._assert_in_range(i,0.4,0.6,"Analog supply current %.3f A"%i)
    self._assert_in_range(p,0.6,0.8,"Analog power consumption %.3f W"%p)
    if not r1 or not r2:
      self.logging.error("Analog power consumption out of spec")
      cat='F_a'
      cont=False

    ret_values['VDDA']="%.3f"%v
    ret_values['IDDA']="%.3f"%i
    ret_values['PDDA']="%.3f"%p

    fn=self.fname+"/results.txt"
    self.dict2file(fn,ret_values)
    self.logging.info("Results stored to %s"%fn)
    
    return {'category':cat,'info':keywords['info'], 'continue':cont}
    
class test01_bias(tpx3_test):
  """Biasing values test"""

  def _execute(self,**keywords):
    cat=keywords['category']
    cont=True  
    self.tpx.reinitDevice()
    ret_values={}

    self.tpx.ctrl.setSenseDac(0,0x1B)
    v=self.tpx.get_adc(4)
    r=self._assert_in_range(v,0.2,1.0,"PLL Control Voltage %.3f V"%v)
    ret_values['VOL_PLL_CONTROL']="%.4f"%v
    if not r:
      self.logging.error("PLL control voltage out of spec")
      cat='E'
      cont=False
      
    self.tpx.ctrl.setSenseDac(0,0x1C)
    v=self.tpx.get_adc(4)
    self._assert_in_range(v,0.6,0.7,"Bandgap voltage %.3f V"%v)
    ret_values['VOL_BANDGAP']="%.4f"%v
    bgv=v
    if not r:
      self.logging.error("Bandgap voltage out of spec")
      cat='E'
      cont=False
    
    self.tpx.ctrl.setSenseDac(0,0x1D)
    v=self.tpx.get_adc(4)
    self._assert_in_range(v,0.6,0.7,"Temperature voltage %.3f V"%v)
    ret_values['VOL_TEMPERATURE']="%.4f"%v
    tempv=v

    if not r:
      self.logging.error("Temperature voltage out of spec")
      cat='E'
      cont=False

    temp=88.75-607.3*(tempv-bgv)
    ret_values['TEMPERATURE']="%.4f"%temp

    self.logging.info("Temperature %.2f C"%temp)
    
    self.tpx.ctrl.setSenseDac(0,0x1E)
    v=self.tpx.get_adc(4)
    self._assert_in_range(v,1.1,1.2,"IBias DAC voltage %.3f V"%v)
    ret_values['VOL_IBIAS_DAC']="%.4f"%v
    if not r:
      self.logging.error("IBIAS dac voltage out of spec")
      cat='E'
      cont=False
      
    self.tpx.ctrl.setSenseDac(0,0x1F)
    v=self.tpx.get_adc(4)
    self._assert_in_range(v,0.9,1.1,"IBias DAC cascode voltage %.3f V"%v)
    ret_values['VOL_IBIAS_DAC_CASCODE']="%.4f"%v
    if not r:
      self.logging.error("IBias DAC cascode voltage out of spec")
      cat='E'
      cont=False
      
    fn=self.fname+"/results.txt"
    self.dict2file(fn,ret_values)
    self.logging.info("Results stored to %s"%fn)
    return {'category':cat,'info':keywords['info'], 'continue':cont}


from tpx3_test import tpx3_test

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


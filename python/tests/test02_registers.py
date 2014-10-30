from tpx3_test import tpx3_test

class test02_registers(tpx3_test):
  """Default register values"""

  def _execute(self,**keywords):

    r,v=self.tpx.ctrl.getDeviceId(self.tpx.id)
    self._assert_true(r,"Reading device ID")
    self.logging.info("Device ID 0x%08X"%v)
    self.logging.info("Device name %s"%self.tpx.readName())
    
    r,v=self.tpx.ctrl.getGenConfig(self.tpx.id)
    self._assert_true(r,"Reading General Config")
    GEN_CONFIG_DEFAULT_VALUE=1
    self._assert_true((v==GEN_CONFIG_DEFAULT_VALUE),"General Config value 0x%0X"%v)
    self.results['REG_GEN_CONFIG']=v

    r,v=self.tpx.ctrl.getPllConfig(self.tpx.id)
    self._assert_true(r,"Reading PLL Config")
    PLL_CONFIG_DEFAULT_VALUE=0x16
    self._assert_true((v==PLL_CONFIG_DEFAULT_VALUE),"PLL Config value 0x%0X"%v)
    self.results['REG_PLL_CONFIG']=v

    r,v=self.tpx.ctrl.getOutBlockConfig(self.tpx.id)
    self._assert_true(r,"Reading Output Block Config")
    OUT_BLOCK_CONFIG_DEFAULT_VALUE=[0x7BFF, 0x901, 0x9FF]#0x7B01#0x9FF
    self._assert_true((v in OUT_BLOCK_CONFIG_DEFAULT_VALUE),"Output Block value 0x%0X"%v)
    self.results['REG_OUT_BLOCK_CONFIG']=v

    r,v1,v2=self.tpx.ctrl.getTpPeriodPhase(self.tpx.id)
    self._assert_true(r,"Reading TP Period & Phase")
    TP_PERIOD_DEFAULT_VALUE=0x0
    TP_PHASE_DEFAULT_VALUE=0x0
    self._assert_true((v1==TP_PERIOD_DEFAULT_VALUE),"TP period value 0x%0X"%v1)
    self._assert_true((v2==TP_PHASE_DEFAULT_VALUE),"TP phase value 0x%0X"%v2)
    self.results['TP_PERIOD']=v1
    self.results['TP_PHASE']=v2

    r,v=self.tpx.ctrl.getTpNumber(self.tpx.id)
    self._assert_true(r,"Reading TP Number")
    TP_NUMBER_DEFAULT_VALUE=0x0
    self._assert_true((v==TP_NUMBER_DEFAULT_VALUE),"TP number value 0x%0X"%v)
    self.results['TP_NUMBER']=v1


    return

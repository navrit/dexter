from tpx3_test import tpx3_test

class test02(tpx3_test):
  """Default register values"""

  def _execute(self,**keywords):
    cat=keywords['category']
    self.tpx.reinitDevice()
    r,v=self.tpx.ctrl.getDeviceId(self.tpx.id)
    self._assert_true(r,"Reading device ID")
#    DEF_DEVIDE_ID=0
#    self._assert_true((v==DEF_DEVIDE_ID),"Device ID 0x%0X"%v)
    self.logging.info("Device ID 0x%08X"%v)
    self.logging.info("Device name %s"%self.tpx.readName())
    
    
    r,v=self.tpx.ctrl.getGenConfig(self.tpx.id)
    self._assert_true(r,"Reading General Config")
    GEN_CONFIG_DEFAULT_VALUE=1
    self._assert_true((v==GEN_CONFIG_DEFAULT_VALUE),"General Config value 0x%0X"%v)

    r,v=self.tpx.ctrl.getPllConfig(self.tpx.id)
    self._assert_true(r,"Reading PLL Config")
    PLL_CONFIG_DEFAULT_VALUE=0x1E
    self._assert_true((v==PLL_CONFIG_DEFAULT_VALUE),"PLL Config value 0x%0X"%v)

    r,v=self.tpx.ctrl.getOutBlockConfig(self.tpx.id)
    self._assert_true(r,"Reading Output Block Config")
    OUT_BLOCK_CONFIG_DEFAULT_VALUE=[0x7BFF, 0x903, 0x901, 0x9FF]#0x7B01#0x9FF
    self._assert_true((v in OUT_BLOCK_CONFIG_DEFAULT_VALUE),"Output Block value 0x%0X"%v)

    r,v1,v2=self.tpx.ctrl.getTpPeriodPhase(self.tpx.id)
    self._assert_true(r,"Reading TP Period & Phase")
    TP_PERIOD_DEFAULT_VALUE=0x0
    TP_PHASE_DEFAULT_VALUE=0x0
    self._assert_true((v1==TP_PERIOD_DEFAULT_VALUE),"TP period value 0x%0X"%v1)
    self._assert_true((v2==TP_PHASE_DEFAULT_VALUE),"TP phase value 0x%0X"%v2)

    r,v=self.tpx.ctrl.getTpNumber(self.tpx.id)
    self._assert_true(r,"Reading TP Number")
    TP_NUMBER_DEFAULT_VALUE=0x0
    self._assert_true((v==TP_NUMBER_DEFAULT_VALUE),"TP number value 0x%0X"%v)
    return {'category':cat,'info':keywords['info'], 'continue':True}

from tpx3_test import tpx3_test
import logging
import Gnuplot, Gnuplot.funcutils

class test17_efuse(tpx3_test):
  """Efuse test"""

  def _execute(self,**keywords):
    self.tpx.reinitDevice()
    eth_filter,cpu_filter=self.tpx.getHeaderFilter()
    self.tpx.setHeaderFilter(0xffff,cpu_filter) # cpu should not see 0x90 packets
    
    self.tpx.readEfuse()
    self.tpx.burnEfuse(0,1)
    self.tpx.readEfuse()



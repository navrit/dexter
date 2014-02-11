from tpx3_test import tpx3_test
from SpidrTpx3_engine import *

class test99_summary(tpx3_test):
  """Summary display (category + problematic pixels)"""
  def _execute(self,**keywords):
    cat=keywords['category']
    mask_pixels=keywords['mask_pixels']
    ret_values={'CATEGORY':cat,'BAD_PIXELS':len(mask_pixels)}
    fn=self.fname+"/results.txt"
    self.dict2file(fn,ret_values)
    self.logging.info("Results stored to %s"%fn)
    return {'category':cat,'info':keywords['info'], 'continue':False}


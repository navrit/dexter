from tpx3_test import tpx3_test

class test05_ctpr(tpx3_test):
  """Test CTPR write/read"""

  def _execute(self):
    self.tpx.send_byte_array([0xd0,0,0])
    resp=self.tpx.recv(130)
#    for p in resp:
#      print p


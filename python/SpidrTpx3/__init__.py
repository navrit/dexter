import sys
import os
d=os.path.dirname(__file__)
d=d[:-len('SpidrTpx3')]
sys.path.append(d+"/tmp/")
print sys.path
from tpx3 import TPX3
from tpx3 import tpx3packet



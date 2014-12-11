import sys
import os
d=os.path.dirname(__file__)
d=d[:-len('tuomas')]
sys.path.append(d+"/tmp/")

from TPX3Conf import TPX3ConfBase
from TPX3Conf import TPX3ConfMatrixTPEnable
from TPX3Conf import TPX3ConfBeforeDAQ
from TPX3Conf import TPX3ConfTestPulses
from TPX3Seq import TPX3SeqBase
from TPX3Seq import TPX3DataDrivenSeq

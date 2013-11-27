import inspect
import logging
from SpidrTpx3_engine import *
#logging.basicConfig(format='[%(levelname)6s] [%(asctime)s] %(message)s',level=logging.DEBUG)

class tpx3_test(object):
  """ Abstract tpx3 test object"""
  wiki=False
  def __init__(self, tpx,fname=""):
    self.tpx=tpx
    self.console=80
    self.errors=[]
    self.fname=fname

    
  def execute(self,**keywords):
    BLEN=100
    BSPACERLEN=BLEN+4
    
    logging.info("#"*BSPACERLEN)
    logging.info("# %-100s #"%self.__doc__.split('\n')[0])
    m="%s@%s"%(self.__class__.__name__ ,inspect.getfile(self.__class__))
    logging.info("# %-100s #"%m)
    
    if len(keywords)>0:
      logging.info("# %-100s #"%"Run time parameters:")
      for key, value in keywords.iteritems():
        l=" %s = %s"% (key, value)
        if not key in ['wiki']:
          self.fname+="_%s%s"%(key, value)
        logging.info("# %-100s #"%l)
    logging.info("# %-100s #"%("Deafult filename : %s"%self.fname))
    logging.info("#"*BSPACERLEN)
    self._execute(**keywords)

  def _execute(self,**keywords):
    print "Virtual !!"
    
  def _assert_true(self,val,msg):
    if val:
      logging.info("%-95s [  OK  ]"%msg)
    else:
      logging.error("%-95s [FAILED]"%msg)

    if not val:
      self.errors.append(msg)

  def _assert_in_range(self,val,min,max,msg):
    ok=False
    if val>=min and val<=max:
      ok=True
      logging.info("%-95s [  OK  ]"%msg)
    else:
      logging.error("%-95s [FAILED]"%msg)
    
    if not ok:
      self.errors.append(msg)

  def _assert_equal(self,val,min,max,msg):
    ok=False
    if val>=min and val<=max:
      ok=True
    self.log(msg,ok)
    if not ok:
      self.errors.append(msg)
      
  def mkdir(self,d):
    if not os.path.exists(d):
      os.makedirs(d)  
      logging.info("Creating directory '%s'"%d)

  def wiki_banner(self,**keywords):
    if "wiki" in keywords and keywords["wiki"] :
      dac_name=  ['none','IB_PRE_ON',  'IB_PRE_OFF','VPRE_NCAS',     'IB_IKRUM',
                  'VFBK',       'VTHR_FIN',  'VTHR_COA',      'IB_DIS1_ON',
                  'IB_DIS1_OFF','IB_DIS2_ON','IB_DIS2_OFF',   'IB_PIXDAC',
                  'IB_TPBIN',   'IB_TPBOUT', 'VTP_COA',       'VTP_FINE',
                  'IB_CP_PLL','PLL_VCNTRL'] 
      logging.info("# %-100s #"%"Settings")
      l=""
      for di in range(1,len(dac_name)):
        tl="%2d) %s = %d "%(di,dac_name[di],self.tpx.getDac(di))
        l+="%-25s"%tl
        if (di-1)%4==3 or di==len(dac_name)-1: 
          logging.info("# %-100s #"%l)
          l=""
      gc=self.tpx.getGenConfig()
      pll=self.tpx.getPllConfig()
      gc_line="Gen Config 0x%04x ("%gc
      
      if gc&TPX3_POLARITY_EMIN:
        gc_line+="electrons,"
      else:
        gc_line+="holes,"
        
      if gc&0x6==TPX3_ACQMODE_TOA_TOT:
        gc_line+="ToA&ToT,"
      elif gc&0x6==TPX3_ACQMODE_TOA:
        gc_line+="ToA,"
      elif gc&0x6==TPX3_ACQMODE_EVT_ITOT:
        gc_line+="Event&iToT,"

      if gc&TPX3_GRAYCOUNT_ENA :
        gc_line+="GC ena,"
      else:
        gc_line+="GC dis,"

      if gc&TPX3_ACKCOMMAND_ENA :
        gc_line+="ACK ena,"
      else:
        gc_line+="ACK dis,"

      if gc&TPX3_TESTPULSE_ENA :
        gc_line+="TestPulse ena,"
      else:
        gc_line+="TestPulse dis,"

      if gc&TPX3_FASTLO_ENA :
        gc_line+="SP oscilator ena"
      else:
        gc_line+="SP oscilator dis"

      gc_line+=")"
      
      logging.info("# %-100s #"%gc_line)
      logging.info("# %-100s #"%"PLL Config 0x%04x"%pll)
      logging.info("#"*104)



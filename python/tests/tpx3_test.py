import inspect
import logging
from SpidrTpx3_engine import *
import os
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

    self.mkdir(self.fname)
    logname='%s/log.txt'%(self.fname)
    self.logging = logging.getLogger(self.__class__.__name__)
    self.logging.setLevel(logging.DEBUG)
    fh = logging.FileHandler(logname,mode='w')
    fh.setLevel(logging.DEBUG)
    formatter = logging.Formatter( '[%(levelname)7s] [%(relativeCreated)5d] %(message)s',  datefmt='%M:%S')
    fh.setFormatter(formatter)
    self.logging.addHandler(fh)


    self.logging.info("")
    self.logging.info("#"*BSPACERLEN)
    self.logging.info("# %-100s #"%self.__doc__.split('\n')[0])
    m="%s@%s"%(self.__class__.__name__ ,inspect.getfile(self.__class__))
    self.logging.info("# %-100s #"%m)
    
    if len(keywords)>0:
      self.logging.info("# %-100s #"%"Run time parameters:")
      for key, value in keywords.iteritems():
        l=" %s = %s"% (key, value)
        if not key in ['wiki','category', 'info', 'mask_pixels','continue','name']:
          if str(value)!="":
            self.fname+="_%s%s"%(key, value)
        if not key in ['mask_pixels']:
          self.logging.info("# %-100s #"%l)
        else:
          lb=" %s : "% (key)
          l=lb
          for i,p in enumerate(value):
            l+="(%3d,%3d) "%p
            if (i)%8==7:
              self.logging.info("# %-100s #"%l)
              l=" "*len(lb)
          if len(l)>len(lb):
            self.logging.info("# %-100s #"%l)

    self.logging.info("# %-100s #"%("Data directory : %s"%self.fname))
    self.logging.info("# %-100s #"%("Log file : %s"%logname))
    self.logging.info("#"*BSPACERLEN)

    result=self._execute(**keywords)
    if not result:
       logging.warning("Test didn't return any result!")
       result={'category':'Z','continue':False}
    if not 'category' in result: 
       logging.warning("Test should return category!")
    if not 'continue' in result: 
       logging.warning("Test should return continue flag!")
    if not 'mask_pixels' in result: 
       result['mask_pixels']=[]
    return result
  def _execute(self,**keywords):
    print "Virtual !!"
    
  def _assert_true(self,val,msg):
    if val:
      self.logging.info("%-95s [  OK  ]"%msg)
    else:
      self.logging.error("%-95s [FAILED]"%msg)

    if not val:
      self.errors.append(msg)

  def _assert_in_range(self,val,min,max,msg):
    if val>=min and val<=max:
      self.logging.info("%-95s [  OK  ]"%msg)
      return True
    else:
      self.logging.error("%-95s [FAILED]"%msg)
      return False
    


      
  def mkdir(self,d):
    if not os.path.exists(d):
      os.makedirs(d)  
#      self.logging.info("Creating directory '%s'"%d)

  def dict2file(self,fname,d):
    f=open(fname,"w")
    for k in d.keys():
      f.write("%s %s\n"%(k,d[k]))
    f.close()

    
    
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
      
      self.logging.info("# %-100s #"%gc_line)
      self.logging.info("# %-100s #"%"PLL Config 0x%04x"%pll)
      self.logging.info("#"*104)



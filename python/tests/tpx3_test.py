import inspect
import logging

#logging.basicConfig(format='[%(levelname)6s] [%(asctime)s] %(message)s',level=logging.DEBUG)

class tpx3_test(object):
  """ Abstract tpx3 test object"""

  def __init__(self, tpx,fname=""):
    self.tpx=tpx
    self.console=80
    self.errors=[]
    self.fname=fname

    
  def execute(self,**keywords):
    logging.info("#"*89)
    logging.info("# %-85s #"%self.__doc__.split('\n')[0])
    m="%s@%s"%(self.__class__.__name__ ,inspect.getfile(self.__class__))
    logging.info("# %-85s #"%m)
    if len(keywords)>0:
      logging.info("# %-85s #"%"Run time parameters:")
      for key, value in keywords.iteritems():
        l=" %s = %s"% (key, value)
        logging.info("# %-85s #"%l)
    logging.info("#"*89)
    self._execute(**keywords)

  def _execute(self,**keywords):
    print "Virtual !!"
    
  def _assert_true(self,val,msg):
    if val:
      logging.info("%-80s [  OK  ]"%msg)
    else:
      logging.error("%-80s [FAILED]"%msg)

    if not val:
      self.errors.append(msg)

  def _assert_in_range(self,val,min,max,msg):
    ok=False
    if val>=min and val<=max:
      ok=True
      logging.info("%-80s [  OK  ]"%msg)
    else:
      logging.error("%-80s [FAILED]"%msg)
    
    if not ok:
      self.errors.append(msg)

  def _assert_equal(self,val,min,max,msg):
    ok=False
    if val>=min and val<=max:
      ok=True
    self.log(msg,ok)
    if not ok:
      self.errors.append(msg)



#!/usr/bin/env python
from appy.pod.renderer import Renderer
import sys
from time import gmtime, strftime
import time
from appy.pod import PodError
import os
def gen_report(data,fout):

  #print data['dacs']
  if os.path.isfile(fout):
    os.remove(fout)
  renderer = Renderer('report.odt', data, fout)
  while 1:
    try :
      renderer.run()
      break
    except PodError:
      print "starting open office deamon"
      os.system('soffice --invisible --headless "--accept=socket,host=localhost,port=2002;urp;" &')
    except:
      print "Unexpected error:", sys.exc_info()[0]
      raise


if __name__=="__main__":
  main()

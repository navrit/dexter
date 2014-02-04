from tpx3_test import tpx3_test
import logging
import Gnuplot, Gnuplot.funcutils


import sys

def query_yes_no(question, default="yes"):
    """Ask a yes/no question via raw_input() and return their answer.

    "question" is a string that is presented to the user.
    "default" is the presumed answer if the user just hits <Enter>.
        It must be "yes" (the default), "no" or None (meaning
        an answer is required of the user).

    The "answer" return value is one of "yes" or "no".
    """
    valid = {"yes":True,   "y":True,  "ye":True,
             "no":False,     "n":False}
    if default == None:
        prompt = " [y/n] "
    elif default == "yes":
        prompt = " [Y/n] "
    elif default == "no":
        prompt = " [y/N] "
    else:
        raise ValueError("invalid default answer: '%s'" % default)

    while True:
        sys.stdout.write(question + prompt)
        choice = raw_input().lower()
        if default is not None and choice == '':
            return valid[default]
        elif choice in valid:
            return valid[choice]
        else:
            sys.stdout.write("Please respond with 'yes' or 'no' \
                             (or 'y' or 'n').\n")

class test17_efuse(tpx3_test):
  """Efuse test"""

  def _execute(self,**keywords):
    self.tpx.reinitDevice()
    name=None
    if 'name' in keywords :  name=keywords['name'].upper()
    name
    efuses=self.tpx.readEfuse()
    bname=self.tpx.readName()
    
    logging.info( "Efuses value : 0x%08x"%efuses)
    logging.info( "Chip name    : %s"%bname)
    if name!=None and bname=='-':
      err=None
      if (name[0] != 'W' ) or len(name.split("_"))!=2:
        err="Wrong wafer name format (e.g. W000_D5)"
      if not err:
        wnum=int(name.split("_")[0][1:])
        xs=name.split("_")[1][0]
        x=(ord(xs)-ord('A') +1)&0xf
        y=int(name.split("_")[1][1:])
        if (xs=='A' and y>=5 and y<=7) or\
           (xs=='B' and y>=4 and y<=9) or\
           (xs=='C' and y>=3 and y<=10) or\
           (xs=='D' and y>=2 and y<=10) or\
           (xs=='E' and y>=2 and y<=11) or\
           (xs=='F' and y>=1 and y<=11) or\
           (xs=='G' and y>=1 and y<=11) or\
           (xs=='H' and y>=1 and y<=11) or\
           (xs=='I' and y>=2 and y<=11) or\
           (xs=='J' and y>=2 and y<=10) or\
           (xs=='K' and y>=3 and y<=10) or\
           (xs=='L' and y>=4 and y<=9) or\
           (xs=='M' and y>=5 and y<=7) :
            logging.info( "Wafer number : %d"%wnum)
            logging.info( "X            : %s (%d)"%(xs,x))
            logging.info( "Y            : %d"%(y))
            efuses=x | y<<4 | wnum<<8
            logging.info( "Going to burn 0x%08x"%(efuses))
            burn_ok=query_yes_no("Burn ?!",default="no")
            if burn_ok:
              for bit in range(20):
                if 1<<bit & efuses :
#                  print "burn bit no.",bit
                  self.tpx.burnEfuse(bit,program_width=1)
                  x=self.tpx.readEfuse()
                  logging.info( " %d) efuses : 0x%08x"%(bit,x))

              efuses_final=self.tpx.readEfuse()
              bname_final=self.tpx.readName()
              logging.info( "Efuses value : 0x%08x"%efuses_final)
              logging.info( "Chip name    : %s"%bname_final)
              if efuses_final==efuses:
                logging.info( "Burning efuses succeded")
              else:
                logging.error( "Burning efuses failed (is:0x%08x, should be:0x%08x)"%(efuses_final,efuses))
              

        else:
          err="Wrong die specification (e.g. W000_D5)"
        
      if err:
        logging.error(err)


from tpx3_test import tpx3_test
import logging
import Gnuplot, Gnuplot.funcutils
import time

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

SPIDR_3V3_ENA_PIN=0

def transitions(prev,next):
  zero2one=0
  one2zero=0
  while prev>0 or next>0:
    pbit=prev&0x1
    nbit=next&0x1
    if pbit==0 and nbit==1: zero2one+=1
    if pbit==1 and nbit==0: one2zero+=1
    prev>>=1
    next>>=1
  mod=0
  if one2zero>0:mod=1
  
  return {'zero2one':zero2one,'one2zero':one2zero, 'mod':mod}

def str2bool(s):
  return s.lower() in ['true', '1', 't', 'y', 'yes']

class test17_efuse(tpx3_test):
  """Efuse test"""

  def _execute(self,**keywords):
    name=None
    force=False
    if 'name' in keywords :  name=keywords['name'].upper()
    if 'force' in keywords :  force=str2bool(keywords['force'])

    efuses=self.tpx.readEfuses()
    self.logging.info( "Efuses value : 0x%08x"%efuses)
    bname= self.tpx.readName()
    self.logging.info( "Chip name    : %s"%bname)

    self.results={'EFUSES':"%08x"%efuses, "NAME":bname}
    
    if name!=None :
      r=self.tpx.decode_die_name(name)
      if r==None:
        self.logging.error("Provided name to burn is incorect")
      else:
        name=self.tpx.generate_die_name(*r)
        if name!=bname:
          r=list(r)
          burned_x=efuses&0xf
          burned_y=(efuses>>4)&0xf
          burned_wnum=(efuses>>8)&0xfff
          burned_mod=(efuses>>20)&0x3
          burned_mod_val=(efuses>>22)&0x3FF
          new_wnum , new_x , new_y = r
          desired_efuses = new_x | new_y<<4 | new_wnum<<8

          self.logging.info( "Trying to burn : %s"%name)
          self.logging.info( "Burned  efuses : %08x (wnum:%03x x:%0x y:%0x mod:%x mod_val:%x)"%(efuses,burned_wnum , burned_x , burned_y, burned_mod, burned_mod_val ) )
          self.logging.info( "Desired efuses : %08x (wnum:%03x x:%0x y:%0x)"%(desired_efuses,new_wnum , new_x , new_y ) )
          
          xt=transitions(burned_x, new_x)
          yt=transitions(burned_y, new_y)
          wnumt=transitions(burned_wnum, new_wnum)
          fields_to_replace=xt['mod']+yt['mod']+wnumt['mod']
          if fields_to_replace>0 and burned_mod:
            msg="Unable to burn fuses. Modification field is alread used"
            self.logging.error(msg)
            self.update_category('F_fuse')
          elif fields_to_replace>1:
            msg="Unable to burn fuses, more then one field has to be replaced ("
            if wnumt['mod'] : msg+="wnum "
            if xt['mod'] : msg+="x "
            if yt['mod'] : msg+="y "
            msg=msg[:-1]+")"
            self.logging.error(msg)
            self.update_category('F_fuse')
          else:
            self.logging.info("Look like burning is possible")
            if xt['one2zero'] :
              self.logging.info("Using modification field for x")
              desired_efuses &=  ~ (0xF)
              desired_efuses |=  (1<<20) | (new_x<<22) 
              
            if yt['one2zero'] :
              desired_efuses &=  ~ (0xF<<4)
              self.logging.info("Using modification field for y")
              desired_efuses |=  (2<<20) | (new_y<<22)
            if wnumt['one2zero'] :
              desired_efuses &=  ~ (0x3FF<<8)
              self.logging.info("Using modification field for wnum")
              desired_efuses |=  (3<<20) | ( (new_wnum&0x3FF) <<22)

            new_mod=(desired_efuses>>20)&0x3
            new_mod_val=(desired_efuses>>22)&0x3FF
            self.logging.info( "Desired efuses  (after mod): %08x (wnum:%03x x:%0x y:%0x mod:%x mod_val:%x)"%(desired_efuses,new_wnum , new_x , new_y, new_mod, new_mod_val ) )
            final=transitions(efuses,desired_efuses)
            self.logging.info( "  Zero -> one : %d"%( final['zero2one']) )
            self.logging.info( "  One -> zero : %d"%( final['one2zero']) )
            
            bit_pos=0
            val=desired_efuses
            pval=efuses
            bits_to_burn=[]
            while val>0:
              if val&0x1 and (not pval&0x1):
                bits_to_burn.append(bit_pos)
              val>>=1
              pval>>=1
              bit_pos+=1
            self.logging.info( "Going to burn : %s"%str(bits_to_burn) )

            burn_ok=True
            if not force:
              burn_ok=query_yes_no("Burn ?!",default="no")

            if burn_ok:
              self.tpx.setGPIO(SPIDR_3V3_ENA_PIN,1)
              time.sleep(0.02)
              for fuse in bits_to_burn:
                self.logging.info( "  Burning efuse %d"%(fuse) )
                rd=self.tpx.burnEfuse(selection=fuse,program_width=1)
                rd=self.tpx.readEfuses()
                if not rd&(1<<fuse): 
                  self.logging.error( "  Problem with burning efuse %d. Ending ..."%(bit_pos))
                  self.update_category('F_fuse')
                  break
              self.tpx.setGPIO(SPIDR_3V3_ENA_PIN,0)
              time.sleep(0.01)
              efuses=self.tpx.readEfuses()
              self.logging.info( "Efuses value : 0x%08x"%efuses)
              bname= self.tpx.readName()
              self.logging.info( "Chip name    : %s"%bname)

              self.results['EFUSES']="%08x"%efuses
              self.results['NAME']=bname

            else:
              self.logging.warning( "Operation canceled by user")
            

    return 
    

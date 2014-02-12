from tpx3_test import *
import os
import random
import time
import logging
from SpidrTpx3_engine import *
import numpy
from scipy.optimize import curve_fit
import matplotlib.pyplot as plt
from scipy.special import erf
import numpy as np
from dac_defaults import dac_defaults

def line(x, *p):
    g = p[0]
    return  numpy.array(x) *g


def gauss(x, *p):
    A, mu, sigma = p
    return A*numpy.exp(-(x-mu)**2/(2.*sigma**2))
    
    
def errorf(x, *p):
    a, mu, sigma = p
#    print x
    return 0.5*a*(1.0+erf((x-mu)/sigma))


class sGnuplot:
    def __init__(self,fname):
        self.fout=fname
        self.fgnu=fname[:-4]+".gnu"
        self.gnu=open(self.fgnu,"w")
        self("set output '%s'"%self.fout)

    def __call__(self, *args):
        for a in args:
          self.gnu.write(a+"\n")
    def run(self):
        self.gnu.close()
        os.system("gnuplot %s"%self.fgnu)

class test04_diagonal_scurves(tpx3_test):
  """Diagonal scurves
  
Parameters:
electrons - polarity bit set to electrons 
th_start  - threshold start [LSB] (defult 0)
th_stop   - threshold stop [LSB] (defult 511)
th_step   - threshold step size [LSB] (defult 4)"""

  def _execute(self,**keywords):
    mask_pixels=keywords['mask_pixels']
    cat=keywords['category']
    self.tpx.reinitDevice()
    self.tpx.resetPixels()
    params={}
    params['electrons']=False
    if 'electrons' in keywords :     params['electrons']=True
    params['amps']=[0,1000,2000]
    params['shutter_len']=15000
    params['shutter_len_noise']=500
    params['th_start']=0
    params['th_stop']=511
    params['th_step']=2
    if 'th_start' in keywords : params['th_start' ]=int(keywords['th_start'])
    if 'th_stop' in keywords : params['th_stop' ]=int(keywords['th_stop'])
    if 'th_step' in keywords : params['th_step' ]=int(keywords['th_step'])

    params['gen_config'] = TPX3_ACQMODE_EVT_ITOT|TPX3_TESTPULSE_ENA
    params['pll_config'] = TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK | TPX3_PHASESHIFT_DIV_8 | TPX3_PHASESHIFT_NR_16

    if params['electrons']:
       params['gen_config']|=TPX3_POLARITY_EMIN
       params['shutter_len']=params['shutter_len']
       params['shutter_len_noise']=params['shutter_len_noise']/2

    self.tpx.setGenConfig(params['gen_config'])
    self.tpx.setPllConfig(params['pll_config'])
    self.tpx.setTpPeriodPhase(0xF,0)
    TESTPULSES=250
    self.tpx.setTpNumber(TESTPULSES)
    self.tpx.setCtprBits(1)
    self.tpx.setCtpr()
    self.tpx.sequentialReadout()



    self.logging.info("Optimization of DC operating point")
#    dac_defaults(self.tpx)
    self.tpx.setDac(TPX3_IBIAS_IKRUM,15)
    self.tpx.setDac(TPX3_VTHRESH_FINE,256)
    self.tpx.setDac(TPX3_VTHRESH_COARSE,7) 

    self.tpx.setSenseDac(TPX3_VTHRESH_COARSE)
    vthcorse=self.tpx.get_adc(1)
    time.sleep(0.001)
    vthcorse=self.tpx.get_adc(64)
    self.logging.info("  TPX3_VTHRESH_COARSE code=7 voltage=%.1f mV"%(1000.0*vthcorse))

    best_vfbk_val=0
    best_vfbk_code=0
    self.tpx.setSenseDac(TPX3_VFBK)
    for code in range(64,192):
      self.tpx.setDac(TPX3_VFBK,code) 
      vfbk=self.tpx.get_adc(1)
      time.sleep(0.001)
      vfbk=self.tpx.get_adc(16)
      if abs(vfbk-vthcorse)<abs(best_vfbk_val-vthcorse):
        best_vfbk_val=vfbk
        best_vfbk_code=code
    self.tpx.setDac(TPX3_VFBK,best_vfbk_code) 
    vfbk=self.tpx.get_adc(1)
    time.sleep(0.001)
    vfbk=self.tpx.get_adc(8)
    self.logging.info("  TPX3_VFBK code=%d voltage=%.1f mV"%(best_vfbk_code,(1000.0*vfbk)))


    self.wiki_banner(**keywords)

    self.tpx.setDac(TPX3_VTP_COARSE,64)
    self.tpx.setSenseDac(TPX3_VTP_COARSE)
    coarse=self.tpx.get_adc(32)
    fit_res={}
    amp_meas=[0.0]
    for amp in range(len(params['amps'])):
      fit_res[amp]={}
      dv=0
      electrons=0
      if amp>0:
        self.tpx.setSenseDac(TPX3_VTP_FINE)
        time.sleep(0.1)
        best_vtpfine_diff=1e3
        best_vtpfine_code=0
        for code in range(64,512):
          self.tpx.setDac(TPX3_VTP_FINE,code) # (0e-) slope 44.5e/LSB -> (112=1000e-)  (135=2000e-)
          time.sleep(0.001)
          fine=self.tpx.get_adc(8)
          dv=1000.0*(fine-coarse)
          electrons=20.0*abs(dv)
          diff=params['amps'][amp]-electrons
#          print "%d %6.4f %6.4f dv %6.4f el %6.1f"%(code, fine, coarse,dv,electrons)
          if diff>0: break
          diff=abs(diff)
          if diff<best_vtpfine_diff:
            best_vtpfine_diff=diff
            best_vtpfine_code=code
        self.tpx.setDac(TPX3_VTP_FINE,best_vtpfine_code) 
        fine=self.tpx.get_adc(64)
        dv=1000.0*abs(fine-coarse)
        amp_meas.append(dv)
        electrons=20.0*dv
        logging.info("  TPX3_VTP_FINE code=%d voltage=%.1f"%(best_vtpfine_code,fine*1000.0))

        logging.info("Test pulse voltage %.4f mv (~ %.0f e-)"%(dv,electrons))
        

      else:
        logging.info("Test pulse voltage 0.0 mv (0 e-)")

      M=(params['th_stop']+1-params['th_start'])
      res={}
      if 1:
        self.tpx.resetPixelConfig()
        self.tpx.setPixelMask(ALL_PIXELS,ALL_PIXELS,1)
        self.tpx.setPixelThreshold(ALL_PIXELS,ALL_PIXELS,8)
                          
        for x in range(256):
            if not (x,x) in mask_pixels : self.tpx.setPixelMask(x,x, 0)
            if amp>0:
                self.tpx.setPixelTestEna(x,x, testbit=True)
        self.tpx.setPixelConfig()
        
        st=params['shutter_len']
        if amp==0:
            st=params['shutter_len_noise']
        self.tpx.setShutterLen(st)
        
        self.tpx.flush_udp_fifo(0x718F000000000000)
        for threshold in range(params['th_start'],params['th_stop']+1,params['th_step']):
            self.tpx.setDac(TPX3_VTHRESH_FINE,threshold)
            self.tpx.openShutter()
            time.sleep(0.001)
            data=self.tpx.recv_mask(0x71A0000000000000, 0xFFFF000000000000)
            
#            data=self.tpx.recv_mask(0x71AF000000000000, 0xFFFF000000000000)

            logging.debug("Packets received %d"%(len(data)))
            
            for d in data:
                if d.type==0xA:
                  if d.col==d.row:
                    if not d.col in res:
                        res[d.col]={}
                    if not d.row in res[d.col]:
                        res[d.col][d.row]={}
                    res[d.col][d.row][threshold]=d.event_counter
                  elif not (d.col, d.row) in mask_pixels: #suppres information about the pixels which are know be be bad/noisy/...
                    self.logging.warning("Unexpected packet %s"%str(d))
                elif d.type!=0x7:
                  self.logging.warning("Unexpected packet %s"%str(d))

      w2f=True
      fit=True
      if 1: #fitting
        if w2f:
          dn=self.fname+"/amp%02d/"%(amp)
          logging.info("Saving files to %s"%dn)
          self.mkdir(dn)
          g=sGnuplot(dn+"plot.png")
          g("set terminal png size 800,600","set grid","set xtic 64","set xr [0:512]","set yr [0:1024]","set ytic 128", "set xlabel 'Threshold[LSB]'", "set ylabel 'Counts'" )#, "set key out top cent samp 0.1"
          pcmd='plot '
          for col in range(256):
            row =col
            if (col,row) in mask_pixels : 
              continue
            if not col in res or not row in res[col] :
              self.logging.warning("No data for pixel (%d,%d)"%(col,row))
              continue
            if w2f:
              fn=dn+"/%03d_%03d.dat"%(col,row)
              f=open(fn,"w")
              f.write("# amp step %d TPX3_VTP_FINE %d\n"%(amp,params['amps'][amp]))
              f.write("# voltage  %.4f (measured)\n"%(dv))
              f.write("# charge   %.1f (estimated)\n"%(electrons))
              for code in sorted(res[col][row]):
                  f.write("%d %d\n"%(code,res[col][row][code]))
              f.close()
              if len(pcmd)>5:
                 pcmd+=','
              pcmd+="'%s' w l t '' "%(fn)
            if fit:
              if amp==0: #gauss fitting
                codes=[]
                vals=[]
                avr=0.0
                N=0
                for code in sorted(res[col][row]):
                  val=res[col][row][code]
                  if val>2:
                    codes.append(code)
                    vals.append(val)
                    avr+=code*val
                    N+=val
                fit_res[amp][col] =-1.0
                if N>2:
                  avr=avr/N
                  try:
                    p0 = [max(vals), avr, 6.]
                    coeff, var_matrix = curve_fit(gauss, codes, vals, p0=p0)
                    fit_res[amp][col] =coeff[1]
                  except:
                    pass
                else:
                    self.logging.warning("No hits for pixel (%d,%d)"%(col,row))
              else:
                codes=[]
                vals=[]
                avr=0.0
                N=0
                for code in sorted(res[col][row]):
                  if code>fit_res[0][col]: continue
                  val=res[col][row][code]
                  if val<(TESTPULSES+2):
                    codes.append(code)
                    vals.append(val)
                    avr+=code*val
                    N+=val
                fit_res[amp][col] =-1.0
                if N>2:
                  avr=avr/N
                  try:
#                  if 1:
                    p0 = [TESTPULSES, min(codes)+5, 6.]
                    coeff, var_matrix = curve_fit(errorf, codes, vals, p0=p0)
                    fit_res[amp][col] =coeff[1]
#                    f=open("/tmp/f%d.dat"%col,"w")
#                    for c in range(len(codes)):
#                      fit=errorf(codes[c],*coeff)
#                      f.write("%d %d %d\n"%(codes[c],vals[c],fit))
#                    f.close()
                  except:
                    pass
                else:
                    self.logging.warning("No hits for pixel (%d,%d)"%(col,row))
          g(pcmd)
          g.run()
          logging.info("Saving plot to %s"%g.fout)
    gains=[]
    for col in range(256):
      
      if not col in fit_res[0] or fit_res[0][col]<0:
         logging.info("No baseline point for pixel (%d,%d)"%(col,col))
         continue
      Y=[0.0]
      for amp in range(1,len(params['amps'])):
        if not col in fit_res[0] or fit_res[0][col]<0.0:
          logging.info("No amplitude %d point for pixel (%d,%d)"%(amp,col,col))
          continue
        Y.append(-fit_res[amp][col]+fit_res[0][col])

      if len(Y)!=len(amp_meas): continue
      p0 = [Y[-1]/amp_meas[-1],]
      coeff, var_matrix = curve_fit(line, amp_meas, Y, p0=p0)
      gains.append(coeff[0])
    ret_values={}
    ret_values['GAIN_MEAN']=numpy.mean(gains)
    ret_values['GAIN_RMS']=numpy.std(gains)
    proc=100.0*ret_values['GAIN_RMS']/ret_values['GAIN_MEAN']
    self.logging.info("")
    electrons=20.0/ret_values['GAIN_MEAN']
    self.logging.info("Gain mean %.3f +/- %.3f LSB/mV (~%.1f%%)  [~%.1f e- / TH LSB]"%(ret_values['GAIN_MEAN'],ret_values['GAIN_RMS'],proc,electrons))

    
    fn=self.fname+"/results.txt"
    self.dict2file(fn,ret_values)
    self.logging.info("Results stored to %s"%fn)
    
    return {'category':cat,'info':keywords['info'], 'continue':True}


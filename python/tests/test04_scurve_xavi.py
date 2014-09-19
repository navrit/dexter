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
    return 0.5*a*(1.0+erf((x-mu)/(sigma*1.4142)))

def errorfc(x, *p):
    a, mu, sigma = p
#    print x
    return 0.5*a*(1.0-erf((x-mu)/(sigma*1.4142)))


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

class test04_diagonal_scurves_xavi(tpx3_test):
  """Diagonal scurves
  
Parameters:
electrons - polarity bit set to electrons 
th_start  - threshold start [LSB] (default 0)
th_stop   - threshold stop [LSB] (default 511)
th_step   - threshold step size [LSB] (default 4)"""

  def _execute(self,**keywords):
    params={}
    params['electrons']=False
    if 'electrons' in keywords :     
    	params['electrons']=True
    params['amps']=[0,100]
    params['shutter_len']=15000
    params['shutter_len_noise']=300
    params['th_start']=300
    params['th_stop']=1300
    params['th_step']=2
    if 'th_start' in keywords : params['th_start' ]=int(keywords['th_start'])
    if 'th_stop' in keywords : params['th_stop' ]=int(keywords['th_stop'])
    if 'th_step' in keywords : params['th_step' ]=int(keywords['th_step'])

    params['gen_config'] = TPX3_ACQMODE_EVT_ITOT | TPX3_TESTPULSE_ENA #| TPX3_GRAYCOUNT_ENA
    params['pll_config'] = TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK | TPX3_PHASESHIFT_DIV_8 | TPX3_PHASESHIFT_NR_16

    polarity=True
    if params['electrons']:
       params['gen_config']|=TPX3_POLARITY_EMIN
       params['shutter_len']=params['shutter_len']
       params['shutter_len_noise']=params['shutter_len_noise']/2
       polarity=False
       
    self.tpx.setGenConfig(params['gen_config'])
    self.tpx.setPllConfig(params['pll_config'])
    period=0x4
    self.tpx.setTpPeriodPhase(period,0)
    TESTPULSES=1000
    self.tpx.setTpNumber(TESTPULSES)

    self.logging.info("Optimization of DC operating point")
    
    self.tpx.setDac(TPX3_IBIAS_IKRUM,5)
    self.tpx.setDac(TPX3_IBIAS_DISCS1_ON,100)
    self.tpx.setDac(TPX3_IBIAS_DISCS2_ON,128)
    self.tpx.setDac(TPX3_IBIAS_PREAMP_ON,128)
    self.tpx.setDac(TPX3_IBIAS_PIXELDAC,128)
    self.tpx.setDac(TPX3_VFBK,164)

    self.wiki_banner(**keywords)
    self.tpx.setDecodersEna(True)
    self.tpx.setDac(TPX3_VTP_COARSE,64)
    self.tpx.setSenseDac(TPX3_VTP_COARSE)
    coarse=self.tpx.get_adc(32)
    fit_res={}
    fit_res_noise={}
    amp_meas=[0.0]
    fbase="calib/"

    if polarity:
      codes_eq=numpy.loadtxt(fbase+"eq_codes_noiseFloor_ik15_w0c0.dat", int)
      mask=numpy.loadtxt(fbase+     "eq_mask_noiseFloor_ik15_w0c0.dat", int)
    else:
      codes_eq=numpy.loadtxt(fbase+"eq_code_pixelDAC_scan_elec.dat", int)
      mask=numpy.loadtxt(fbase+"eq_code_pixelDAC_scanMask_elec.dat", int)

    self.tpx.resetPixelConfig()

    for x in range(256):
         for y in range(256):
            self.tpx.setPixelThreshold(x,y,codes_eq[x][y])
            self.tpx.setPixelMask(x,y,mask[x][y])

    self.tpx.setCtprBits(0)

    for x in range(256):
        self.tpx.setCtprBit(x,1)
    
    self.tpx.setCtpr()
    
    logdir=self.fname+"/details/"

    VTP_value=[]
    for amp in range(len(params['amps'])):
      self.warning_detailed_restart()
      fit_res[amp]={}
      fit_res_noise[amp]={}
      dv=0
      electrons=0
      if amp>0:
        #optimize test pulse amplitude
        self.tpx.setSenseDac(TPX3_VTP_FINE)
        shutter_length=int(((2*(64*period+1)*TESTPULSES)/40) + 100)
        time.sleep(0.1)
        best_vtpfine_diff=2000
        best_vtpfine_code=0
        target=params['amps'][amp]
        for code in range(64,512):
          self.tpx.setDac(TPX3_VTP_FINE,code) # (0e-) slope 44.5e/LSB -> (112=1000e-)  (135=2000e-)
          time.sleep(0.001)
          fine=self.tpx.get_adc(64)
          electrons=1000.0*20*(fine-coarse)
          
          if abs(electrons-target)<abs(best_vtpfine_diff) and electrons>0:
              best_vtpfine_diff=electrons-target
              best_vtpfine_code=code
              print "%d %.3f"%(code, best_vtpfine_diff)
      #print "  TPX3_VTP_FINE code=%d %.2f v_fine=%.1f v_coarse=%.1f"%(best_vtpfine_code,best_vtpfine_diff,fine*1000.0,coarse*1000.0)
        self.tpx.setDac(TPX3_VTP_FINE,best_vtpfine_code) 
        fine=self.tpx.get_adc(64)
        dv=1000.0*abs(fine-coarse)
        amp_meas.append(dv)
        VTP_value.append(best_vtpfine_code)
        electrons=20.0*dv
        self.logging.info("  TPX3_VTP_FINE code=%d v_fine=%.1f v_coarse=%.1f"%(best_vtpfine_code,fine*1000.0,coarse*1000.0))
        self.logging.info("Test pulse voltage %.4f mv (~ %.0f e-)"%(dv,electrons))
      else:
        self.logging.info("Test pulse voltage 0.0 mv (0 e-)")

      M=(params['th_stop']+1-params['th_start'])
      res={}
      res_itot={}
      if 1:
       # self.tpx.resetPixelConfig()
        self.tpx.setPixelMask(ALL_PIXELS,ALL_PIXELS,1)
        #self.tpx.setPixelThreshold(ALL_PIXELS,ALL_PIXELS,8)

      for col in range(256):
        if mask[col][col]==0 :
          self.tpx.setPixelMask(col,col,0)
          if amp>0:
                self.tpx.setPixelTestEna(col,col, testbit=True)

      self.mask_bad_pixels()
      self.tpx.pauseReadout()
      self.tpx.setPixelConfig()
      self.tpx.sequentialReadout(tokens=1)
      self.tpx.resetPixels()
        
      if amp==0:
          shutter_length=params['shutter_len_noise']
      self.tpx.setShutterLen(shutter_length)
        
      for threshold in range(params['th_start'],params['th_stop']+1,params['th_step']):
            self.tpx.setThreshold(threshold)
            #print threshold
            self.tpx.openShutter()
            data=self.tpx.get_frame()
            for d in data:
                if d.type==0xA:
                  if d.col==d.row:
                    if not d.col in res:
                        res[d.col]={}
                        res_itot[d.col]={}
                    if not d.row in res[d.col]:
                        res[d.col][d.row]={}
                        res_itot[d.col][d.row]={}
                    res[d.col][d.row][threshold]=d.event_counter
                    res_itot[d.col][d.row][threshold]=d.event_counter
                  elif not (d.col, d.row) in self.bad_pixels: #suppres information about the pixels which are know be be bad/noisy/...
                    self.warning_detailed("Unexpected packet %s"%str(d))
                elif d.type!=0x7:
                  self.warning_detailed("Unexpected packet %s"%str(d))
      self.warning_detailed_summary()

      w2f=True
      fit=True

      if 1: #fitting
        if w2f:
          dn=logdir+"amp%02d/"%(amp)
          self.logging.info("Saving files to %s"%dn)
          self.mkdir(dn)
          g=sGnuplot(dn+"plot.png")
          g("set terminal png size 800,600","set grid","set xtic 64","set xr [0:512]","set yr [0:1024]","set ytic 128", "set xlabel 'Threshold[LSB]'", "set ylabel 'Counts'" )#, "set key out top cent samp 0.1"
          pcmd='plot '
          for col in range(256):
            row =col
            if (col,row) in self.bad_pixels : 
              continue
            if not col in res or not row in res[col] :
              self.logging.warning("No data for pixel (%d,%d)"%(col,row))
              continue
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
                fit_res_noise[amp][col] =-1.0
                if N>2:
                  avr=avr/N
                  try:
                    p0 = [max(vals), avr, 6.]
                    coeff, var_matrix = curve_fit(gauss, codes, vals, p0=p0)
                    fit_res[amp][col] =coeff[1]
                    fit_res_noise[amp][col] =coeff[2]
                    #print amp,col,coeff,codes,vals
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
                  if code<fit_res[0][col] and not polarity: continue    #electrons
                  if code>fit_res[0][col] and polarity: continue        #holes
                  val=res[col][row][code]
                  if val<(TESTPULSES+2):
                    codes.append(code)
                    vals.append(val)
                    avr+=code*val
                    N+=val
                fit_res[amp][col] =-1.0
                fit_res_noise[amp][col] =-1.0
                if N>2:
                  avr=avr/N
                  try:
#                  if 1:
                    if polarity:
                        p0 = [TESTPULSES, min(codes)+5, 6.]
                        coeff, var_matrix = curve_fit(errorf, codes, vals, p0=p0)
                    else:
                        p0 = [TESTPULSES, max(codes)-25, 6.]
                        coeff, var_matrix = curve_fit(errorfc, codes, vals, p0=p0)
                    fit_res[amp][col] =coeff[1]
                    fit_res_noise[amp][col] =coeff[2]
                    #print amp,col,coeff,p0,codes,vals
#                    f=open("/tmp/f%d.dat"%col,"w")
#                    for c in range(len(codes)):
#                      fit=errorf(codes[c],*coeff)
#                      f.write("%d %d %d\n"%(codes[c],vals[c],fit))
#                    f.close()
                  except:
                    pass
                else:
                    self.logging.warning("No hits for pixel (%d,%d)"%(col,row))
            if w2f:
              fn=dn+"/%03d_%03d.dat"%(col,row)
              f=open(fn,"w")
              f.write("# amp step %d TPX3_VTP_FINE %d\n"%(amp,params['amps'][amp]))
              f.write("# voltage  %.4f (measured)\n"%(dv))
              f.write("# charge   %.1f (estimated)\n"%(electrons))
              f.write("# THR:%.2f\tENC:%.2f\n"%(coeff[1],coeff[2]))
              for code in sorted(res[col][row]):
                  f.write("%d %d\n"%(code,res[col][row][code]))
              f.close()
              if len(pcmd)>5:
                 pcmd+=','
              pcmd+="'%s' w l t '' "%(fn)

        g(pcmd)
        g.run()
        self.logging.info("Saving plot to %s"%g.fout)
    gains=[]
    for col in range(256):
      if not col in fit_res[0] or fit_res[0][col]<0:
         self.logging.info("No baseline point for pixel (%d,%d)"%(col,col))
         continue
      Y=[0.0]
      for amp in range(1,len(params['amps'])):
        if not col in fit_res[0] or fit_res[0][col]<0.0:
          self.logging.info("No amplitude %d point for pixel (%d,%d)"%(amp,col,col))
          continue
        Y.append(-fit_res[amp][col]+fit_res[0][col])

      if len(Y)!=len(amp_meas): continue
      p0 = [Y[-1]/amp_meas[-1],]
      coeff, var_matrix = curve_fit(line, amp_meas, Y, p0=p0)
      gains.append(coeff[0])
    gmean=numpy.mean(gains)
    grms=numpy.std(gains)
    proc=100.0*grms/gmean
    self.logging.info("")
    electrons=20.0/gmean
    self.results['GAIN_ELECTRONS']="%.2f"%electrons
    self.results['GAIN_MEAN']="%.2f"%gmean
    self.results['GAIN_RMS']="%.3f"%grms
    self.results['GAIN_SPREAD']="%.1f"%proc
    self.logging.info("Gain mean %.3f +/- %.3f LSB/mV (~%.1f%%)  [~%.2f e- / TH LSB]"%(gmean,grms,proc,electrons))

    self.tpx.setDac(TPX3_VTP_COARSE,64)
    self.tpx.setSenseDac(TPX3_VTP_COARSE)
    coarse=self.tpx.get_adc(32)
    for amp in range(len(params['amps'])):
        thr=[]
        noise=[]
        for col in range(256):
            #if fit_res_noise[amp][col] > 2 and fit_res[amp][col] > 0 and mask[col][col]==0:
            if mask[col][col]==0:
                #print "%d\t%d\t%.3f\t%.3f\t"%(amp,col,fit_res[amp][col],fit_res_noise[amp][col])			
                thr.append(fit_res[amp][col])
                noise.append(fit_res_noise[amp][col])
        mean_thr=numpy.mean(thr)
        mean_thr_rms_e=numpy.std(thr)*electrons
        mean_noise_e=numpy.mean(noise)*electrons
        mean_noise_rms_e=numpy.std(noise)*electrons
        energy_resolution=pow(pow(mean_thr_rms_e,2)+pow(mean_noise_e,2),0.5)
        dac_code_ikrum=self.tpx.getDac(TPX3_IBIAS_IKRUM)
        self.tpx.setDac(TPX3_VTP_FINE,VTP_value[amp-1])       #re-mesure TP setting
        self.tpx.setSenseDac(TPX3_VTP_FINE)
        fine=self.tpx.get_adc(32)
        if amp==0:
            fine=coarse
        self.logging.info("Ik:%d\tPolarity %d\t Gain[%d]:%.1f\t(%3.2f)e-\t THR:%.1f\t +/-%.1fe-\t NOISE:%.1fe-\t +/-%.1fe-\t Resolution:%.1f (pix: %d,%d)"\
            %(dac_code_ikrum,polarity,amp,amp_meas[amp],1000.0*20*(fine-coarse), mean_thr,mean_thr_rms_e,mean_noise_e,mean_noise_rms_e,energy_resolution,len(thr),len(noise)))

#    aname=logdir[:-1]+".zip"
#    self.logging.info("Creating arhive %s"%aname)
#    self.zipdir(aname,logdir)

    return 


from tpx3_test import tpx3_test
import logging
import Gnuplot, Gnuplot.funcutils

class test03_dac_scan(tpx3_test):
  """DAC scan. 
Parameters:
  step - step size (integer), default 4 
  mpc  - measurements per code (integer), default 1
Example:
  test03_dac_scan(step=1 mpc=64) <- precise scan"""

  def _execute(self,**keywords):
    results=[]
    def_step=4
    meas_per_code=16
    logging.info("DAC scan settings:")
    if 'step' in keywords:
      def_step=int(keywords['step'])
    logging.info("Step size: %d"%def_step)

    if 'mpc' in keywords:
      meas_per_code=int(keywords['mpc'])
    logging.info("Measurements per code: %d"%meas_per_code)
    
   
    g = Gnuplot.Gnuplot(debug=0)
    g('set xlabel "Code / Max Code')
    g('set ylabel "Voltage [V]"')
    g("set xtic 1.0/4")
    g("set ytic 0.2")
    g("set yr[0:1.79]")
    g("set xr[0:1]")
    g("set grid")
    g("set key  horiz top samp 1")

    g._clear_queue()
    dac_name=  ['none','IB_PRE_ON',  'IB_PRE_OFF','VPRE_NCAS',     'IB_IKRUM',
                       'VFBK',       'VTHR_FIN',  'VTHR_COA',      'IB_DIS1_ON',
                       'IB_DIS1_OFF','IB_DIS2_ON','IB_DIS2_OFF',   'IB_PIXDAC',
                       'IB_TPBIN',   'IB_TPBOUT', 'VTP_COA',       'VTP_FINE',
                       'IB_CP_PLL','PLL_VCNTRL']
    fsr=  [(0,0),    (0.3,0.4),  (0.1,0.2),  (0.9,1.2),  (0.3,0.4),
                     (0.9,1.2),  (0.2,0.3),  (1.0,1.2),  (0.5,0.6),
                     (0.15,0.25),(0.25,0.35),(0.1,0.2),  (0.35,0.45),   
                     (0.3,0.4),  (0.4,0.5),  (0.9,1.2),  (0.9,1.2),
                     (0.4,0.5),  (0.0,1.25)]
    codes=[]
    tab=dict()
    retdict={}
    for dac_id in range(1,19):
      if dac_id==18:
        r,pll_conf=self.tpx.ctrl.getPllConfig(self.tpx.id)
        self.tpx.ctrl.setPllConfig(self.tpx.id,pll_conf&~(0x4))
      self.tpx.ctrl.setSenseDac(self.tpx.id,dac_id)
      name=dac_name[dac_id]
      max_code=self.tpx.ctrl.dacMax(dac_id)
      x=[]
      y=[]
      val=0
      self.tpx.ctrl.setDacsDflt(self.tpx.id)
      step=def_step
      if max_code<255:
        step=1
      steps=[]
      for code in range(0,max_code+1,step):
        steps.append(code)
      if steps[-1]!=max_code:steps.append(max_code)
      tab[dac_id]={}
      for code in steps:
        if not code in codes: codes.append(code)
        r=self.tpx.ctrl.setDac(self.tpx.id,dac_id,code)
        val=self.tpx.get_adc(meas_per_code)
        x.append(float(code)/max_code)
        y.append(val)
        tab[dac_id][code]=val
      fs=y[-1]-y[0]
      mono="YES"
      sigma=1.5/256
      for i in range(3,len(y)-3):
        if (y[i]>y[i-1]+sigma) and fs<0:
          mono="NO"
        if (y[i]<y[i-1]-sigma) and fs>0:
          mono="NO"
      
      fsstr=name+"_FS"
      retdict[fsstr]="%.3f"%abs(fs)
      monostr=name+"_MONO"
      retdict[monostr]=mono
      d = Gnuplot.Data(x, y,   title=name,    with_='lp pt 5 ps 0.2')
      g._add_to_queue([d])
      logging.info("  Measured full scale %.3f"%(abs(fs)))
      logging.info("  Acceptance range [%.3f,%3f]"%fsr[dac_id])

      self._assert_in_range(abs(fs),fsr[dac_id][0],fsr[dac_id][1],"DAC %s FS range %.3f V"%(name,abs(fs)))
      self._assert_true((mono=="YES"),"DAC %s monotonicity %s"%(name,mono))
      if dac_id==18:
        self.tpx.ctrl.setPllConfig(self.tpx.id,pll_conf)
    g("set terminal png size 450,370 small")
    fn='%s.png'%self.fname
    g("set output '%s'"%fn)
    g.refresh() 
    logging.info("Plot saved to %s"%fn)
    logging.debug("Details:")

    l="code | "
    for dac_id in range(1,19):
      l+="%-11s | "%dac_name[dac_id]
    logging.debug(l)
    for c in sorted(codes):
     l=" %3d | "%c
     for dac_id in range(1,19):
       if c in tab[dac_id]: l+= "%11.6f | "%tab[dac_id][c]
       else:l+="            | "
     logging.debug(l)

    #save data to file
    fn=self.fname+".dat"
    f=open(fn,"w")
    l="#code  "
    for dac_id in range(1,19):
      l+="%-11s "%dac_name[dac_id]
    f.write(l+"\n")
    for c in sorted(codes):
      l="%3d "%c
      for dac_id in range(1,19):
        if c in tab[dac_id]: l+= "%11.6f "%tab[dac_id][c]
        else:l+="            "
      f.write(l+"\n")
    f.close()
    logging.info("Data saved to %s"%fn)

    return retdict

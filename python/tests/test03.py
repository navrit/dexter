from tpx3_test import tpx3_test
import logging
import Gnuplot, Gnuplot.funcutils

class test03(tpx3_test):
  """DAC scan"""

  def _execute(self):
    results=[]
    def_step=1
    meas_per_code=64
    logging.info("DAC scan settings:")
    logging.info("default step size: %d"%def_step)
    logging.info("measurements per code: %d"%meas_per_code)
    
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
    fsr=  [(0,0),    (0.3,0.4),(0.1,0.2),  (0.9,1.2),  (0.3,0.4),
                     (0.9,1.2),(0.2,0.3),  (1.0,1.2),  (0.5,0.6),
                     (0.15,0.25),(0.25,0.35),(0.1,0.2),  (0.35,0.45),   
                     (0.3,0.4),(0.4,0.5),  (0.9,1.2),  (0.9,1.2),
                     (0.4,0.5),(0,1.2)]
    codes=[]
    tab=dict()
    retdict={}
    for dac_id in range(1,19):
      if dac_id==18:
        r,pll_conf=self.tpx.ctrl.getPllConfig(self.tpx.id)
        self.tpx.ctrl.setPllConfig(self.tpx.id,pll_conf&~(0x4))
        print "~~~~~~~~~ PLL %x -> %x"%(pll_conf,pll_conf&~(0x4))
      self.tpx.ctrl.setSenseDac(self.tpx.id,dac_id)
      name=dac_name[dac_id]
      max_code=self.tpx.ctrl.dacMax(dac_id)
      x=[]
      y=[]
      val=0
      self.tpx.ctrl.setDacsDflt(self.tpx.id)
#      f=open(self.dlogdir+"%s.txt"%name,"w")
      step=def_step
      if max_code<255:
        step=1
      steps=[]
      for code in range(0,max_code+1,step):
        steps.append(code)
      if steps[-1]!=max_code:steps.append(max_code)
#      logging.info("DAC %s scan details:"%name)
      tab[dac_id]={}
      for code in steps:
        if not code in codes: codes.append(code)
        r=self.tpx.ctrl.setDac(self.tpx.id,dac_id,code)
#        print r
        val=self.tpx.get_adc(meas_per_code)


#        print "%c%20s [%03d/%03d] %0.3f"%(13,name,code,max_code, val),
#        sys.stdout.flush()
        x.append(float(code)/max_code)
        y.append(val)
        tab[dac_id][code]=val
#        logging.info("  %03d %6.4f"%(code,val))
#        f.write("%03d %6.4f\n"%(code,val))
#      f.close()
      fs=y[-1]-y[0]
      mono="YES"
      sigma=0.002
      for i in range(3,len(y)-3):
        if (y[i]>y[i-1]+sigma) and fs<0:
          mono="NO"
#          print '\n',name,i,y[i-1],y[i]
        if (y[i]<y[i-1]-sigma) and fs>0:
          mono="NO"
#          print '\n',name,i,y[i-1],y[i]
      
#      print "FS: %.3f Mono: %s"%(abs(fs),mono)
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

    return retdict

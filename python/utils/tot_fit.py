#!/usr/bin/env python
import numpy
from scipy.optimize import curve_fit
import Gnuplot

def linf(x, *p):
    a, b = p
    return a*x+b
    
def gain_plot(fbase):
  amps=[]
  N=10

  qin=numpy.zeros( (N) )
  vout=numpy.zeros( (N) )

  for amp in range(N):
    data=numpy.loadtxt(fbase+"itot%02d.dat"%amp)
    amps.append(data)
    qin[amp]=1.0*(1.0+amp)
  print qin
  gain_map=numpy.zeros((256,256))
  offset_map=numpy.zeros((256,256))
  g = Gnuplot.Gnuplot(debug=0)
  g("set output '%s/diagonal.png'"%fbase)
  g("set terminal png")
  g("set xlabel 'Qin [k e-]")
  g("set ylabel 'TOT [LSB]")
  g("set grid")
  s="plot "
  data_lines=""
  
  for col in range(256):
    for row in range(256):
      for i in range(N):
         vout[i]=amps[i][col][row]/16

      p0 = [0.01,0]
      coeff, var_matrix = curve_fit(linf, qin, vout, p0=p0)
      gain_map[col][row]=coeff[0]
      offset_map[col][row]=coeff[1]
      
      if col==row and col%8==0:
        for i in range(N):
          data_lines+="%.3f %.3f\n"%(qin[i],float(vout[i]))
        data_lines+="e\n"
        if col>0: s+=","
        s+="'-'  w p pt 5 ps 0.5 lc %d t '' "%(col)
        s+=", %.4f*x+%.4f w l  lc %d t '' "%(coeff[0],coeff[1],col)
        
  pcmd=s+"\n"+data_lines
  g(pcmd)
  numpy.savetxt(fbase+"gain_map.dat",gain_map,fmt="%.5e")
  numpy.savetxt(fbase+"offset_map.dat",offset_map,fmt="%.5e")

def main():
   gain_plot("logs/F3_default_eq_bruteforce/test10_tot_matching/")
if __name__=="__main__":
  main()

#!/usr/bin/env python
import numpy
from scipy.optimize import curve_fit
import Gnuplot

def linf(x, *p):
    a, b = p
    return a*x+b
    
def gain_plot(fbase):
  bl=numpy.loadtxt(fbase+"bl.dat")
  amps=[]
  for amp in range(3):
    data=numpy.loadtxt(fbase+"amps%02d.dat"%amp)
    amps.append(data)
  qin=numpy.array( (0.0,1011.0,1570.0,2020.0) )
  gain_map=numpy.zeros((256,256))
  offset_map=numpy.zeros((256,256))
  g = Gnuplot.Gnuplot(debug=0)
  g("set output '%s/diagonal.png'"%fbase)
  g("set terminal png")
  g("set xlabel 'Qin [e-]")
  g("set ylabel 'Vout [LSB]")
  g("set grid")
  s="plot "
  data_lines=""
  
  for col in range(256):
#      row=col
    for row in range(256):

      b=bl[col][row]
      vout=numpy.array( (b,amps[0][col][row] ,amps[1][col][row] ,amps[2][col][row]) )
      vout = 320.0 - vout
      p0 = [0.01,0]
      coeff, var_matrix = curve_fit(linf, qin, vout, p0=p0)
      gain_map[col][row]=coeff[0]
      offset_map[col][row]=coeff[1]
      if col==row and col%8==0:
        for i in range(4):
          data_lines+="%.3f %.3f\n"%(qin[i],vout[i])
        data_lines+="e\n"
        if col>0: s+=","
        s+="'-'  w p pt 5 ps 0.5 lc %d t '' "%(col)
        s+=", %.4f*x+%.4f w l  lc %d t '' "%(coeff[0],coeff[1],col)
        
  pcmd=s+"\n"+data_lines
  g(pcmd)
  numpy.savetxt(fbase+"gain_map.dat",gain_map,fmt="%.5e")
  numpy.savetxt(fbase+"offset_map.dat",offset_map,fmt="%.5e")

def main():
   gain_plot("logs/F3_default_eq_bruteforce/test09_gain_map/")
if __name__=="__main__":
  main()

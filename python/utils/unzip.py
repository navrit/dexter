#!/usr/bin/env python
import zipfile
import os.path
import glob
import os.path
import Gnuplot
pixel=(0,0)
g=Gnuplot.Gnuplot(debug=True)
g("set terminal png")
g("set output 'pixel.png'")
g("set grid")
g("PI=3.14159")

pcmd='plot '
fit=""
fn="%03d_%03d.dat"%pixel
print fn
for dac in range(16):
  zipname="0x%0X.zip"%dac
  if not os.path.isfile(zipname):
     break
  zfile = zipfile.ZipFile(zipname)
  for name in zfile.namelist():
    (dirname, filename) = os.path.split(name)
    if filename==fn:
      print "Decompressing " + filename + " on " + dirname
#  if not os.path.exists(dirname):
#    os.mkdir(dirname)
      fout="dac0x%X_"%dac+filename
      print fout
      fd = open(fout,"w")
      fd.write(zfile.read(name))
      fd.close()
  if dac>0:
      pcmd+=','
  pcmd+=' "%s" w p lc %d pt 5 ps 0.5 t ""'%(fout,dac)
  g("s%d=10"%(dac))
  g("m%d=%.0f"%(dac,220+dac*10))
  g("a%d=600"%(dac))
  g("f%d(x)=a%d*x+b%d"%(dac,dac,dac))
  g("g%d(x) = a%d/(2*PI*s%d**2)**0.5*exp(-(x-m%d)**2/(2*s%d**2))"%(dac,dac,dac,dac,dac))
  g("fit g%d(x)   '%s'  via s%d, m%d, a%d"%(dac,fout,dac,dac,dac))
  pcmd+=',g%d(x) w l lc %d t ""'%(dac,dac)
  print pcmd
g(pcmd)


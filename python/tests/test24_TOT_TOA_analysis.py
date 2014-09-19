from tpx3_test import *
import os
import random
import time
import logging
from SpidrTpx3_engine import *
import numpy as np
from scipy.optimize import curve_fit
import matplotlib.pyplot as plt
from scipy.special import erf
from dac_defaults import dac_defaults
import zipfile
import shutil
import sys

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


def tot_fit(x,a,b):#,c,t):
    return a*x+b#-(c/(x-t))

def func(x, a, b, c):
    return a*np.exp(-b*x) + c


class tot_toa_analysis(tpx3_test):
  """TOT and TOA Analysis"""

  def _execute(self,**keywords):
    x=10
    y=10
    energy=[]
    TOT=[]
    f=open(self.fname+"/TOT.dat","w")
    for TP in range(500,8000,250):
      tot=np.loadtxt('TOT_TOA_calib/TOT_%d.map'%TP, dtype=float)
      resolution=np.loadtxt('TOT_TOA_calib/Resolution_TOT_%d.map'%TP, dtype=float)
      toa=np.loadtxt('TOT_TOA_calib/TOA_%d.map'%TP, dtype=float)
      std_toa=np.loadtxt('TOT_TOA_calib/Stdev_TOA_%d.map'%TP, dtype=float)

      line="%d "%TP
      for x in range(20):
        line+="%.2f "%tot[x][x]
      print line
      f.write(line)
      f.write("\n")  
      energy.append(TP)
      TOT.append(tot[x][y])

    f.close()

#      print"%d %.2f %.2f %.2f %.2f"%(TP,tot[x][y],resolution[x][y],toa[x][y],std_toa[x][y])
    
    if 0: #tot fitting
      try:
#         p0 = [1, 1, 6.,999]
         print energy,TOT
#         coeff,d1 = curve_fit(tot_fit, energy, TOT)
         p0 = [50]#, 1000, 150.]
         coeff, var_matrix = curve_fit(line, energy, tot, p0=p0)
         print coeff, var_matrix
      except:
       pass


    x = np.linspace(0,4,50)
    y = func(x, 2.5, 1.3, 0.5)
    yn = y + 0.2*np.random.normal(size=len(x))

    popt, pcov = curve_fit(tot_fit, energy,TOT)

    print np.size(energy),np.size(TOT)
    print popt




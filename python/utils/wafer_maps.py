#!/usr/bin/env python
import sys
import glob
import numpy as np
import svgwrite
import os 
import math

def load_wafer(bd,nr):
  WAFER=nr
  basedir=bd
  dies=[]
  keys=[]
  names=[]
  data={}
  loaded=0
  for wdir in sorted(glob.glob(basedir+"/W%d_*"%WAFER)):
    die={}
    n=wdir.split("/")[-1]
    nn=n.split("_")

    if nn[-1]=='efuse' : continue
    n=nn[1]
    names.append(n)
    for test in glob.glob(wdir+"/*/result.dat"):
      f=open(test,"r")
      for l in f.readlines():
        k,v=l.split()
        die[k]=v
        if not k in data: data[k]={}
        data[k][n]={'val':v,'col':'#EEEEEE'}
      f.close()
      loaded+=1
  print "Loaded %d wafers"%loaded
  return data

def check_floats(data):
  try :
    for k in data:
      v=float(data[k]['val'])
      data[k]['val']=v
    return True
  except:
    return False


class ColorWaferMap:
  def __init__(self,name,data,type="symetric",ibmname=""):
    self.name=name
    self.data=data
    self.type=type
    self.legend=None
    self._info=""
    self.frm="%f"
    self.ibmname=ibmname
    self.create_legend()

  def save(self,fname,font=5,wname="",meas=""):
    DIAM=200.00
    LEG=20
    MARGIN=20
    PX,PY=DIAM+MARGIN,DIAM+MARGIN+LEG
    dwg = svgwrite.Drawing(fname+".svg", profile='tiny', size=(PX,PY))
    X2LETER=['M','L','K','J','I','H','G','F','E','D','C','B','A']
    DIE_W=14.2
    DIE_H=16.69
    j=0
    _x0=MARGIN/2.0+DIAM/2
    _y0=MARGIN/2.0+DIAM/2

    dwg.add(svgwrite.shapes.Rect(insert=(0, 0), size=(PX,PY), fill="#FFFFFF",stroke="#ffffff", stroke_width=0.3))
    dwg.add(svgwrite.shapes.Circle(center=(_x0, _y0), r=DIAM/2+0.1,fill="#CCCCCC",stroke="black", stroke_width=0.2))
    dwg.add(svgwrite.shapes.Circle(center=(_x0, _y0), r=DIAM/2-0.6,fill="#CCCCCC",stroke="black", stroke_width=0.2))
    dwg.add(dwg.line((_x0, _y0-DIAM/2+5), (_x0+3, _y0-DIAM/2),stroke="black", stroke_width=0.2))
    dwg.add(dwg.line((_x0, _y0-DIAM/2+5), (_x0-3, _y0-DIAM/2),stroke="black", stroke_width=0.2))
    dwg.add(dwg.line((_x0, _y0-DIAM/2+4), (_x0+2.2, _y0-DIAM/2),stroke="black", stroke_width=0.2))
    dwg.add(dwg.line((_x0, _y0-DIAM/2+4), (_x0-2.2, _y0-DIAM/2),stroke="black", stroke_width=0.2))

    for YM in range(1,11+1):
      Y=12-YM
      if Y in (1,) :    MINX,MAXX=6,8
      if Y in (11,) :   MINX,MAXX=5,9
      if Y in (2,) :    MINX,MAXX=4,10
      if Y in (3,10) :  MINX,MAXX=3,11
      if Y in (4,8,9) : MINX,MAXX=2,12
      if Y in (5,6,7) : MINX,MAXX=1,13
  
      for XM in range(MINX,MAXX+1):
        X=MAXX-XM+MINX
        j+=1
        name="%s%d"%(X2LETER[X-1],Y)
        x0=_x0+((14-X)-7-0.5)*DIE_W
        y0=_y0+((12-Y)-6-0.5)*DIE_H
        val=""
        col='#EEEEEE'
        if name in self.data:
          val=self.data[name]['val']
          col=self.data[name]['col']

        dwg.add(svgwrite.shapes.Rect(insert=(x0, y0), size=(DIE_W,DIE_H), fill=col,stroke="#CCCCCC", stroke_width=0.3))
        dwg.add(dwg.text(val, x=[x0+DIE_W/2], y=[y0+0.4*DIE_H],text_anchor="middle",font_size = "%dpx"%font))
        dwg.add(dwg.text(name, x=[x0+DIE_W/2], y=[y0+0.85*DIE_H],text_anchor="middle",font_size = "3px"))

    if self.legend:
      N=len(self.legend)
      dN=DIAM/(N+2)

      for i,e in enumerate(self.legend):
        x0=MARGIN/2 + (1+i)*dN
        y0=MARGIN/2 + DIAM+LEG/4
        col=e['col']
        txt=e['txt']
        cnt="[%d]"%e['cnt']
        dwg.add(svgwrite.shapes.Rect(insert=(x0, y0), size=(dN,12), fill=col,stroke="black", stroke_width=0.3))
        dwg.add(dwg.text(txt, x=[x0+0.5*dN], y=[y0+LEG/3],text_anchor="middle",font_size = "5px"))
        dwg.add(dwg.text(cnt, x=[x0+0.5*dN], y=[y0+2*LEG/4],text_anchor="middle",font_size = "4px"))


    dwg.add(dwg.text("CERN:"+wname, x=[MARGIN/2], y=[MARGIN/2],text_anchor="start",font_size = "5px"))
    dwg.add(dwg.text("IBM:"+self.ibmname, x=[MARGIN/2], y=[MARGIN/2 +7],text_anchor="start",font_size = "5px"))

    dwg.add(dwg.text(meas, x=[PX-MARGIN/2], y=[MARGIN/2],text_anchor="end",font_size = "5px"))
    for i,l in enumerate(self._info):
      x=MARGIN/2
      y=MARGIN/2+DIAM-(len(self._info)-i)*5
      dwg.add(dwg.text(l, x=[x], y=[y],text_anchor="start",font_size = "5px"))

    dwg.save()
    os.system("inkscape --export-dpi=300 -f %s.svg -e %s.png > inkscape.log && rm -rf %s.svg "%(fname,fname,fname))




  def val2col(self,val):
    col=""

    if math.isnan(val) : col=self.colors[-1]
    elif self.type=="symetric":
      i=0
      d= (val-self.mean)/self.std
      d=int(d+3.5)
      d=max ( (d,0 ) )
      d=min ( (d,len(self.colors)-1 ) )
      col=self.colors[d]

    else :
      i=0
      g=(self.max-self.min)
      if g==0:
        col=self.colors[0]
      else:
        d= (val-self.min)/g * (len(self.colors))
        d=int(d)
        d=min( (d, len(self.colors)-1 ) )
        col=self.colors[d]
    return col

  def create_legend(self):
    if self.type=="category":
      self.colors=["#6dff84","#009b1a","#ddff6d","#ffac6d","#ffac6d","#ff3636"]
      l=[]
      for i,c in enumerate(self.colors):
        txt=chr( ord('A') + i)
        x={'col':c,'txt':txt,'cnt':0}
        l.append(x)
    elif self.type=="symetric":
      self.colors=["#716dff","#6dcbff","#36ffe5","#6dff84","#ddff6d","#ffac6d","#ff3636"]
      l=[]
      for i,c in enumerate(self.colors):
        i-=3
        txt="%d "%i+u"\u03C3"
        if i==-3: txt="< "+txt
        if i==3: txt="> "+txt
        x={'col':c,'txt':txt,'cnt':0}
        l.append(x)
    else:
      self.colors=["#6dff84","#009b1a","#ddff6d","#ffac6d","#ff3636"]
      l=[]
      for i,c in enumerate(self.colors):
        txt="%d - %d %%"%(i*20,(i+1)*20)
        x={'col':c,'txt':txt,'cnt':0}
        l.append(x)
    self.legend=l




  def process(self,ignore_zeros=False):
    if self.type=='category':
      for die in self.data:
        cindex=ord(self.data[die]['val'][0])-ord('A')
        cindex=min( (cindex, len(self.colors)-1) )
        col=self.colors[cindex]
        self.data[die]['col']=col
        self.legend[cindex]['cnt']+=1
    elif self.type=='symetric':
      vals=[]
      for die in self.data:
        v=d[die]['val']
        if math.isnan(v) : continue
        if ignore_zeros and abs(v)<1e-3: continue
        vals.append(v)
      self.mean=np.mean(vals)
      self.std=np.std(vals)
      if self.std==0:
        return
      mmax=max(vals)
      self.frm="%.3f"
      if mmax>1000:
        self.frm="%.0f"
      elif mmax>100:
        self.frm="%.1f"
      elif mmax>10:
        self.frm="%.2f"
      for die in d:
        col=self.val2col(d[die]['val'])
        d[die]['col']=col
        d[die]['val']=self.frm%d[die]['val']
        for e in self.legend:
          if e['col']==col:e['cnt']+=1
      self._info=["Mean: "+self.frm%self.mean,"RMS: "+self.frm%self.std]

    elif self.type=='linear':
      vals=[]
      for die in self.data:
        v=d[die]['val']
        if math.isnan(v) : continue
        if ignore_zeros and abs(v)<1e-3: continue
        vals.append(v)
      self.max=np.max(vals)
      self.min=np.min(vals)
      self.frm="%.3f"
      if self.max>1000:
        self.frm="%.0f"
      elif self.max>100:
        self.frm="%.1f"
      elif self.max>10:
        self.frm="%.2f"
      for die in d:
        col=self.val2col(d[die]['val'])
        d[die]['col']=col
        d[die]['val']=self.frm%d[die]['val']
        for e in self.legend:
          if e['col']==col:e['cnt']+=1
      self._info=["Max: "+self.frm%self.max,"Min: "+self.frm%self.min]


  def info(self):
    if self.type=='category':
      print "%-30s"%(self.name)
    elif self.type=='linear':
      print "%-30s  [%s,%s]"%(self.name,self.frm%self.min,self.frm%self.max)
    else:
      print "%-30s %s +/- %s"%(self.name,self.frm%self.mean,self.frm%self.std)

if __name__=="__main__":

  WNAME, bd, IBMNAME =1,'/home/skulis/logs/Wafer_01/retest_2014_03_03',"AAPVZ5H"
  WNAME, bd, IBMNAME =2,'/home/skulis/logs/Wafer_02/logs',"ATPW1KH"
  WNAME, bd, IBMNAME =3,'/home/skulis/logs/Wafer_03/logs',"AMPW1RH"
  WNAME, bd, IBMNAME =4,'/home/skulis/logs/Wafer_04/logs',"AJPW1UH"
  WNAME, bd, IBMNAME =5,'/home/skulis/logs/Wafer_05/logs',"A7PVYRH"
  WNAME, bd, IBMNAME =7,'../logs/',"xxx"

  if len(sys.argv)<4:
    print sys.argv[0],"wafer_no ibm_name path [maps]"
  maps=[]
  if len(sys.argv)>4:
    maps=sys.argv[4:]

  WNAME=int(sys.argv[1])
  IBMNAME=sys.argv[2]
  bd=sys.argv[3]
     
  data=load_wafer(bd,WNAME)
  
  wafer_dir=bd+"/W%d_MAPS"%WNAME
  if not os.path.exists(wafer_dir):
      os.makedirs(wafer_dir)
  if len(maps)==0:
    maps=sorted(data.keys())

  for var in  maps:#[3:5]: 
    if var in ('ERROR'): continue
    if var.find("REG")>=0: continue
    if var.find("MONO")>=0: continue
    font=4
    d=data[var]
    if var in ('CATEGORY',):
      t="category"
      font=6
    elif var.find("BAD_")>=0 or var.find("PIXELS")>=0 or var.find("DISTANT")>=0  or var.find("KID")>=0 or \
        var.find("_HIGHER")>=0 or var.find("_LOWER")>=0 or (var in ("TIMEOUTS",)) :

      t="linear"
      if not check_floats(d): continue
    else:
      t="symetric"
      if not check_floats(d): continue
    print var,t
    cwm=ColorWaferMap(name=var,data=d,type=t,ibmname=IBMNAME)
    cwm.process()

    fname="%s/w%03d_%s"%(wafer_dir,WNAME,var.lower())
    cwm.save(fname=fname,font=font,wname="W%d"%WNAME,meas=var)
    cwm.info()


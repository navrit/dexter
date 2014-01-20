#!/usr/bin/env python
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import matplotlib.mlab as mlab
import matplotlib.pyplot as plt
import wx
from math import pow,sqrt,sin,cos
import matplotlib.colors as colors
import matplotlib.cm as cm
import matplotlib
from wx.lib.embeddedimage import PyEmbeddedImage
import os
import random
import wx.lib.agw.buttonpanel as bp
import icons

class ProbeStation:
    def __init__(self,address=22):
      self.address=address
    def goto(self):
      pass
      

class WaferMap(object):
    def __init__(self, fname=""):
      if fname!="":
        self.load(fname)
      
    def load(self,fname):
      self.fname=fname
      data=np.loadtxt(fname)
      X,Y=data.shape
      if X!=Y:
        raise OPException("File doesn't contain a square matrix")
      self.data=data
      self.mmin=np.amin(self.data)
      self.mmax=np.amax(self.data)
      
      mean=self.data.mean()
      std=self.data.std()

      self.nice_min=max(self.mmin,mean-3*std)
      self.nice_max=min(self.mmax,mean+3*std)
      self.mmin=self.nice_min
      self.mmax=self.nice_max
      
      self.nice_inc=max((self.mmax-self.mmin)/100,0.001)
#      print self.nice_min, self.nice_max,self.nice_inc
      self.N=X
      self.process()

from xml.dom import minidom

class MapPanel(wx.Panel):
    def __init__(self, parent,fname=None):
        wx.Panel.__init__(self, parent, size=(512,512),style= wx.NO_BORDER)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.grid=0
        self.dies=[]
        self.load(fname)
        self.diameter=100.0
#        self.Refresh()
        self.Bind(wx.EVT_SIZE, self.OnSize)

        self.Bind(wx.EVT_RIGHT_DOWN, self.onContext)
        self.chuck_pos=(-90,-90)
        self.scale=0
        self.x0=0
        self.y0=0
        self.rclic_die=None
        self.dieno=0
    def goToDieNo(self,no):
          d=self.dies[self.dieno]
          x,y=(d['x']+d['w']/4,d['y']+d['h']*3/4)
          self.chuck_pos=(x,y)
    
    def goFirst(self):
        self.dieno=0

        self.goToDieNo(self.dieno)
        statuses=['A','B','C','D','E','F']
        self.dies[self.dieno]["status"]=statuses[random.randint(0,len(statuses)-1)]
        self.Refresh()
        
    def stepNext(self):
        while self.dieno<len(self.dies):
          self.dieno+=1
          if self.dieno<len(self.dies):
            die=self.dies[self.dieno]
            statuses=['A','B','C','D','E','F']
            self.Refresh()
            if 'skip' in die and die['skip']: continue
            self.goToDieNo(self.dieno)
            die["status"]=statuses[random.randint(0,len(statuses)-1)]
            return True
          else:
            return False
     
         
    def onContext( self, event ):
        """
        Create and show a Context Menu
        """
        x,y= event.GetPosition()
        x=(x-self.x0)/self.scale
        y=(y-self.y0)/self.scale
        print x,y
        die=None
        for d in self.dies:
          if x>d['x'] and x<d['x']+d['w'] and \
             y>d['y'] and y<d['y']+d['h']:
             die=d
             print die
        # only do this part the first time so the events are only bound once 
        if not hasattr(self, "popupID1"):
            self.popup_name = wx.NewId()
            self.popup_goto = wx.NewId()
            self.popup_test = wx.NewId()
            self.popup_skip = wx.NewId()
            self.popup_logs = wx.NewId()
            
            self.Bind(wx.EVT_MENU, self.RclicGoTo, id=self.popup_goto)
            self.Bind(wx.EVT_MENU, self.SkipDie, id=self.popup_skip)
#            self.Bind(wx.EVT_MENU, self.onExit, id=self.itemThreeId)

        if die!=None:
          # build the menu
          menu = wx.Menu()
          print die['name']
          self.rclic_die=die
          itemOne = menu.Append(self.popup_name, die['name'])
          menu.Enable(self.popup_name, 0) 
          menu.AppendSeparator()
          itemTwo = menu.Append(self.popup_goto, "go to")
          itemTwo = menu.Append(self.popup_skip, "skip")
          itemThree = menu.Append(self.popup_test, "test")

          logs_menu = wx.Menu()
          logs_menu.Append(wx.ID_ANY, '1) 2014/01/22 23:22')
          logs_menu.Append(wx.ID_ANY, '2) 2014/01/23 13:22')
          logs_menu.Append(wx.ID_ANY, '1) 2014/01/24 23:22')
          menu.AppendMenu(self.popup_logs, '&Logs', logs_menu)
#          menu.Enable(self.popup_logs, 0) 
          
          # show the popup menu
          self.PopupMenu(menu)
          menu.Destroy()
    def SkipDie(self,event):
        self.rclic_die['skip']=True
        self.Refresh()

    
    def RclicGoTo(self,event):
        print self.rclic_die
        self.chuck_pos= (self.rclic_die['x']+self.rclic_die['w']/2,self.rclic_die['y']+self.rclic_die['h']/2)
        self.Refresh()
    def OnSize(self, event):
        self.Refresh()
        
    def load(self,fname):
      xmldoc = minidom.parse(fname)
      waferlist = xmldoc.getElementsByTagName('wafer') 
      if len(waferlist)!=1:
        print "No wafer defined."
        return
      if 'diameter' in waferlist[0].attributes.keys():
        self.diameter=waferlist[0].attributes['diameter']
      else:
        print "No diameter defined!"
        return

      dielist = xmldoc.getElementsByTagName('die') 
      if len(dielist)<1:
        print "No dies found."
        return
      print "Dies found : %d"%len(dielist)
      self.dies=[]
      for s in dielist :
        die={'name':s.attributes['name'].value,
             'x':float(s.attributes['x'].value),
             'y':float(s.attributes['y'].value),
             'w':float(s.attributes['w'].value),
             'h':float(s.attributes['h'].value),
             'status':'?'#statuses[random.randint(0,len(statuses)-1)
             }
#        print die
        self.dies.append(die)
      print "Loaded dies",len(self.dies)

      notchlist = xmldoc.getElementsByTagName('notch') 
      if len(notchlist)!=1:
        print "No notch found."
        return
      self.notch={'angle':float(notchlist[0].attributes['angle'].value),
                  'len':float(notchlist[0].attributes['length'].value)}
      print self.notch
    
    def die2screen(self,pos):
        x,y=pos
        x=self.x0+x*self.scale
        y=self.y0+y*self.scale
        return (x,y)
        
    def OnPaint(self, evt):
        dc = wx.PaintDC(self)
#        dc.SetBackground(wx.Brush((250,250,250))) 
        MX,MY=dc.GetSizeTuple()
        dc.GradientFillLinear( (0,0, MX,MY), (160,255,255), (240,255,255), wx.NORTH)
        
#        gc = wx.GraphicsContext.Create(dc)
        #gc.DrawBitmap(self.bmp, 00, 00, self.bmp.Width,self.bmp.Height)
#        dc.DrawRectangle(self.x0-1, self.y0+cm_offset-1,self.cm_bmp.GetSize()[0]+2,self.cm_bmp.GetSize()[1]+2) 
#        dc.DrawBitmap(self.cm_bmp, self.x0, self.y0+cm_offset, False)
        wafer_brush = wx.Brush(wx.Colour(230,230,230))
        dc.SetBrush(wafer_brush)
#        dc.Clear()
        dc.SetPen(wx.Pen(wx.Colour(100,100,100), 1, wx.SOLID))
        dc.BeginDrawing()
        self.x0=MX/2
        self.y0=MY/2
        self.scale=0.95*min ( (MX,MY) )/2.0/self.diameter

#        print MX,MY,x0,y0
#        print (x0,y0,self.diameter*SCALE)
        gc = wx.GraphicsContext.Create(dc)
        gc.SetBrush( gc.CreateRadialGradientBrush(
                       self.x0, self.y0,
                        self.x0+self.diameter*self.scale, self.y0 + self.diameter*self.scale,
                        self.diameter * self.scale*3,
                        wx.Colour(200,200,220), wx.Colour(240,240,240) ) )
        def drawCircle( x, y, r ):
            gc.DrawEllipse( x - r, y - r, r * 2, r * 2 )
        drawCircle (self.x0,self.y0,self.diameter*self.scale)

        dc.SetBrush( wx.Brush(wx.Colour(230,230,230),wx.TRANSPARENT) )


        dc.DrawCircle (self.x0,self.y0,self.diameter*self.scale)
        dc.DrawCircle (self.x0,self.y0,self.diameter*self.scale*0.995)

        wbrush = {'?':wx.Brush(wx.Colour(220,220,220))}
        
        for c in range(6):
          green=255-c*40
          red=255-(5-c)*30
          wbrush[chr(ord('A')+c)]=wx.Brush(wx.Colour(red,green,150))

        for die in self.dies:
          dc.SetPen(wx.Pen(wx.Colour(100,100,100), 1, wx.SOLID))
#          dc.SetBrush(wbrush[die['status']])
          colour=wbrush[die['status']].GetColour()
          cg=0.8
          colour2=(colour[0]*cg,colour[1]*cg,colour[2]*cg,0)
          dc.GradientFillLinear((self.x0+die['x']*self.scale+1, self.y0+die['y']*self.scale+1, 
                           die['w']*self.scale-2, die['h']*self.scale-2), colour, colour2, wx.NORTH)
#          dc.SetBrush(wbrush[die['status']])
          
          dc.DrawRectangle(self.x0+die['x']*self.scale+1, self.y0+die['y']*self.scale+1, 
                           die['w']*self.scale-2, die['h']*self.scale-2)
          
          dc.SetFont(wx.Font(8, wx.FONTFAMILY_DEFAULT, style=0,weight=wx.FONTWEIGHT_BOLD) )
          name_size= dc.GetFullTextExtent(die['name'])
          tx=self.x0+die['x']*self.scale-name_size[0]/2+(die['w']*self.scale-2)/2
          ty=self.y0+die['y']*self.scale
          dc.DrawText( die['name'], tx,ty) 


          dc.SetFont(wx.Font(16, wx.FONTFAMILY_DEFAULT, style=0,weight=wx.FONTWEIGHT_BOLD) )
          status_size= dc.GetFullTextExtent(die['status'])
          tx=self.x0+die['x']*self.scale-status_size[0]/2+(die['w']*self.scale-2)/2
          ty=self.y0+die['y']*self.scale+name_size[1]

          dc.DrawText( die['status'], tx,ty) 

          if 'skip' in die and die['skip'] : 
            dc.SetPen(wx.Pen(wx.Colour(255,0,0), 2, wx.SOLID))
            x0,y0=die['x'],die['y']
            x1,y1=die['x']+die['w'],die['y']+die['h']
            x0,y0=self.die2screen( (x0,y0) )
            x1,y1=self.die2screen( (x1,y1) )
            
            dc.DrawLine(x0+2,y0+2,x1-2,y1-2)
            dc.DrawLine(x0+2,y1-2,x1-2,y0+2)


        #draw chuch position
        cp=self.die2screen(self.chuck_pos)
        wpen = wx.Pen(wx.Colour(0,0,200), 3, wx.SOLID)
        dc.SetPen(wpen)
        dc.DrawLine(cp[0],cp[1],cp[0]-15,cp[1]+15)
        dc.DrawLine(cp[0],cp[1],cp[0]-5,cp[1])
        dc.DrawLine(cp[0],cp[1],cp[0],cp[1]+5)
          
        #draw notch
        wpen = wx.Pen(wx.Colour(50,50,50), 3, wx.SOLID)
        dc.SetPen(wpen)

        x0=0.0
        y0=-(self.diameter-self.notch['len'])
        a=self.notch['angle']/180*3.1415
        x1_=-self.notch['len']/2
        y1_=-(self.diameter)
        x0,y0=self.die2screen((x0,y0))
        x1,y1=self.die2screen((x1_,y1_))
        x2_=self.notch['len']/2
        y2_=-(self.diameter)
        x2,y2=self.die2screen((x2_,y2_))

        dc.DrawLine(x0,y0,x1,y1)
        dc.DrawLine(x0,y0,x2,y2)
        dc.DrawLine(x1,y1,x2,y2)

        
#        self.Update()

#          print (x0+die['x'], y0+die['y'], die['w'], die['h'])
#        dc.DrawLine(self.x0+512+rs[0]+avrs,self.y0+0,  self.x0+512+rs[0]+avrs,    self.y0+512+1)
#        dc.DrawLine(self.x0+512+avrs,      self.y0,    self.x0+512+rs[0]+avrs,    self.y0)
#            dc.DrawLine(self.x0+512+avrs,      self.y0+512,self.x0+512+rs[0]+avrs,    self.y0+512)
#            dc.DrawLine(self.x0+512+avrs,      self.y0+0,  self.x0+512+avrs,          self.y0+512+1)

#            cs=512,10
#            dc.DrawLine(self.x0,    self.y0+512+cs[1]+avrs,self.x0+512,    self.y0+512+cs[1]+avrs)
#            dc.DrawLine(self.x0,    self.y0+512+avrs,      self.x0+512,    self.y0+512+avrs)
#            dc.DrawLine(self.x0,    self.y0+512 +avrs,     self.x0,        self.y0+512+cs[1]+avrs)
#            dc.DrawLine(self.x0+512,self.y0+512 +avrs,     self.x0+512,    self.y0+512+cs[1]+avrs)



#        for i in range(0,8):
#          avr_offset=0
#          if self.show_avr: avr_offset=11+3
#          lbl="%d"%(self.p0[1]+256*i/4/self.PPP)
#          w,h = dc.GetTextExtent(lbl)
#          dc.DrawText( lbl, self.x0-2-w,                   self.y0+(i)*64+self.PPP/2-h/2) 
#          dc.DrawText( lbl, self.x0+512+2+avr_offset,                 self.y0+(i)*64+self.PPP/2-h/2) 

#          lbl="%d"%(self.p0[0]+256*i/4/self.PPP)
#          w,h = dc.GetTextExtent(lbl)
#          dc.DrawText( lbl, self.x0+(i)*64+self.PPP/2-w/2, self.y0-2-h) 

        dc.EndDrawing()

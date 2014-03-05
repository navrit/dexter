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
import xml.etree.cElementTree as ET
import time
import ast
from xml.dom import minidom


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


class LogViewer(wx.Dialog):
    def __init__(self, parent, fname):
        wx.Dialog.__init__(self, parent, title=fname, size=(400,500),style=wx.DEFAULT_DIALOG_STYLE|wx.OK|wx.RESIZE_BORDER)
        self.txt = wx.TextCtrl(self, style=wx.TE_MULTILINE|wx.TE_READONLY)
        f=open(fname,'r')
        self.txt.SetValue(f.read())
        f.close()
        self.SetMinSize( (300,100))
        self.Show(True)
        
        
class MapPanel(wx.Panel):
    def __init__(self, parent,fname=None):
        wx.Panel.__init__(self, parent, size=(512,512),style= wx.NO_BORDER)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.grid=0
        self.dies=[]
        self.diameter=100.0
        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.Bind(wx.EVT_RIGHT_DOWN, self.onContext)
        self.chuck_pos=(-90,-90)
        self.scale=1.0
        self.x0=0
        self.y0=0
        self.rclic_die=None
        self.dieno=0
        self.home=""
        
        self.notch={'angle':0.0,  'length':5.0}
        self.name=None
        self.number=None
        if fname:
          self.load(fname)

    def goToDieNo(self,dieno):
          d=self.dies[dieno]
          x,y=(d['x']+d['w']/4,d['y']+d['h']*3/4)
          self.chuck_pos=(x,y)
          self.Refresh()

    def CreateNewFromTemplate(self, wafer_name, wafer_number, template, directory=""):

        import shutil
        wname     = "%03d_%s"%(int(wafer_number),wafer_name)
        wafer_dir = os.path.join(directory,wname)
        if not os.path.exists(wafer_dir):
          os.makedirs(wafer_dir)  
        wafer_file=os.path.join(wafer_dir,wname+'.xml')
        shutil.copyfile(template,wafer_file)
        #self.OpenWafer(wafer_file)
        self.load(wafer_file)
        self.Refresh()
        return None

    def onContext( self, event ):
        """
        Create and show a Context Menu
        """
        x,y= event.GetPosition()
        x=(x-self.x0)/self.scale
        y=(y-self.y0)/self.scale
#        print x,y
        die=None
        for d in self.dies:
          if x>d['x'] and x<d['x']+d['w'] and \
             y>d['y'] and y<d['y']+d['h']:
             die=d
#             print die
        # only do this part the first time so the events are only bound once 
        if not hasattr(self, "popup_name"):
            self.popup_name = wx.NewId()
            self.popup_goto = wx.NewId()
            self.popup_test = wx.NewId()
            self.popup_skip = wx.NewId()
            self.popup_sethome = wx.NewId()
            self.Bind(wx.EVT_MENU, self.OnGoTo, id=self.popup_goto)
            self.Bind(wx.EVT_MENU, self.OnSkipDie, id=self.popup_skip)
            self.Bind(wx.EVT_MENU, self.OnTestDie, id=self.popup_test)
            self.Bind(wx.EVT_MENU, self.OnSetHome, id=self.popup_sethome)


            self.popup_logs = wx.NewId()
            self.popup_logs_entries=[]
            for i in range(32):
              self.popup_logs_entries.append(wx.NewId())
              self.Bind(wx.EVT_MENU, self.OnLogEntry, id=self.popup_logs_entries[-1])
            
#            self.Bind(wx.EVT_MENU, self.onExit, id=self.itemThreeId)

        if die!=None:
          # build the menu
          menu = wx.Menu()
          self.rclic_die=die
          itemOne = menu.Append(self.popup_name, die['name'])
          menu.Enable(self.popup_name, 0) 
          menu.AppendSeparator()
          menu.Append(self.popup_goto, "go to")
          
          if die['skip']:
            menu.Append(self.popup_skip, "Don't skip")
          else:
            menu.Append(self.popup_skip, "skip")

          menu.Append(self.popup_sethome, "set as home")
          
          itemThree = menu.Append(self.popup_test, "test")

          logs_menu = wx.Menu()
          if 'tests' in die:
            for i,test in enumerate(die['tests']):
              logs_menu.Append(self.popup_logs_entries[i], '%d) %s Result:%s'%(test['id'],test['date'],test['result'] ))
              
          menu.AppendMenu(self.popup_logs, '&Logs', logs_menu)
#          menu.Enable(self.popup_logs, 0) 
          
          # show the popup menu
          self.PopupMenu(menu)
          menu.Destroy()

    def _get_die_by_name(self,die_name):
        for die in self.dies:
          if die['name']==die_name:
             return die
        return None

    def _get_die_id_by_name(self,die_name):
        for i,die in enumerate(self.dies):
          if die['name']==die_name:
             return i
        return None

    def GoHome(self):
        hid=self._get_die_id_by_name(self.home)
        self.goToDieNo(hid)
        self.Refresh()


    def _test_die(self,die_name):
        print "-> test",die_name
        did=self._get_die_id_by_name(die_name)
        self.goToDieNo(did)
        
        die=self._get_die_by_name(die_name)
        if not 'tests' in die:
          die['tests']=[]
        max_test=-1
        for test in die['tests']:
          max_test= max ( (max_test,test['id']) )
        test_id=max_test+1
        test_fname="logs/dddd"
        dir_name=os.path.dirname(self.fname)
        dir_name=os.path.join(dir_name,die['name'])
        if not os.path.exists(dir_name):
          os.makedirs(dir_name)
        test_result='A'
        
        statuses=['A','B','C','D','E','F']
        
        r=random.randint(0,100)
        if r<60:
#        test_result=statuses[]
          die["status"]='A'
        elif r<80:
          die["status"]='B'
        elif r<90:
          die["status"]='C'
        else:
          die["status"]='F'

        test_date=time.strftime('%Y/%m/%d %H:%M:%S')
        test_date_fname=time.strftime('%Y%m%d_%H%M%S')

        test_fname=os.path.join(dir_name,"%s_%03d_%s.txt"%(die['name'],test_id,test_date_fname))
        
        f=open(test_fname,"w")
        f.write("DIE   : %s\n"%die['name'])
        f.write("DATE  : %s\n"%test_date)
        f.write("RESULT: %s\n"%test_result)
        f.write("\n\nDetails:\n")
        for i in range(256):
          f.write(" result %d\n"%i)
        f.close()
        
        die['tests'].append( {'id':test_id, 'fname':test_fname, 'date':test_date,'result': test_result})
#        dlg=LogViewer(None,test_fname)
#        dlg.ShowModal()
#        dlg.Destroy()
        self.save()
        self.Refresh()
        
    def OnTestDie(self,event):
        self._test_die(self.rclic_die['name'])

    def OnSkipDie(self,event):
        self.rclic_die['skip']=not self.rclic_die['skip']
        self.save()
        self.Refresh()
    def OnSetHome(self,event):
        self.home=self.rclic_die['name']
        print self.home
        self.save()
        self.Refresh()

    
    def OnLogEntry(self, event):
        log_id = self.popup_logs_entries.index(event.GetId())
#        print self.rclic_die['tests'][log_id]
        fname=self.rclic_die['tests'][log_id]['fname']
        dlg=LogViewer(None,fname)
        dlg.ShowModal()
        dlg.Destroy()

#        path = self.filehistory.GetHistoryFile(fileNum)
#        self.filehistory.AddFileToHistory(path)
#        self.OpenWafer(path)
#        print log_id
        
    def SetName(self,name):
        self.name=name

    def SetNumber(self,number):
        self.number=number


    def OnGoTo(self,event):
        did=self._get_die_id_by_name(self.rclic_die['name'])
        self.goToDieNo(did)

    def OnSize(self, event):
        self.Refresh()

    def save(self):
      root = ET.Element("wafer")
      root.set("diameter", "%.3f"%self.diameter)

      for die in self.dies:
        d = ET.SubElement(root, "die")
        for k in die.keys():
          d.set(k, str(die[k]))
      if self.notch:
        d = ET.SubElement(root, "notch")
        for k in self.notch:
          d.set(k, str(self.notch[k]))

      d = ET.SubElement(root, "home")
      d.set("name",self.home)
      
      tree = ET.ElementTree(root)
      tree.write(self.fname)

    def load(self,fname):
      print "->load",fname
      self.fname=fname
      xmldoc = minidom.parse(fname)
      waferlist = xmldoc.getElementsByTagName('wafer') 
      if len(waferlist)!=1:
        print "No wafer defined."
        return
      if 'diameter' in waferlist[0].attributes.keys():
        self.diameter=float(waferlist[0].attributes['diameter'].value)
      else:
        print "No diameter defined!"
        return

      notchlist = xmldoc.getElementsByTagName('notch') 
      if len(notchlist)!=1:
        print "No notch found."
        return
      self.notch={'angle':float(notchlist[0].attributes['angle'].value),
                  'length':float(notchlist[0].attributes['length'].value)}

      homelist = xmldoc.getElementsByTagName('home') 
      if len(homelist)!=1:
        print "No home found."
      else:
        self.home=homelist[0].attributes['name'].value



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
             'h':float(s.attributes['h'].value)
             }
        if 'status' in s.attributes.keys():
           die['status']=s.attributes['status'].value
        else:
           die['status']='?'
        def str2bool(s):
          if s.lower() in ['true', '1', 't', 'y', 'yes']:
            return True
          return False
        if 'skip' in s.attributes.keys():
           die['skip']=str2bool(s.attributes['skip'].value)
        else:
           die['skip']=False

        if 'tests' in s.attributes.keys():
           die['tests']=ast.literal_eval(s.attributes['tests'].value)
        else:
           die['tests']=[]
        self.dies.append(die)
        
      print "Loaded dies",len(self.dies)

    
    def die2screen(self,pos):
        x,y=pos
        x=self.x0+x*self.scale
        y=self.y0+y*self.scale
        return (x,y)
    def _dies_to_scan(self):
        i=0
        for d in self.dies:
          if d['skip']: continue
          i+=1
        return i
        
    def _first_die_to_scan(self):
        self.dieno=0
        while self.dies[self.dieno]['skip']:
          self.dieno+=1
        return self.dieno
        

     
     
    def ScanAll(self):
        progressMax = self._dies_to_scan()
        
        dialog = wx.ProgressDialog("Wafer scanning", "Time remaining", progressMax,
                                   style=wx.PD_APP_MODAL|wx.PD_CAN_ABORT | wx.PD_ELAPSED_TIME | wx.PD_REMAINING_TIME)
        dialog.Centre()
        keepGoing = True
        count = 0
        did=self._first_die_to_scan()
        scanned=0
        while keepGoing and did<len(self.dies):
          self._test_die(self.dies[did]['name'])
          keepGoing, skip = dialog.Update(scanned,"%d/%d %s"%(scanned,progressMax,self.dies[did]['name']))
          scanned+=1
          did+=1
          while did<len(self.dies) and self.dies[did]['skip']:
            did+=1
          wx.MilliSleep(10)
        dialog.Destroy()
        
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

        self.scale=0.95*min ( (MX,MY) )/self.diameter
#        print MX,MY,self.diameter, self.scale

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
        drawCircle (self.x0,self.y0,self.diameter/2.0*self.scale)

        dc.SetBrush( wx.Brush(wx.Colour(230,230,230),wx.TRANSPARENT) )


        dc.DrawCircle (self.x0,self.y0,self.diameter/2*self.scale)
        dc.DrawCircle (self.x0,self.y0,self.diameter/2*self.scale*0.995)

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
            
          if self.home==die['name']:
            dc.SetBrush(wx.Brush(wx.Colour(0,0,255)) )
            dc.SetPen(wx.Pen(wx.Colour(0,0,120), 1, wx.SOLID))
            x0,y0=die['x']+die['w'],die['y']
            x0,y0=self.die2screen( (x0,y0) )
            x0-=7
            y0+=5
            dc.DrawCircle (x0,y0,2)
            dc.SetBrush( wx.Brush(wx.Colour(230,230,230),wx.TRANSPARENT) )


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
        y0=-(self.diameter/2-self.notch['length'])
        a=self.notch['angle']/180*3.1415
        x1_=-self.notch['length']/2
        y1_=-(self.diameter/2)
        x0,y0=self.die2screen((x0,y0))
        x1,y1=self.die2screen((x1_,y1_))
        x2_=self.notch['length']/2
        y2_=-(self.diameter/2)
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

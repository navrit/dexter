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
from WaferMap import *
from NewWafer import NewWafer
import glob

class OPException(Exception):
    pass

ico = PyEmbeddedImage(
    "iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAABHNCSVQICAgIfAhkiAAAAAlw"
    "SFlzAAAN1wAADdcBQiibeAAAABl0RVh0U29mdHdhcmUAd3d3Lmlua3NjYXBlLm9yZ5vuPBoA"
    "AAC6SURBVDiNpZMhDsMwDEWfpx6krJcYLC4f6zHG0sLdImx8eGcIH+tNPNBZjdKmibSPYlv5"
    "+v75EVWlBJG5BcaDkW+Kt1eMwA34RL0OoJaA3+VXVA8rgUhOnsHDlB02GXmGriTLVkjlGYaE"
    "LK1DrQf+oBeofQVVt5Ax4lKpIIu/CWyF1CCifjgjEIWWkxzce96PK33U8up02QgKf0Fmmdhy"
    "0gFPdTrZvNYDy8kubLUE5tEumTU58NE5JDVfI9osXTfTOocAAAAASUVORK5CYII=")


class WaferTester(wx.Frame):
  def __init__(self,fname=None):
     wx.Frame.__init__(self, None, wx.ID_ANY, "waferTESTER", size=(800,870))#,style= wx.SYSTEM_MENU | wx.CAPTION | wx.CLOSE_BOX)

     self.map_panel=MapPanel(self)

     self.filehistory = wx.FileHistory(8)
     self.config = wx.Config("waferprober", style=wx.CONFIG_USE_LOCAL_FILE)
     self.filehistory.Load(self.config)

     mainsizer= wx.BoxSizer(wx.VERTICAL) # left controls, right image output
     
     self.titleBar  = bp.ButtonPanel(self, -1,  agwStyle=wx.NO_BORDER)
     self.indices = []
     self.button_handlers=dict()

     self.buttons=(
          (icons.compfile.GetBitmap(),    wx.ID_NEW,     'New wafer', self.OnNewWafer,'New wafer'       ,'Normal'),
          (icons.settings.GetBitmap(),    wx.NewId(),    'Settings',  self.OnSettings,'Settings'        ,'Normal'),
          (icons.start.GetBitmap(),       wx.NewId(),    'Start',     self.OnStart,'Open file'       ,'Normal'),
          (None,   wx.ID_OPEN,       'Home',      self.OnStart,'Home'       ,'Normal'),
          (icons.gohome.GetBitmap(),      wx.NewId(),    'Home',      self.OnStart,'Home'       ,'Normal'),
          (icons.player_eject.GetBitmap(),wx.NewId(),    'Eject',     self.OnStart,'Eject'       ,'Normal'),
          (icons.leftarrow.GetBitmap(),   wx.NewId(),    'Step Left', self.OnStart,'Open file'       ,'Normal'),
          (icons.rightarrow.GetBitmap(),  wx.NewId(),    'Step Right',self.OnStart,'Open file'       ,'Normal'),
          (icons.uparrow.GetBitmap(),     wx.NewId(),    'Step Up',   self.OnStart,'Open file'       ,'Normal'),
          (icons.downarrow.GetBitmap(),   wx.NewId(),    'Step Down', self.OnStart,'Open file'       ,'Normal'),
        )

     for icon_png, wxid,short_help,handler, long_help,status  in self.buttons:
            if not icon_png:
              self.titleBar.AddSeparator()
              continue
            kind = wx.ITEM_NORMAL
            btn = bp.ButtonInfo(self.titleBar, wxid , icon_png, kind=kind, shortHelp=short_help, longHelp=long_help,status=status)
#            btn.SetText(short_help)
            self.titleBar.AddButton(btn)
            self.button_handlers[short_help]=btn
            self.Bind(wx.EVT_BUTTON, handler, id=btn.GetId())
            self.indices.append(btn.GetId())
#     self.titleBar.AddSeparator()
#     self.Centre()
     print "%x"%self.titleBar.GetBorder()
     

     
#     self.Bind(wx.EVT_SIZE, self._onSize)

     wafer_cnf_sizer= wx.BoxSizer(wx.HORIZONTAL)
     
     wname_box         = wx.StaticBox(self, wx.ID_ANY, "Wafer name")
     wname_sizer       = wx.StaticBoxSizer(wname_box, wx.VERTICAL)
     self.wname_info   = wx.TextCtrl(self,value="-------",style=wx.TE_READONLY)

     wafer_font = wx.Font(20, wx.FONTFAMILY_TELETYPE, wx.NORMAL, wx.NORMAL)
     self.wname_info.SetFont(wafer_font)



     wname_sizer.Add(self.wname_info, 3,  wx.GROW|wx.EXPAND, 3)
     
#     wafer_cnf_sizer.Add(wname_box, 1, wx.LEFT|wx.GROW| wx.EXPAND, 1)
     
     wnumber_box       = wx.StaticBox(self, wx.ID_ANY, "Wafer number")
     wnumber_sizer     = wx.StaticBoxSizer(wnumber_box, wx.VERTICAL)
     self.wnumber_info = wx.TextCtrl(self,value="-",style=wx.TE_READONLY)
     wnumber_sizer.Add(self.wnumber_info, 1,  wx.GROW|wx.EXPAND, 1)

     self.wnumber_info.SetFont(wafer_font)

     
#     wafer_cnf_sizer.Add(wnumber_box, 1, wx.LEFT|wx.GROW| wx.EXPAND, 1)
     self.titleBar.DoLayout()

     wafer_cnf_sizer.Add(self.titleBar, 0, wx.ALL|wx.GROW| wx.EXPAND, 2)
     wafer_cnf_sizer.Add(wname_sizer, 1, wx.ALL|wx.GROW| wx.EXPAND, 2)
     wafer_cnf_sizer.Add(wnumber_sizer, 1, wx.ALL|wx.GROW|wx.EXPAND, 2)


#     mainsizer.Add(self.titleBar, 0, wx.EXPAND, border=0)
     
     mainsizer.Add(wafer_cnf_sizer, 0,  wx.GROW|wx.EXPAND, 1)
#     self.titleBar.AddControl(wafer_cnf_sizer,1,flag=wx.EXPAND)
#     self.titleBar.SetAlignment(bp.BP_ALIGN_RIGHT)
     mainsizer.Add(self.map_panel, 1,  wx.EXPAND, 1)
#     mainsizer.Fit(self)
     self.SetIcon(ico.GetIcon()) 
     
     menubar = wx.MenuBar()
     fileMenu = wx.Menu()

     file_new=fileMenu.Append(wx.ID_NEW, '&New Wafer')
     self.Bind(wx.EVT_MENU, self.OnNewWafer, file_new)

     file_open=fileMenu.Append(wx.ID_OPEN, '&Open')

     recent = wx.Menu()
     self.filehistory.UseMenu(recent)
     self.filehistory.AddFilesToMenu()
     file_recent=fileMenu.AppendMenu(wx.ID_ANY, "&Recent Wafers",recent)
     self.Bind(wx.EVT_MENU_RANGE, self.OnWaferHistory, id=wx.ID_FILE1, id2=wx.ID_FILE9)
     fileMenu.AppendSeparator()
     file_exit = fileMenu.Append(wx.ID_EXIT, '&Quit')

     self.Bind(wx.EVT_MENU, self.OnOpen, file_open)
#     self.Bind(wx.EVT_MENU, self.OnFileSaveAs, file_save)
     self.Bind(wx.EVT_MENU, self.OnQuit, file_exit)
     menubar.Append(fileMenu, '&File')
     self.SetMenuBar(menubar)

     viewMenu = wx.Menu()
     view_zoom_fit=viewMenu.Append(wx.ID_ZOOM_FIT, 'Zoom &fit')
     view_zoom_in=viewMenu.Append(wx.ID_ZOOM_IN, 'Zoom &in')
     view_zoom_out=viewMenu.Append(wx.ID_ZOOM_IN, 'Zoom &out')
     self.savr = viewMenu.Append(wx.ID_ANY, 'Show averages', 'Show Averages', kind=wx.ITEM_CHECK)
#     self.Bind(wx.EVT_MENU, self.ToggleShowAvr, self.savr)
     viewMenu.Check(self.savr.GetId(), True)

#     show_mod_win=viewMenu.Append(wx.ID_ZOOM_IN, 'Show &modulo window')
#     self.Bind(wx.EVT_MENU, self.OnZoomFit, view_zoom_fit)


#     self.smodwin = viewMenu.Append(wx.ID_ANY, 'Show modulo window', 
#            'Show Modulo window', kind=wx.ITEM_CHECK)
#     self.Bind(wx.EVT_MENU, self.ToggleModWin, self.smodwin)
#     viewMenu.Check(self.smodwin.GetId(), False)

     menubar.Append(viewMenu, '&View')
     
     helpMenu = wx.Menu()
     file_about=helpMenu.Append(wx.ID_ABOUT, "&About", "Display information about the program")
     self.Bind(wx.EVT_MENU, self.OnHelpAbout, file_about)
     menubar.Append(helpMenu, '&Help')

     if fname:
       self.OpenWafer(fname)


     mainsizer.Layout()
     self.SetSizer(mainsizer)

     self.Refresh()
     self.SetMinSize(self.GetEffectiveMinSize())
     self._wafer_dir="./wafers"
     self.Update()
     self.Refresh()
     self.Show(True)
     wx.Yield()

  def OnSettings(self,e):
    pass
    
  def on_timer(self,event):
    if not self.map_panel.stepNext():
      self.timer.Stop()

  def OpenWafer(self,fname):
    self.map_panel.load(fname)
    self.Refresh()
    self.filehistory.AddFileToHistory(fname)
    self.filehistory.Save(self.config)
    self.config.Flush()

  def OnStart(self,e):
#    TIMER_ID = 100  # pick a number
#    self.timer = wx.Timer(self, TIMER_ID)  # message will be sent to the panel
#    self.timer.Start(500)  # x100 milliseconds
#    wx.EVT_TIMER(self, TIMER_ID, self.on_timer)  # call the on_timer function
    
    self.map_panel.ScanAll()

 

    
  def OnWaferHistory(self, event):
    fileNum = event.GetId() - wx.ID_FILE1
    path = self.filehistory.GetHistoryFile(fileNum)
    self.filehistory.AddFileToHistory(path)
    self.OpenWafer(path)


  def mkdir(self,d):
    if not os.path.exists(d):
      os.makedirs(d)  

  def OnNewWafer(self,e):
    from glob import glob
    import shutil
    templates=[]
    for fn in glob("templates/*.xml"):
      die=os.path.splitext(os.path.basename(fn))[0]
      templates.append(die)
    dlg=NewWafer(wafer_types=templates)
    if dlg.ShowModal()== wx.ID_OK:
      wname     = "%03d_%s"%(int(dlg.GetNumber()),dlg.GetName())
      wafer_dir = os.path.join(self._wafer_dir,wname)
      self.mkdir(wafer_dir)
      wafer_file=os.path.join(wafer_dir,wname+'.xml')
      tname="templates/%s.xml"%dlg.GetType()
      shutil.copyfile(tname,wafer_file)
      self.OpenWafer(wafer_file)
    dlg.Destroy()

  def OnOpen(self, e):
#        """ File|Open event - Open dialog box. """
#        dirName=self._wafer_dir
#        dlg = wx.DirDialog(self, "Open Wafer", dirName, style= wx.OPEN)
#        if (dlg.ShowModal() == wx.ID_OK):
#            dirName = dlg.GetPath()
#            print dirName
#            self.OpenWafer(dirName)
#        dlg.Destroy()
        """ File|Open event - Open dialog box. """
        dirName=self._wafer_dir
        fileName=''
        dlg = wx.FileDialog(self, "Open", dirName, fileName,
                           "Wafer Description (*.xml)|*.xml|All Files|*.*", wx.OPEN)
        if (dlg.ShowModal() == wx.ID_OK):
            fileName = dlg.GetFilename()
            dirName = dlg.GetDirectory()
            fn=os.path.join(dirName, fileName)
            self.OpenWafer(str(fn))
        dlg.Destroy()



  def OnHelpAbout(self, e):
    """ Help|About event """
    title = self.GetTitle()
    d = wx.MessageDialog(self, "waferTESTER v0.1\nAuthor: Szymon Kulis\n2014 CERN","About" , wx.ICON_INFORMATION | wx.OK)
    d.ShowModal()
    d.Destroy()

  def OnQuit(self, e):
    self.Close()

  def EvtColorComboBox(self,event):
    cb = event.GetEventObject()
    self.data_map.change_cm(event.GetString())
    self.refresh()
     

  def refresh(self):
    self.map_panel.refresh()
    self.hist_panel.refresh()
    self.mw.refresh()






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
     wx.Frame.__init__(self, None, wx.ID_ANY, "waferTESTER", size=(920,600))#,style= wx.SYSTEM_MENU | wx.CAPTION | wx.CLOSE_BOX)

     self.map_panel=MapPanel(self)

     self.filehistory = wx.FileHistory(8)
     self.config = wx.Config("waferprober", style=wx.CONFIG_USE_LOCAL_FILE)
     self.filehistory.Load(self.config)

     mainsizer= wx.BoxSizer(wx.HORIZONTAL) # left controls, right image output
     
#     self.titleBar  = bp.ButtonPanel(self, -1,  agwStyle=wx.NO_BORDER)
#     self.indices = []
#     self.button_handlers=dict()

#     self.buttons=(
#          (icons.compfile.GetBitmap(),    wx.ID_NEW,     'New wafer', self.OnNewWafer,'New wafer'       ,'Normal'),
#          (icons.settings.GetBitmap(),    wx.NewId(),    'Settings',  self.OnSettings,'Settings'        ,'Normal'),
#          (icons.start.GetBitmap(),       wx.NewId(),    'Start',     self.OnStart,'Open file'       ,'Normal'),
#          (None,   wx.ID_OPEN,       'Home',      self.OnStart,'Home'       ,'Normal'),
#          (icons.gohome.GetBitmap(),      wx.NewId(),    'Home',      self.OnStart,'Home'       ,'Normal'),
#          (icons.player_eject.GetBitmap(),wx.NewId(),    'Eject',     self.OnStart,'Eject'       ,'Normal'),
#          (icons.leftarrow.GetBitmap(),   wx.NewId(),    'Step Left', self.OnStart,'Open file'       ,'Normal'),
#          (icons.rightarrow.GetBitmap(),  wx.NewId(),    'Step Right',self.OnStart,'Open file'       ,'Normal'),
#          (icons.uparrow.GetBitmap(),     wx.NewId(),    'Step Up',   self.OnStart,'Open file'       ,'Normal'),
#          (icons.downarrow.GetBitmap(),   wx.NewId(),    'Step Down', self.OnStart,'Open file'       ,'Normal'),
#        )

#     for icon_png, wxid,short_help,handler, long_help,status  in self.buttons:
#            if not icon_png:
#              self.titleBar.AddSeparator()
#              continue
#            kind = wx.ITEM_NORMAL
#            btn = bp.ButtonInfo(self.titleBar, wxid , icon_png, kind=kind, shortHelp=short_help, longHelp=long_help,status=status)
##            btn.SetText(short_help)
#            self.titleBar.AddButton(btn)
#            self.button_handlers[short_help]=btn
#            self.Bind(wx.EVT_BUTTON, handler, id=btn.GetId())
#            self.indices.append(btn.GetId())
##     self.titleBar.AddSeparator()
##     self.Centre()
#     print "%x"%self.titleBar.GetBorder()
     


     ProjectGridSizer = wx.GridSizer(rows=1, cols=5, hgap=5, vgap=5)
     
     btn_new    =      wx.BitmapButton(self, id=wx.NewId(), bitmap=icons.cd.GetBitmap(),  name="up", style = wx.NO_BORDER)
     self.Bind(wx.EVT_BUTTON, self.OnNewWafer,btn_new)
     
     btn_run =      wx.BitmapButton(self, id=-1, bitmap=icons.start.GetBitmap(), style = wx.NO_BORDER)
     btn_run.Bind(wx.EVT_BUTTON, self.OnStart)

     btn_settings = wx.BitmapButton(self, id=-1, bitmap=icons.settings.GetBitmap(), style = wx.NO_BORDER)
     btn_settings.Bind(wx.EVT_BUTTON, self.OnSettings)
     
     ProjectGridSizer.Add( btn_new, 0, wx.ALIGN_RIGHT)
     ProjectGridSizer.Add( btn_settings, 0, wx.ALIGN_RIGHT)
     ProjectGridSizer.Add( btn_run, 0, wx.ALIGN_RIGHT)
     ProjectSizer = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"Menu" ), wx.HORIZONTAL )
     ProjectSizer.Add( ProjectGridSizer, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5 )
     
     gridSizer = wx.GridSizer(rows=3, cols=5, hgap=5, vgap=5)
#     btn = bp.ButtonInfo(self, wx.NewId() , icons.gohome.GetBitmap(), kind=wx.ITEM_NORMAL, shortHelp="short_help", longHelp="long_help",status='Normal')
#     imageFile = "Btn_down.jpg"
#     image1 = wx.Image(imageFile, wx.BITMAP_TYPE_ANY).ConvertToBitmap()
     btn_up = wx.BitmapButton(self, id=-1, bitmap=icons.uparrow.GetBitmap(),    style = wx.NO_BORDER)
     btn_down   = wx.BitmapButton(self, id=-1, bitmap=icons.downarrow.GetBitmap() , style = wx.NO_BORDER)
     btn_left   = wx.BitmapButton(self, id=-1, bitmap=icons.leftarrow.GetBitmap() , style = wx.NO_BORDER)
     btn_home   = wx.BitmapButton(self, id=-1, bitmap=icons.gohome.GetBitmap()    , style = wx.NO_BORDER)
     btn_right  = wx.BitmapButton(self, id=-1, bitmap=icons.rightarrow.GetBitmap(), style = wx.NO_BORDER)

     btn_home.Bind(wx.EVT_BUTTON, self.OnProbeStationHome)

    
     self.btn_safe    = wx.BitmapButton(self, id=-1, bitmap=icons.top.GetBitmap()     , style = wx.NO_BORDER)
     self.btn_safe.Enable(False)
     self.btn_contact = wx.BitmapButton(self, id=-1, bitmap=icons.bottom.GetBitmap()  , style = wx.NO_BORDER)
     self.btn_align  = wx.BitmapButton(self, id=-1, bitmap=icons.edit.GetBitmap()     , style = wx.NO_BORDER)

     self.btn_safe.Bind(wx.EVT_BUTTON, self.OnProbeStationHeight)
     self.btn_align.Bind(wx.EVT_BUTTON, self.OnProbeStationHeight)
     self.btn_contact.Bind(wx.EVT_BUTTON, self.OnProbeStationHeight)
     
#     btn_right = wx.BitmapButton(self, id=-1, bitmap=icons.rightarrow.GetBitmap())
#     btn1_sizer.Add(btn,0,wx.ALL,0)
#    self.button1.Bind(wx.EVT_BUTTON, self.button1Click)


     gridSizer.Add(wx.StaticText(self, -1, ''), 0, wx.ALIGN_RIGHT)
     gridSizer.Add(btn_up, 0, wx.ALIGN_RIGHT)
     gridSizer.Add(wx.StaticText(self, -1, ''), 0, wx.ALIGN_RIGHT)
     gridSizer.Add(wx.StaticText(self, -1, ''), 0, wx.ALIGN_RIGHT)
     gridSizer.Add(self.btn_safe, 0, wx.ALIGN_RIGHT)
     
     
     gridSizer.Add(btn_left, 0, wx.ALIGN_RIGHT)
     gridSizer.Add(btn_home, 0, wx.ALIGN_RIGHT)
     gridSizer.Add(btn_right, 0, wx.ALIGN_RIGHT)
     gridSizer.Add(wx.StaticText(self, -1, ''), 0, wx.ALIGN_RIGHT)
     gridSizer.Add(self.btn_align, 0, wx.ALIGN_RIGHT)

     gridSizer.Add(wx.StaticText(self, -1, ''), 0, wx.ALIGN_RIGHT)
     gridSizer.Add(btn_down, 0, wx.ALIGN_RIGHT)
     gridSizer.Add(wx.StaticText(self, -1, ''), 0, wx.ALIGN_RIGHT)
     gridSizer.Add(wx.StaticText(self, -1, ''), 0, wx.ALIGN_RIGHT)
     gridSizer.Add(self.btn_contact, 0, wx.ALIGN_RIGHT)
     
#     btn_bottom.Disable() 
     

     bSizer = wx.StaticBoxSizer( wx.StaticBox( self, wx.ID_ANY, u"Probe station" ), wx.HORIZONTAL )
     bSizer.Add( gridSizer, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5 )

#     gridSizer.Add(btn_down, 0, wx.ALIGN_RIGHT)
     
     
##     self.Bind(wx.EVT_SIZE, self._onSize)

     menu_sizer= wx.BoxSizer(wx.VERTICAL)
     menu_sizer.Add(ProjectSizer, 0, wx.ALL, border=3)
     
     menu_sizer.Add(bSizer, 0, wx.ALL, border=3)
     
#     wname_box         = wx.StaticBox(self, wx.ID_ANY, "Wafer name")
#     wname_sizer       = wx.StaticBoxSizer(wname_box, wx.VERTICAL)
#     self.wname_info   = wx.TextCtrl(self,value="-------",style=wx.TE_READONLY)

#     wafer_font = wx.Font(20, wx.FONTFAMILY_TELETYPE, wx.NORMAL, wx.NORMAL)
#     self.wname_info.SetFont(wafer_font)



#     wname_sizer.Add(self.wname_info, 3,  wx.GROW|wx.EXPAND, 3)
#     
##     wafer_cnf_sizer.Add(wname_box, 1, wx.LEFT|wx.GROW| wx.EXPAND, 1)
#     
#     wnumber_box       = wx.StaticBox(self, wx.ID_ANY, "Wafer number")
#     wnumber_sizer     = wx.StaticBoxSizer(wnumber_box, wx.VERTICAL)
#     self.wnumber_info = wx.TextCtrl(self,value="-",style=wx.TE_READONLY)
#     wnumber_sizer.Add(self.wnumber_info, 1,  wx.GROW|wx.EXPAND, 1)

#     self.wnumber_info.SetFont(wafer_font)

#     
##     wafer_cnf_sizer.Add(wnumber_box, 1, wx.LEFT|wx.GROW| wx.EXPAND, 1)
#     self.titleBar.DoLayout()

#     wafer_cnf_sizer.Add(self.titleBar, 0, wx.ALL|wx.GROW| wx.EXPAND, 2)
     
#     wafer_cnf_sizer.Add(bSizer, 0, wx.ALL|wx.GROW| wx.EXPAND, 2)
     
#     wafer_cnf_sizer.Add(wname_sizer, 1, wx.ALL|wx.GROW| wx.EXPAND, 2)
#     wafer_cnf_sizer.Add(wnumber_sizer, 1, wx.ALL|wx.GROW|wx.EXPAND, 2)


##     mainsizer.Add(self.titleBar, 0, wx.EXPAND, border=0)
#     
#     mainsizer.Add(wafer_cnf_sizer, 0,  wx.GROW|wx.EXPAND, 1)

#     self.titleBar.AddControl(wafer_cnf_sizer,1,flag=wx.EXPAND)
#     self.titleBar.SetAlignment(bp.BP_ALIGN_RIGHT)
     mainsizer.Add(menu_sizer, 0,  0, 10)

     mainsizer.Add(self.map_panel, 1,  wx.EXPAND, 1)
#     mainsizer.Fit(self)
     self.SetIcon(ico.GetIcon()) 
     
     menubar = wx.MenuBar()
     fileMenu = wx.Menu()

     file_new=fileMenu.Append(wx.ID_NEW, '&New Wafer')
     self.Bind(wx.EVT_MENU, self.OnNewWafer,  id=wx.ID_NEW)


     file_open=fileMenu.Append(wx.ID_OPEN, '&Open')

     recent = wx.Menu()
     self.filehistory.UseMenu(recent)
     self.filehistory.AddFilesToMenu()
     file_recent=fileMenu.AppendMenu(wx.ID_ANY, "&Recent Wafers",recent)
     self.Bind(wx.EVT_MENU_RANGE, self.OnWaferHistory, id=wx.ID_FILE1, id2=wx.ID_FILE9)
     fileMenu.AppendSeparator()
     file_exit = fileMenu.Append(wx.ID_EXIT, '&Quit')

     self.Bind(wx.EVT_MENU, self.OnOpen, id=file_open.GetId())
#     self.Bind(wx.EVT_MENU, self.OnFileSaveAs, file_save)
     self.Bind(wx.EVT_MENU, self.OnQuit, file_exit)
     menubar.Append(fileMenu, '&File')
     self.SetMenuBar(menubar)

#     viewMenu = wx.Menu()
#     view_zoom_fit=viewMenu.Append(wx.ID_ZOOM_FIT, 'Zoom &fit')
#     view_zoom_in=viewMenu.Append(wx.ID_ZOOM_IN,   'Zoom &in')
#     view_zoom_out=viewMenu.Append(wx.ID_ZOOM_IN, 'Zoom &out')
#     self.savr = viewMenu.Append(wx.ID_ANY, 'Show averages', 'Show Averages', kind=wx.ITEM_CHECK)
#     viewMenu.Check(self.savr.GetId(), True)

#     menubar.Append(viewMenu, '&View')
     print wx.ID_NEW
     ProbeStationMenu = wx.Menu()
     spacer1 = ProbeStationMenu.Append(wx.NewId(), 'Horizontal')
     spacer1.Enable(False)
     ProbeStationMenu.Append(btn_up.GetId(),       '&Up')
     ProbeStationMenu.Append(btn_down.GetId(),     '&Down')
     ProbeStationMenu.Append(btn_left.GetId(),     '&Left')
     ProbeStationMenu.Append(btn_right.GetId(),    '&Right')

     ProbeStationMenu.AppendSeparator()
     spacer2 = ProbeStationMenu.Append(wx.NewId(), 'Vertical')
     spacer2.Enable(False)
     self.btn_safe_menu=ProbeStationMenu.Append(self.btn_safe.GetId(),     '&Safe')
     self.Bind(wx.EVT_MENU, self.OnProbeStationHeight, self.btn_safe, id=self.btn_safe.GetId())
     self.btn_safe_menu.Enable(False)
     
     self.btn_align_menu=ProbeStationMenu.Append(self.btn_align.GetId(),    '&Align')
     self.Bind(wx.EVT_MENU, self.OnProbeStationHeight, id=self.btn_align.GetId())

     self.btn_contact_menu=ProbeStationMenu.Append(self.btn_contact.GetId(),  '&Concact')
     self.Bind(wx.EVT_MENU,   self.OnProbeStationHeight, id=self.btn_contact.GetId())

#          btn_up = wx.BitmapButton(self, id=-1, bitmap=icons.uparrow.GetBitmap(),    style = wx.NO_BORDER)
#     btn_down   = wx.BitmapButton(self, id=-1, bitmap=icons.downarrow.GetBitmap() , style = wx.NO_BORDER)
#     btn_left   = wx.BitmapButton(self, id=-1, bitmap=icons.leftarrow.GetBitmap() , style = wx.NO_BORDER)
#     btn_home   = wx.BitmapButton(self, id=-1, bitmap=icons.gohome.GetBitmap()    , style = wx.NO_BORDER)
#     btn_right  = wx.BitmapButton(self, id=-1, bitmap=icons.rightarrow.GetBitmap(), style = wx.NO_BORDER)

#     view_zoom_out = ProbeStationMenu.Append(wx.ID_ZOOM_IN, 'Zoom &out')
#     self.savr     = ProbeStationMenu.Append(wx.ID_ANY, 'Show averages', 'Show Averages', kind=wx.ITEM_CHECK)
#     self.Bind(wx.EVT_MENU, self.ToggleShowAvr, self.savr)
#     viewMenu.Check(self.savr.GetId(), True)
     menubar.Append(ProbeStationMenu, '&Probe station')


     
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

  def OnProbeStationHome(self,e):
      self.map_panel.GoHome()
      
  def OnProbeStationHeight(self,e):
     if e.GetId()== self.btn_safe.GetId():
       self.btn_safe.Disable()
       self.btn_align.Enable()
       self.btn_contact.Enable()
       self.btn_safe_menu.Enable(False)
       self.btn_align_menu.Enable(True)
       self.btn_contact_menu.Enable(True)
     elif e.GetId()== self.btn_align.GetId():
       self.btn_safe.Enable()
       self.btn_align.Disable()
       self.btn_contact.Enable()
       self.btn_safe_menu.Enable(True)
       self.btn_align_menu.Enable(False)
       self.btn_contact_menu.Enable(True)
     elif e.GetId()== self.btn_contact.GetId():
       self.btn_safe.Enable()
       self.btn_align.Enable()
       self.btn_contact.Disable()
       self.btn_safe_menu.Enable(True)
       self.btn_align_menu.Enable(True)
       self.btn_contact_menu.Enable(False)
     
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
    templates=[]
    for fn in glob("templates/*.xml"):
      die=os.path.splitext(os.path.basename(fn))[0]
      templates.append(die)
    dlg=NewWafer(wafer_types=templates)
    if dlg.ShowModal()== wx.ID_OK:
      wafer_name=dlg.GetName()
      wafer_number=int(dlg.GetNumber())
      tname="templates/%s.xml"%dlg.GetType()

      res=self.map_panel.CreateNewFromTemplate(wafer_name,wafer_number,template=tname,directory=self._wafer_dir)
      self.filehistory.AddFileToHistory(self.map_panel.fname)
      self.filehistory.Save(self.config)
      self.config.Flush()

      
#      wafer_dir = os.path.join(,wname)
#      self.mkdir(wafer_dir)
#      wafer_file=os.path.join(wafer_dir,wname+'.xml')
#      shutil.copyfile(tname,wafer_file)
#      self.OpenWafer(wafer_file)
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






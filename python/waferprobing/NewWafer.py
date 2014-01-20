#!/usr/bin/env python
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import matplotlib.mlab as mlab
import matplotlib.pyplot as plt
import wx
from math import pow,sqrt
import matplotlib.colors as colors
import matplotlib.cm as cm
import matplotlib
from wx.lib.embeddedimage import PyEmbeddedImage
import os


class WaferNumberValidator(wx.PyValidator):
     def __init__(self):
         wx.PyValidator.__init__(self)

     def Clone(self):
         return WaferNumberValidator()

     def Validate(self, win):
         textCtrl = self.GetWindow()
         text = textCtrl.GetValue()

         if len(text) == 0:
             wx.MessageBox("Wafer number can not be empty!", "Error")
             textCtrl.SetBackgroundColour("pink")
             textCtrl.SetFocus()
             textCtrl.Refresh()
             return False

         try:
             numeric=int(text)
         except:
             wx.MessageBox("Wafer number should be an integer!", "Error")
             textCtrl.SetBackgroundColour("pink")
             textCtrl.SetFocus()
             textCtrl.Refresh()
             return False
         #if everything went ok
         textCtrl.SetBackgroundColour(
           wx.SystemSettings_GetColour(wx.SYS_COLOUR_WINDOW))
         textCtrl.Refresh()
         return True

     def TransferToWindow(self):
         return True # Prevent wxDialog from complaining.


     def TransferFromWindow(self):
         return True # Prevent wxDialog from complaining.

class WaferNameValidator(wx.PyValidator):
     def __init__(self):
         wx.PyValidator.__init__(self)

     def Clone(self):
         return WaferNameValidator()

     def Validate(self, win):
         textCtrl = self.GetWindow()
         text = textCtrl.GetValue()

         if len(text) <7 :
             wx.MessageBox("Wafer name should have at least 7 characters!", "Error")
             textCtrl.SetBackgroundColour("pink")
             textCtrl.SetFocus()
             textCtrl.Refresh()
             return False
         else:
           #if everything went ok
           textCtrl.SetBackgroundColour(
             wx.SystemSettings_GetColour(wx.SYS_COLOUR_WINDOW))
           textCtrl.Refresh()
           return True

     def TransferToWindow(self):
         return True # Prevent wxDialog from complaining.


     def TransferFromWindow(self):
         return True # Prevent wxDialog from complaining.


class NewWafer(wx.Dialog):
    def __init__(self, wafer_types,*args, **kw):
#        super(NewWafer, self).__init__(*args, **kw) 
        wx.Dialog.__init__(self, None,*args, **kw)

        self.SetSize((250, 220))
        self.SetTitle("Create new wafer")
        self.wafer_types=wafer_types
        self._InitUI()
          
        
    def _InitUI(self):

        vbox = wx.BoxSizer(wx.VERTICAL)
        wname_box         = wx.StaticBox(self, wx.ID_ANY, "Wafer name")
        wname_sizer       = wx.StaticBoxSizer(wname_box, wx.VERTICAL)
        self.wname_info   = wx.TextCtrl(self,value="JOWLTH8G",validator=WaferNameValidator())
        wafer_font = wx.Font(25, wx.FONTFAMILY_TELETYPE, wx.NORMAL, wx.NORMAL)
        self.wname_info.SetFont(wafer_font)
        wname_sizer.Add(self.wname_info, 1,  wx.GROW|wx.EXPAND, 1)

        wnumber_box       = wx.StaticBox(self, wx.ID_ANY, "Wafer number")
        wnumber_sizer     = wx.StaticBoxSizer(wnumber_box, wx.VERTICAL)
        self.wnumber_info = wx.TextCtrl(self,value="000", validator=WaferNumberValidator())
        wnumber_sizer.Add(self.wnumber_info, 1,  wx.GROW|wx.EXPAND, 1)
        self.wnumber_info.SetFont(wafer_font)

        wafertype_box       = wx.StaticBox(self, wx.ID_ANY, "Wafer type")
        wafertype_sizer     = wx.StaticBoxSizer(wafertype_box, wx.VERTICAL)
        self.wafertype      = wx.ComboBox(self, -1, value=self.wafer_types[0], choices=self.wafer_types, style=wx.CB_READONLY)
        wafertype_sizer.Add(self.wafertype, 1,  wx.GROW|wx.EXPAND, 1)
        self.wafertype.SetFont(wafer_font)


        vbox.Add(wname_sizer, 1, wx.ALL|wx.GROW| wx.EXPAND, 2)
        vbox.Add(wnumber_sizer, 1, wx.ALL|wx.GROW|wx.EXPAND, 2)
        vbox.Add(wafertype_sizer, 0, wx.ALL|wx.GROW|wx.EXPAND, 2)


        hbox2 = wx.BoxSizer(wx.HORIZONTAL)
        okButton = wx.Button(self, wx.ID_OK, label='Ok')
        cancelButton = wx.Button(self, wx.ID_CANCEL,label='Cancel')
        hbox2.Add(okButton)
        hbox2.Add(cancelButton, flag=wx.LEFT, border=5)

        vbox.Add(hbox2, flag=wx.ALIGN_CENTER|wx.TOP|wx.BOTTOM, border=10)

        self.SetSizer(vbox)

    def GetName(self):
      return self.wname_info.GetValue()

    def GetNumber(self):
      return self.wnumber_info.GetValue()
    def GetType(self):
      return self.wafertype.GetValue()


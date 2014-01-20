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




class NewWafer(wx.Panel):
    def __init__(self, parent,data_map):
        wx.Panel.__init__(self, parent, size=(320,320))
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.data_map=data_map
        self.x0=0
        self.y0=0
        self.refresh()

    def OnPaint(self, evt):
        dc = wx.PaintDC(self)
        PX,PY=self.GetSize() 
        BX,BY=self.mod_bmp.GetSize()

        self.x0=(PX-BX)/2
        self.y0=(PY-BY)/2
        dc.DrawBitmap(self.mod_bmp, self.x0, self.y0, False)
        # To erase previous rectangle
        DX,DY=self.data_map.mod_bmp.GetSize()

        ls=BX
        ps=BX/DX
        if ps>=16: ls=ps
        for i in range(0,BX+1,ls):
            dc.DrawLine(self.x0+i,self.y0+0,self.x0+i,    self.y0+BY+1)

        ls=BY
        ps=BY/DY
        if ps>=16: ls=ps

        for i in range(0,BY+1,ls):
            dc.DrawLine(self.x0+0,self.y0+i,self.x0+BX+1,self.y0+i)

        dc.EndDrawing()

    def refresh(self):
        X,Y=self.data_map.mod_bmp.GetSize()
        N=max( (X,Y) )
        
        s=256/N
        image = wx.ImageFromBitmap(self.data_map.mod_bmp )
        image = image.Scale(X*s, Y*s, wx.IMAGE_QUALITY_NORMAL)
        self.mod_bmp = wx.BitmapFromImage(image)
        self.Refresh()


class ModuloWindow(wx.Frame):
    def __init__(self,data_map, size=(427,349)):
       wx.Frame.__init__(self, None, wx.ID_ANY, 'Modulo viewer', size=size,style= wx.SYSTEM_MENU | wx.CAPTION | wx.CLOSE_BOX)
       self.data_map=data_map

       topsizer= wx.BoxSizer(wx.HORIZONTAL) # left controls, right image output
       ctrlsizer= wx.BoxSizer(wx.VERTICAL)
 
       col_box = wx.StaticBox(self, -1, "Columns")
       col_sizer = wx.StaticBoxSizer(col_box, wx.VERTICAL)
       # This combobox is created with a preset list of values.
       mod_list=["2","4","8","16","32","64","128"]
       self.mod_col_combo = wx.ComboBox(self, 500, "4", (30, 50), 
                         (70, -1), mod_list,
                         wx.CB_DROPDOWN
                         | wx.TE_PROCESS_ENTER
                         | wx.CB_SORT
                         )
       self.mod_col_combo.Bind(wx.EVT_COMBOBOX, self.EvtModCol)
       col_sizer.Add(self.mod_col_combo , 0, wx.ALL, 2)
       ctrlsizer.Add(col_sizer)

       row_box = wx.StaticBox(self, -1, "Rows")
       row_sizer = wx.StaticBoxSizer(row_box, wx.VERTICAL)
       self.mod_row_combo = wx.ComboBox(self, 500, "4", (90, 50), 
                         (70, -1), mod_list,
                         wx.CB_DROPDOWN
                         | wx.TE_PROCESS_ENTER
                         | wx.CB_SORT
                         )
       self.mod_row_combo.Bind(wx.EVT_COMBOBOX, self.EvtModRol)
       row_sizer.Add(self.mod_row_combo , 0, wx.ALL, 2)
       ctrlsizer.Add(row_sizer)


       mod_box = wx.StaticBox(self, -1, "Modulo plot")
       mod_sizer = wx.StaticBoxSizer(mod_box, wx.VERTICAL)
       self.panel=ModPanel(self, data_map=self.data_map)

       mod_sizer.Add(self.panel , 0, wx.ALL, 2)
       
       topsizer.Add(ctrlsizer, 0, wx.ALL, 2)
       topsizer.Add(mod_sizer,0 ,  wx.ALL,2 )
       self.SetSizer(topsizer)
       topsizer.Layout()
    def refresh(self):
       self.panel.refresh()
    def EvtModCol(self,event):
#       cb = event.GetEventObject()
       self.data_map.mod_cols=int( event.GetString())
       self.data_map.process()
       self.refresh()
    def EvtModRol(self,event):
#       cb = event.GetEventObject()
       self.data_map.mod_rows=int( event.GetString())
       self.data_map.process()
       self.refresh()


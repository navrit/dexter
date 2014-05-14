#!/usr/bin/env python
import wx
from liveviewer import MyForm

if __name__ == '__main__':
    import sys,os
    app = wx.App( redirect=False )
    fname=None
    if len(sys.argv)>1:
      fname=sys.argv[1]
      
    frame = MyForm(fname=fname)
    if 0:
        from wx.lib.inspection import InspectionTool
        inspTool = InspectionTool()
        inspTool.Show( refreshTree=True )      
    
    app.MainLoop()

#!/usr/bin/env python
import wx
from viewer import MyForm

if __name__ == '__main__':
  import sys,os
  if len(sys.argv)<2:
     print "File name!"
  else:
    app = wx.App( redirect=False )

    frame = MyForm(fname=sys.argv[1])

    
    if 0:
        from wx.lib.inspection import InspectionTool
        inspTool = InspectionTool()
        inspTool.Show( refreshTree=True )      
    
    app.MainLoop()

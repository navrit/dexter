#!/usr/bin/env python
import wx
from WaferTester import WaferTester

def main():
    import sys,os
    app = wx.App( redirect=False )

    fname=None
    if len(sys.argv)>1:
      fname=sys.argv[1]
      
    
    frame = WaferTester(fname=fname)
    if 0:
        from wx.lib.inspection import InspectionTool
        inspTool = InspectionTool()
        inspTool.Show( refreshTree=True )      
    
    app.MainLoop()


if __name__ == '__main__':
  main()
#  w=Wafer("timepix3.xml")

#!/usr/bin/env python
import wx
from viewer import WaferTester

def main():
    import sys,os
    app = wx.App( redirect=False )

    frame = WaferTester(fname="timepix3.xml")
    if 0:
        from wx.lib.inspection import InspectionTool
        inspTool = InspectionTool()
        inspTool.Show( refreshTree=True )      
    
    app.MainLoop()


if __name__ == '__main__':
  main()
#  w=Wafer("timepix3.xml")

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
import pycallgraph
import os

try:
    from agw import floatspin as FS
except ImportError: # if it's not there locally, try the wxPython lib.
    import wx.lib.agw.floatspin as FS
DIM=256




class HistPanel(wx.Panel):
    def __init__(self, parent,size):
        wx.Panel.__init__(self, parent, size=size)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.grid=0
        self.hst=None
    def OnPaint(self, evt):
        dc = wx.PaintDC(self)
        wbrush = wx.Brush(wx.Colour(255,255,255), wx.SOLID)
        wpen = wx.Pen(wx.Colour(0,0,0), 1, wx.SOLID)
        dc.SetBrush(wbrush)
        dc.SetPen(wpen)
        dc.BeginDrawing()
        self.x0=30
        self.y0=10
        X,Y=self.GetSize()
        Y=Y-40

        X=512.0/2
        dc.DrawRectangle(self.x0,self.y0,X,Y)         


        wpen = wx.Pen(wx.Colour(0x80,0x80,0x80), 1, wx.SOLID)
        dc.SetPen(wpen)
        for i in range(4):
           xl=self.x0+i*X/4
           dc.DrawLine(xl,self.y0,xl, self.y0+Y)
           yl=self.y0+i*Y/4
           dc.DrawLine(self.x0,yl,self.x0+X, yl)


        wbrush = wx.Brush(wx.Colour(255,128,128), wx.SOLID)
        wpen = wx.Pen(wx.Colour(0xff,0,0), 1, wx.SOLID)
        dc.SetBrush(wbrush)
        dc.SetPen(wpen)

        if self.hst:
          counts,bins= self.hst
          BINSLEN=bins.shape[0]
          GAINBIN=bins[BINSLEN-1]-bins[0]
#          dx=X/(bins.shape[0]-1)
          mmax=np.amax(counts)
          for i in range(BINSLEN-1):
           #  print bins[i],counts[i]
             v=counts[i]*(Y*0.9)/mmax
             v=int(v)
#             x0=int(X*(bins[i]-bins[0])/GAINBIN+0.5)
#             dx=int(X*(bins[i+1]-bins[i])/GAINBIN +1)
             x0=i*4
             dx=5
             if i==BINSLEN-2:dx=4
             dc.DrawRectangle(self.x0+x0,self.y0+Y-v,dx,v) 
             
        if self.cm_bmp:
          dc.DrawBitmap(self.cm_bmp, self.x0, self.y0+Y, False)
          CM_TICS=4
          CM_LEN=self.cm_bmp.GetSize()[0]
          mmax=bins[BINSLEN-1]
          mmin=bins[0]

          for i in range(0,CM_TICS+1):
            v=float(mmin)+float(i)*(mmax-mmin)/CM_TICS
            lbl="%.2f"%(v)
            w,h = dc.GetTextExtent(lbl)
            x=float(i)/CM_TICS*CM_LEN-w/2
            dc.DrawText( lbl, self.x0+x, self.y0+Y+10) 

        wbrush = wx.Brush(wx.Colour(255,255,255), wx.TRANSPARENT)
        wpen = wx.Pen(wx.Colour(0,0,0), 1, wx.SOLID)
        dc.SetBrush(wbrush)
        dc.SetPen(wpen)
        dc.DrawRectangle(self.x0,self.y0,X,Y) 


        dc.EndDrawing()


class TestPanel(wx.Panel):
    def __init__(self, parent,fname):
        wx.Panel.__init__(self, parent, size=(512,512))
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.grid=0
        self.PPP=2 #on screen pixels per one pixel data
        self.MAX_PPP=64
        self.data=np.loadtxt(fname)
        self.mmin=np.amin(self.data)
        self.mmax=np.amax(self.data)

        self.Bind(wx.EVT_LEFT_DOWN, self.OnMouseEvent)
        self.Bind(wx.EVT_LEFT_UP, self.OnMouseEvent)
        self.Bind(wx.EVT_MOTION, self.OnMouseEvent)
        self.Bind(wx.EVT_MOUSEWHEEL, self.OnMouseWheel)

        self.m_stpoint=wx.Point(0,0)
        # mouse selection end point
        self.m_endpoint=wx.Point(0,0)
        # mouse selection cache point
        self.m_savepoint=wx.Point(0,0)
        # flags for left click/ selection
        self._leftclicked=False
        self._selected=False
        self.x0=32 #uper left corner in pixels
        self.y0=32 #uper left corner in pixels
        self.matrix_size=(512,512)
        self.p0=[0,0]
        self.update_pixinfo=None
        self.update_stats=None
        self.hst_panel=None
        self.change_cm('jet')

    def change_cm(self,new_cm):
        self.cm=cm.get_cmap(new_cm)
        self.cm.set_over(color=[1,0,0])
        self.cm.set_under(color=[0,0,1])
        transform = cm.ScalarMappable(cmap=self.cm)
        CBARLEL=256 #self.matrix_size[0]
        data=np.zeros((CBARLEL,1))
        for i in range(CBARLEL):
          data[i][0]=float(i)/CBARLEL
        cdata=transform.to_rgba(data,bytes=True)
        self.cm_bmp = wx.BitmapFromBufferRGBA(cdata.shape[0],cdata.shape[1], cdata)

        image = wx.ImageFromBitmap(self.cm_bmp)
        image = image.Scale(CBARLEL,10, wx.IMAGE_QUALITY_NORMAL)
        self.cm_bmp = wx.BitmapFromImage(image)
        if self.hst_panel:
          self.hst_panel.cm_bmp=self.cm_bmp
        self.refresh()

    def mouse_pos_to_pixels(self,pos):
       px,py=pos
       px-=self.x0
       py-=self.y0
       px/=self.PPP
       py/=self.PPP
       px=int(px)
       py=int(py)
       if px<0:px=0
       if py<0:py=0
       MX,MY=self.data.shape
       if px>MX-1:px=MX-1
       if py>MY-1:py=MY-1
       return px,py
    def OnMouseEvent(self, event):
        """ This function manages mouse events """
        if event:
            pixpos=self.mouse_pos_to_pixels( event.GetPositionTuple() )
            rpx,rpy=self.p0[0]+pixpos[0], self.p0[1]+pixpos[1]
            val=self.data[rpy][rpx]
            if self.update_pixinfo: self.update_pixinfo("[%d,%d]\n%.3f"%(rpx,rpy,val))
            # set mouse cursor
            self.SetCursor(wx.StockCursor(wx.CURSOR_ARROW))
            # get device context of canvas
            dc= wx.ClientDC(self)
            
            # Set logical function to XOR for rubberbanding
            dc.SetLogicalFunction(wx.XOR)
            
            # Set dc brush and pen
            # Here I set brush and pen to white and grey respectively
            # You can set it to your own choices
            
            # The brush setting is not really needed since we
            # dont do any filling of the dc. It is set just for 
            # the sake of completion.

            wbrush = wx.Brush(wx.Colour(255,255,255), wx.SOLID)
            wpen = wx.Pen(wx.Colour(200, 200, 200), 1, wx.SOLID)
            dc.SetBrush(wbrush)
            dc.SetPen(wpen)

            
        if event.LeftDown():
 
           # Left mouse button down, change cursor to
           # something else to denote event capture
           self.m_stpoint = event.GetPosition()
           cur = wx.StockCursor(wx.CURSOR_CROSS)  
           self.SetCursor(cur)
        
           # invalidate current canvas
           self.Refresh()
           # cache current position
           self.m_savepoint = self.m_stpoint
           self._selected = False
           self._leftclicked = True

        elif event.Dragging():   
           
            # User is dragging the mouse, check if
            # left button is down
            
            if self._leftclicked:

                # reset dc bounding box
                dc.ResetBoundingBox()
                dc.BeginDrawing()
                w = (self.m_savepoint.x - self.m_stpoint.x)
                h = (self.m_savepoint.y - self.m_stpoint.y)
                
                # To erase previous rectangle
                dc.DrawRectangle(self.m_stpoint.x, self.m_stpoint.y, w, h)
                
                # Draw new rectangle
                self.m_endpoint =  event.GetPosition()
                
                w = (self.m_endpoint.x - self.m_stpoint.x)
                h = (self.m_endpoint.y - self.m_stpoint.y)
                
                # Set clipping region to rectangle corners
                dc.SetClippingRegion(self.m_stpoint.x, self.m_stpoint.y, w,h)
                dc.DrawRectangle(self.m_stpoint.x, self.m_stpoint.y, w, h) 
                dc.EndDrawing()
               
                self.m_savepoint = self.m_endpoint # cache current endpoint

        elif event.LeftUp():

            # User released left button, change cursor back
            self.SetCursor(wx.StockCursor(wx.CURSOR_ARROW))       
            self._selected = True  #selection is done
            self._leftclicked = False # end of clicking  
            pixpos1=self.mouse_pos_to_pixels( (self.m_stpoint.x,self.m_stpoint.y))
            pixpos2=self.mouse_pos_to_pixels( (self.m_savepoint.x,self.m_savepoint.y))
            cx=(pixpos1[0]+pixpos2[0])/2
            cy=(pixpos1[1]+pixpos2[1])/2
            w=abs(pixpos1[0]-pixpos2[0])
            h=abs(pixpos1[1]-pixpos2[1])
            while 1:
              if w*self.PPP > self.matrix_size[0] or h*self.PPP > self.matrix_size[1] or self.PPP>self.MAX_PPP:
                self.PPP/=2
                break
              else:
                self.PPP*=2

            self.p0[0]=cx-w/2
            self.p0[1]=cy-h/2
#            print "p0",self.p0," -> ",

            if self.p0[0]<0: self.p0[0]=0
            if self.p0[1]<0: self.p0[1]=0
#            print self.p0

            self.update_zoom()
            self.Refresh()
#            print cx,cy,w,h
    def cmaps_list(names=None):
      """display all colormaps included in the names list. If names is None, all
         defined colormaps will be shown."""
      matplotlib.rc('text', usetex=False)
      # get list of all colormap names
      # this only obtains names of built-in colormaps:
      maps = [m for m in cm.datad if not m.endswith("_r")]
      # use undocumented cmap_d dictionary instead
      #maps = [m for m in cm.cmap_d if not m.endswith("_r")]
      maps.sort()
      return maps
#      for m in maps:
#        print m
#     plt.imshow(a,aspect='auto',cmap=cm.get_cmap(m),origin="lower")

    def refresh(self):
#        pycallgraph.start_trace()
        self.make_bitmap()
        self.Refresh()
#        pycallgraph.make_dot_graph('test.png')
    def OnPaint(self, evt):
        dc = wx.PaintDC(self)
#        gc = wx.GraphicsContext.Create(dc)
        #gc.DrawBitmap(self.bmp, 00, 00, self.bmp.Width,self.bmp.Height)
        dc.DrawBitmap(self.bmp2, self.x0, self.y0, False)
        cm_offset=550
        wbrush = wx.Brush(wx.Colour(255,255,255), wx.TRANSPARENT)
#        dc.DrawRectangle(self.x0-1, self.y0+cm_offset-1,self.cm_bmp.GetSize()[0]+2,self.cm_bmp.GetSize()[1]+2) 
#        dc.DrawBitmap(self.cm_bmp, self.x0, self.y0+cm_offset, False)


        gc=wx.Colour(100,100,100)
        wpen = wx.Pen(gc, 1, wx.SOLID)
        dc.SetBrush(wbrush)
        dc.SetPen(wpen)
        dc.BeginDrawing()


        # To erase previous rectangle
        ls=512
        if self.PPP>=16:
          ls=self.PPP
        for i in range(0,512+1,ls):
            dc.DrawLine(self.x0+i,self.y0+0,self.x0+i,    self.y0+512+1)
            dc.DrawLine(self.x0+0,self.y0+i,self.x0+512+1,self.y0+i)

        for i in range(0,8):
          lbl="%d"%(self.p0[1]+256*i/4/self.PPP)
          w,h = dc.GetTextExtent(lbl)
          dc.DrawText( lbl, self.x0-2-w,                   self.y0+(i)*64+self.PPP/2-h/2) 
          dc.DrawText( lbl, self.x0+512+2,                 self.y0+(i)*64+self.PPP/2-h/2) 

          lbl="%d"%(self.p0[0]+256*i/4/self.PPP)
          w,h = dc.GetTextExtent(lbl)
          dc.DrawText( lbl, self.x0+(i)*64+self.PPP/2-w/2, self.y0-2-h) 
          dc.DrawText( lbl, self.x0+(i)*64+self.PPP/2-w/2, self.y0+514) 

        dc.EndDrawing()
    def OnMouseWheel(self,event):
      if event.AltDown():
          dx=event.GetWheelRotation()/event.GetWheelDelta() 
          self.p0[0]+=dx
          self.update_zoom()
          self.refresh()
      elif event.ControlDown():
          dy=event.GetWheelRotation()/event.GetWheelDelta() 
          self.p0[1]+=dy
          self.update_zoom()
          self.refresh()
      else:
        if event.GetWheelRotation() > 0:
          pixpos=self.mouse_pos_to_pixels( event.GetPositionTuple() )
          self.zoom_at_point(pixpos,ppp_scale=2.0)
          self.update_zoom()
          self.refresh()
        elif event.GetWheelRotation() < 0:
          pixpos=self.mouse_pos_to_pixels( event.GetPositionTuple() )
          self.zoom_at_point(pixpos,ppp_scale=0.5)
          self.update_zoom()
          self.refresh()
    def set_max(self,mmax):
        self.mmax=float(mmax)
        self.refresh()
    def set_min(self,mmin):
#        print mmin
        self.mmin=float(mmin)
        self.refresh()
    def save(self,fname):
        print "PNG->",fname
        image = wx.ImageFromBitmap(self.bmp)
        image = image.Scale(1024, 1024, wx.IMAGE_QUALITY_NORMAL)
        bmp = wx.BitmapFromImage(image)
        bmp.SaveFile(fname,wx.BITMAP_TYPE_PNG)

        fhist=fname[:-4]+"_hst.dat"
        fgnu=fname[:-4]+"_hst.gnu"
        fpng=fname[:-4]+"_hst.png"
        print "PNG->",fhist
        counts,bins= self.hst
        f=open(fhist,"w")
        for i in range(counts.shape[0]):
          f.write("%.4e %.3e\n"%(bins[i],counts[i]))
          f.write("%.4e %.3e\n"%(bins[i+1],counts[i]))
        f.close()
        f=open(fgnu,"w")
        f.write("set terminal png\n")
        f.write("set output '%s'\n"%fpng)
        f.write("set grid\n")
        f.write("set xlabel 'X'\n")
        f.write("set ylabel 'Counts'\n")
        f.write("plot '%s' w l t ''\n"%fhist)
        f.close()
        os.system("gnuplot %s"%fgnu)


    def make_bitmap(self):
#        X,Y=self.data.shape
#        PPP=2
#        arr = np.empty((X*PPP+1,Y*PPP+1, 4), np.uint8) #makeByteArray( (DIM,DIM, 4) )
#        fs=self.mmax-self.mmin
        # just some indexes to keep track of which byte is which
#        R, G, B, A = range(4)
        # initialize all pixel values to the values passed in
#        arr[:,:,A] = wx.ALPHA_OPAQUE

        color_norm  = colors.Normalize(vmin=self.mmin, vmax=self.mmax)
        transform = cm.ScalarMappable(norm=color_norm, cmap=self.cm)
        cdata=transform.to_rgba(self.data,bytes=True)
        self.bmp = wx.BitmapFromBufferRGBA(cdata.shape[0],cdata.shape[1], cdata)
        self.hst=np.histogram(self.data, bins=64, range=(self.mmin,self.mmax))
        counts,bins= self.hst

        if self.hst_panel!=None:
          self.hst_panel.hst=self.hst
          self.hst_panel.Refresh()
#          print self.mmin,self.mmax, bins[0],bins[1],bins[100]
        centers=np.zeros((counts.shape[0]))
        for i in range(counts.shape[0]):
          centers[i]=(bins[i]+bins[i+1])/2
        
        self.update_zoom()
        avr=np.average(centers, weights=counts)
        rms=0.0
        for i in range(counts.shape[0]):
          rms+=pow(centers[i]-avr,2.0)*counts[i]
        N=np.sum(counts)
        rms=sqrt(rms/N)
        
        if self.update_stats!=None:
           msg="Zoom:%d"%self.PPP
           msg+="\nAVR:%.3f"%(avr)
           msg+="\nRMS:%.3f"%(rms)
           msg+="\nPoints:%d"%(N)
           D=256*256-np.count_nonzero(self.data) 
           msg+="\nDead:%d"%(D)
           self.update_stats(msg)


#for v in np.arange(-10,10,0.1):
# print v,plt.get_cmap('jet')(v)
#        print arr[0:5,0:5]

    def zoom_at_point(self,pixpos,ppp_scale):
        self.PPP*=ppp_scale
        if self.PPP>self.MAX_PPP:
           self.PPP=self.MAX_PPP
        if self.PPP<2:
           self.PPP=2
        self.PPP=int(self.PPP)
        self.update_zoom()
        self.Refresh()
    def update_zoom(self):
        ptpx,ptpy=self.matrix_size[0]/self.PPP,self.matrix_size[1]/self.PPP
#        print ptpx,ptpy
        MX,MY=self.data.shape
        if self.p0[0]+ptpx>=MX:self.p0[0]=MX-ptpx
        if self.p0[1]+ptpy>=MY:self.p0[1]=MY-ptpy
        if self.p0[0]<0:self.p0[0]=0
        if self.p0[1]<0:self.p0[1]=0

        # finally, use the array to create a bitmap
        r=wx.Rect(self.p0[0],self.p0[1],ptpx,ptpy)
        
        image = wx.ImageFromBitmap(self.bmp.GetSubBitmap( r ) )
        image = image.Scale(self.matrix_size[0], self.matrix_size[1], wx.IMAGE_QUALITY_NORMAL)
        self.bmp2 = wx.BitmapFromImage(image)
#        print self.bmp.Width,self.bmp.Size
           
        
ico = PyEmbeddedImage(
    "iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAABHNCSVQICAgIfAhkiAAAAAlw"
    "SFlzAAAN1wAADdcBQiibeAAAABl0RVh0U29mdHdhcmUAd3d3Lmlua3NjYXBlLm9yZ5vuPBoA"
    "AAC6SURBVDiNpZMhDsMwDEWfpx6krJcYLC4f6zHG0sLdImx8eGcIH+tNPNBZjdKmibSPYlv5"
    "+v75EVWlBJG5BcaDkW+Kt1eMwA34RL0OoJaA3+VXVA8rgUhOnsHDlB02GXmGriTLVkjlGYaE"
    "LK1DrQf+oBeofQVVt5Ax4lKpIIu/CWyF1CCifjgjEIWWkxzce96PK33U8up02QgKf0Fmmdhy"
    "0gFPdTrZvNYDy8kubLUE5tEumTU58NE5JDVfI9osXTfTOocAAAAASUVORK5CYII=")

class MyForm(wx.Frame):
  def __init__(self,fname):
     wx.Frame.__init__(self, None, wx.ID_ANY, "openPIXel", size=(920,615),style= wx.SYSTEM_MENU | wx.CAPTION | wx.CLOSE_BOX)
#     self.Bind(wx.EVT_CHAR, self.OnChar)
#     sizer = wx.BoxSizer(wx.VERTICAL)
     self.tp=TestPanel(self,fname)

     #sizer.Add(self.tp)
     #self.SetMinSize((600,600))
     #self.SetSizer(sizer, wx.EXPAND)
     #self.Show()
#     bsizer.Add(t, 0, wx.TOP|wx.LEFT, 10)

#     border = wx.BoxSizer()
#     border.Add(bsizer, 1, wx.EXPAND|wx.ALL, 25)
#     self.SetSizer(border)

     mmin=np.amin(self.tp.data)
     mmax=np.amax(self.tp.data)
     spread=(mmax-mmin)*100
     inc=float(int(spread))/10000


     topsizer= wx.BoxSizer(wx.HORIZONTAL) # left controls, right image output
     ctrlsizer= wx.BoxSizer(wx.VERTICAL)
     val_sizer= wx.BoxSizer(wx.HORIZONTAL)

     box_min = wx.StaticBox(self, -1, "Min Value")
     sizer_min = wx.StaticBoxSizer(box_min, wx.VERTICAL)
        
     self.cb_min = wx.CheckBox(self, -1, "auto")#, (65, 60), (150, 20), wx.NO_BORDER)
     self.cb_min.SetValue(False)
     sizer_min.Add(self.cb_min)
#     sizer_min.AddSpacer(10)
     self.spin_min = FS.FloatSpin(self, -1, min_val=mmin, max_val=mmax,
                                       increment=inc, value=mmin, size=(150,-1),agwStyle=FS.FS_LEFT)
     self.spin_min.SetFormat("%f")
     self.spin_min.SetDigits(3)
     self.spin_min.Bind(FS.EVT_FLOATSPIN, self.OnMinSpin)

     sizer_min.Add(self.spin_min)
#     sizer_min.AddSpacer(15)
     val_sizer.Add(sizer_min, 0, wx.ALL, 2)


     box_max = wx.StaticBox(self, -1, "Max Value")
     sizer_max = wx.StaticBoxSizer(box_max, wx.VERTICAL)
        
     self.cb_max = wx.CheckBox(self, -1, "auto")#, (65, 60), (150, 20), wx.NO_BORDER)
     self.cb_max.SetValue(False)
     sizer_max.Add(self.cb_max)
#     sizer_max.AddSpacer(10)
     self.spin_max = FS.FloatSpin(self, -1, min_val=mmin, max_val=mmax,
                                       increment=inc, value=mmax, size=(150,-1), agwStyle=FS.FS_LEFT)
     self.spin_max.SetFormat("%f")
     self.spin_max.SetDigits(3)
     self.spin_max.Bind(FS.EVT_FLOATSPIN, self.OnMaxSpin)

     sizer_max.Add(self.spin_max)
#     sizer_max.AddSpacer(15)
     val_sizer.Add(sizer_max, 0, wx.ALL, 2)
     ctrlsizer.Add(val_sizer, 0, wx.ALL, 2)

     txt_sizer= wx.BoxSizer(wx.HORIZONTAL)
     
     txt2_sizer= wx.BoxSizer(wx.VERTICAL)
     pixinfo_box = wx.StaticBox(self, -1, "Pixel info")
     pixinfo_sizer = wx.StaticBoxSizer(pixinfo_box, wx.VERTICAL)
     self.pix_info = wx.TextCtrl(self, style=wx.TE_MULTILINE |  wx.TE_READONLY,size=(150,40))
     pixinfo_sizer.Add(self.pix_info)
     self.update_pixinfo("-")
     txt2_sizer.Add(pixinfo_sizer, 0, wx.ALL, 2)
     self.tp.update_pixinfo=self.update_pixinfo

     color_box = wx.StaticBox(self, -1, "Colors")
     color_sizer = wx.StaticBoxSizer(color_box, wx.VERTICAL)
     sampleList = self.tp. cmaps_list()
     # This combobox is created with a preset list of values.
     self.color_combo = wx.ComboBox(self, 500, "jet", (90, 50), 
                         (150, -1), sampleList,
                         wx.CB_DROPDOWN
                         | wx.TE_PROCESS_ENTER
                         | wx.CB_SORT
                         )
     self.color_combo.Bind(wx.EVT_COMBOBOX, self.EvtColorComboBox)
     color_sizer.Add(self.color_combo)
     txt2_sizer.Add(color_sizer, 0, wx.ALL, 2)
     txt_sizer.Add(txt2_sizer, 0, wx.ALL, 0)



     stats_box = wx.StaticBox(self, -1, "Stats")
     stats_sizer = wx.StaticBoxSizer(stats_box, wx.VERTICAL)
     self.stats_info = wx.TextCtrl(self, style=wx.TE_MULTILINE |  wx.TE_READONLY,size=(150,89))
     stats_sizer.Add(self.stats_info)
     self.tp.update_stats=self.update_stats
     self.update_stats("-")
     txt_sizer.Add(stats_sizer, 0, wx.ALL, 2)

     ctrlsizer.Add(txt_sizer, 0, wx.ALL, 2)




     hist_box = wx.StaticBox(self, -1, "Histogram")
     hist_sizer = wx.StaticBoxSizer(hist_box, wx.VERTICAL)
     self.hist_panel= HistPanel(self, size=(317,330))
     hist_sizer.Add(self.hist_panel)
     ctrlsizer.Add(hist_sizer, 0, wx.ALL, 2)

     self.tp.hst_panel= self.hist_panel

     self.tp.change_cm('jet')#refresh()
      
     topsizer.Add(ctrlsizer, 0, wx.ALL, 2)
     topsizer.Add(self.tp, 1, wx.EXPAND)
    
     self.SetSizer(topsizer)
     topsizer.Layout()
        
     # forward the slider change events to the image window
#     sliderred.Bind(wx.EVT_SCROLL, self.OnScrollRed)
#     slidergreen.Bind(wx.EVT_SCROLL, self.OnScrollGreen)
#     image = wx.Image('logo.png', wx.BITMAP_TYPE_PNG).ConvertToBitmap()
     
#     icon = wx.EmptyIcon()
     
#     icon.CopyFromBitmap(ico)
     self.SetIcon(ico.GetIcon()) 


     
     menubar = wx.MenuBar()

     fileMenu = wx.Menu()
     fileMenu = wx.Menu()
     file_open=fileMenu.Append(wx.ID_OPEN, '&Open')
     file_save=fileMenu.Append(wx.ID_SAVE, '&Save plots')
     fileMenu.AppendSeparator()
     file_exit = fileMenu.Append(wx.ID_EXIT, '&Quit')

     self.Bind(wx.EVT_MENU, self.OnOpen, file_open)
     self.Bind(wx.EVT_MENU, self.OnFileSaveAs, file_save)
     self.Bind(wx.EVT_MENU, self.OnQuit, file_exit)
     menubar.Append(fileMenu, '&File')
     self.SetMenuBar(menubar)

     viewMenu = wx.Menu()
     self.shst = viewMenu.Append(wx.ID_ANY, 'Show statubar', 
            'Show Statusbar', kind=wx.ITEM_CHECK)
     self.statusbar = self.CreateStatusBar()
     self.statusbar.SetStatusText('File %s loaded.'%fname)
     viewMenu.Check(self.shst.GetId(), True)
     self.Bind(wx.EVT_MENU, self.ToggleStatusBar, self.shst)
     menubar.Append(viewMenu, '&View')
     
     helpMenu = wx.Menu()
     file_about=helpMenu.Append(wx.ID_ABOUT, "&About", "Display information about the program")
     self.Bind(wx.EVT_MENU, self.OnHelpAbout, file_about)
     menubar.Append(helpMenu, '&Help')
        
     self.Centre()
     self.Show(True)
     
  def ToggleStatusBar(self, e):
    if self.shst.IsChecked():
        self.statusbar.Show()
    else:
        self.statusbar.Hide()
  def OnOpen(self, e):
        """ File|Open event - Open dialog box. """
        dirName=''
        fileName=''
        dlg = wx.FileDialog(self, "Open", dirName, fileName,
                           "Dat Files (*.dat)|*.dat|Text Files (*.txt)|*.txt|All Files|*.*", wx.OPEN)
        if (dlg.ShowModal() == wx.ID_OK):
            self.fileName = dlg.GetFilename()
            self.dirName = dlg.GetDirectory()

            ### - this will read in Unicode files (since I'm using Unicode wxPython
            #if self.rtb.LoadFile(os.path.join(self.dirName, self.fileName)):
            #    self.SetStatusText("Opened file: " + str(self.rtb.GetLastPosition()) + 
            #                       " characters.", SB_INFO)
            #    self.ShowPos()
            #else:
            #    self.SetStatusText("Error in opening file.", SB_INFO)

            ### - but we want just plain ASCII files, so:
            try:
                f = file(os.path.join(self.dirName, self.fileName), 'r')
                self.rtb.SetValue(f.read())
                self.SetTitle(APP_NAME + " - [" + self.fileName + "]")
                self.SetStatusText("Opened file: " + str(self.rtb.GetLastPosition()) +
                                   " characters.", SB_INFO)
                self.ShowPos()
                f.close()
            except:
                self.PushStatusText("Error in opening file.", SB_INFO)
        dlg.Destroy()
  def OnHelpAbout(self, e):
        """ Help|About event """
        title = self.GetTitle()
        d = wx.MessageDialog(self, "OpenPixel v0.1\nAuthor: Szymon Kulis\n2013 CERN","About" , wx.ICON_INFORMATION | wx.OK)
        d.ShowModal()
        d.Destroy()
  def OnFileSaveAs(self, e):
        """ File|SaveAs event - Prompt for File Name. """
        ret = False
        dirName='.'
        fileName='plots'
        dlg = wx.FileDialog(self, "Save As", dirName, fileName,
                           "PNG Files (*.png)|*.png|All Files|*.*", wx.SAVE)
        if (dlg.ShowModal() == wx.ID_OK):
            fileName = dlg.GetFilename()
            dirName = dlg.GetDirectory()
            if fileName[-4:].lower()!='.png':
              fileName+='.png'
            self.tp.save(dirName+"/"+fileName)
            ret = True
        dlg.Destroy()
        return ret
  def OnQuit(self, e):
        self.Close()
  def EvtColorComboBox(self,event):
    cb = event.GetEventObject()
#    data = cb.GetClientData()
    self.tp.change_cm(event.GetString())

  def update_pixinfo(self,msg):
     self.pix_info.SetValue(msg)

  def update_stats(self,msg):

     self.stats_info.SetValue(msg)


  def OnMaxSpin(self, event):
        if self.spin_max.GetValue()<=self.spin_min.GetValue():
          self.spin_max.SetValue(self.spin_min.GetValue())
        self.tp.set_max(self.spin_max.GetValue())
        self.Refresh()

  def OnMinSpin(self, event):
#        print self.spin_min.GetValue(), self.spin_max.GetValue(),"->",
        if self.spin_min.GetValue()>=self.spin_max.GetValue():
          self.spin_min.SetValue(self.spin_max.GetValue())
        self.tp.set_min(self.spin_min.GetValue())
        self.Refresh()
#        print self.spin_min.GetValue(), self.spin_max.GetValue()


  def OnChar(self, evt):
        keycode =evt.GetKeyCode()
        print "GRID!"
        if keycode==ord('g'):
          self.grid=~self.grid
          self.refresh()
 




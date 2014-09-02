from PySide.QtCore import *
from PySide.QtGui import *
from PySide import QtCore, QtGui

import matplotlib.colors as colors
import matplotlib.cm as cm
import matplotlib
from math import copysign
from kutils import n2h
import sys

UPDATE_RECT=0
UPDATE_ROWS=1
UPDATE_COLUMNS=2
UPDATE_RECT_OUT=3

class ColorMap:
    def __init__(self,parent=None,min=0.0,max=0.0):
        self.parent=parent
        self.min=min
        self.max=max
        self.hmin=min
        self.hmax=max
        self.changeColorMap('gray')#RdYlBu')

    def setHMax(self,max):
        self.max=max
        self.hmax=max

    def setHMin(self,min):
        self.min=min
        self.hmin=min

    def generateMenu(self):
        cmMenu=QtGui.QMenu('Color',None)
        for c in self.colorMapList():
            cmMenu.addAction(QtGui.QAction(c, cmMenu,triggered=lambda c=c: self.changeColorMap(c) ))
        return cmMenu



    def colorMapList(self):
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

    def changeColorMap(self,new_cm):
        #print "New colormap", new_cm
        self.cm=cm.get_cmap(new_cm)
        self.cm.set_over( 'blue')
        self.cm.set_under('red')
        self.generateColorBar()
        if self.parent:
            self.parent._regenerate_bitmap()


    def generateColorBar(self):
        CBARLEL=256 #self.matrix_size[0]
        data=np.zeros((1,CBARLEL))
        for i in range(CBARLEL):
          data[0][i]=float(i)/CBARLEL
          #print data[0][]
        transform = cm.ScalarMappable(cmap=self.cm)
        cdata=transform.to_rgba(data,bytes=True)
        img=QImage(cdata,cdata.shape[0],cdata.shape[1], cdata.shape[0]*4, QImage.Format_ARGB32 )
        self.ColorBar = img.copy()
        #image = wx.ImageFromBitmap(self.cm_bmp)
        #image = image.Scale(CBARLEL,10, wx.IMAGE_QUALITY_NORMAL)
        #self.cm_bmp = wx.BitmapFromImage(image)
        #self.process()

    def processData(self,data):
        #self.min=np.min(data)
        #self.max=np.max(data)
        #print self.min, self.max,np.min(data),np.max(data)
        color_norm  = colors.Normalize(vmin=self.min, vmax=self.max)
        color_norm.clip=False
        transform = cm.ScalarMappable(norm=color_norm, cmap=self.cm)
        cdata=transform.to_rgba(np.transpose(data),bytes=True)
        #print self.min, self.max
        image=QImage(cdata,cdata.shape[0],cdata.shape[1], cdata.shape[0]*4, QImage.Format_ARGB32 )
        return image.copy()



class PixBitmap(QWidget):
    def __init__(self,parent=None):
        self.parent=parent
        super(PixBitmap, self).__init__(parent)
        self.setMinimumSize(552, 552)
        self.data=None
        self.setFocusPolicy(Qt.WheelFocus)
        self.zooming=False
        self.zoom_p1=QPoint(0,0) #in screen coordinates
        self.zoom_p2=QPoint(0,0)
        self.selecting=False
        self.select_p1=QPoint(0,0) #in screen coordinates
        self.select_p2=QPoint(0,0)
        self.force_square=False

        self.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        #self.connect(self, QtCore.SIGNAL('customContextMenuRequested(const QPoint&)'), self.on_context_menu)

        self._popmenu_pos=[0,0]
        self.scale=0.2
        self.visibleRegion=QRect (0,0,0,0)
    def _regenerate_bitmap(self):
        if self.data!=None:
            self.bmp= self.cm.processData(self.data)

    def action_zoom(self,delta):
        self. zoom_delta_pos(delta,self._popmenu_pos)


    def action_zoom_fit(self):
        self._zoom_update_ranges(QPoint(0,0),QPoint(self.data.shape[0]-1, self.data.shape[1]-1))

    def action_square(self):
        self.force_square=not self.force_square
        self.update()




    def mouseMoveEvent(self, event):
        if self.data==None: return
        if self.zooming:
            self.zoom_p2=event.pos()
        if self.selecting:
            self.select_p2=event.pos()
        self.update()

    def _point_to_pixel(self,p):
        x=p.x()
        x=int((x-self.p0.x())/self.pixelsPerPoint.width())+ self.visibleRegion.left()
        x=max ((0,x))
        x=min ((self.data.shape[0]-1,x))

        y=p.y()
        y=int((y-self.p0.y())/self.pixelsPerPoint.height())+ self.visibleRegion.top()
        y=max ((0,y))
        y=min ((self.data.shape[1]-1,y))
        return QPoint(x,y)


    def zoom_delta_pos(self,delta,pos):
        #real zooming happens here
        delta=copysign(1,delta)

        pix=self._point_to_pixel(pos)
        X=self.visibleRegion.width()
        Y=self.visibleRegion.height()
        NX=float(X)*(1-delta*self.scale)
        NY=float(Y)*(1-delta*self.scale)

        NX=int(max( (NX,2)))
        NY=int(max( (NY,2)))

        _x1=int(pix.x()-NX/2)
        _x2=int(pix.x()+NX/2)+1
        _y1=int(pix.y()-NY/2)
        _y2=int(pix.y()+NY/2)+1

        _x1=max((_x1,0))
        _y1=max((_y1,0))
        _x2=min((_x2,self.data.shape[0]-1))
        _y2=min((_y2,self.data.shape[1]-1))

        if _x1==0 and (_x2-_x1<NX): _x2=_x1+NX
        if _y1==0 and (_y2-_y1<NY): _y2=_y1+NY

        if _x2==self.data.shape[0]-1 and (_x2-_x1<NX): _x1=_x2-NX
        if _y2==self.data.shape[1]-1 and (_y2-_y1<NY): _y1=_y2-NY

        self._zoom_update_ranges(QPoint(_x1,_y1),QPoint(_x2,_y2))

    def _zoom_update_ranges(self,p1,p2):

        x1=min((p1.x(),p2.x()))
        x2=max((p1.x(),p2.x()))

        y1=min((p1.y(),p2.y()))
        y2=max((p1.y(),p2.y()))
        #print x1,y1,x2,y2
        x1=max((x1,0))
        y1=max((y1,0))
        x2=min((x2,self.data.shape[0]-1))
        y2=min((y2,self.data.shape[1]-1))

        #print x1,y1,x2,y2
        self.visibleRegion.setTopLeft(QPoint(x1,y1))
        self.visibleRegion.setBottomRight(QPoint(x2,y2))
        #print self.visibleRegion
        self.update()

        self.parent._update_sliders()

    def wheelEvent (self,event):
        if self.data==None: return
        self.zooming=False
        self.selecting=False
        self.zoom_delta_pos(event.delta(),event.pos())

    def mouseReleaseEvent(self, event):
        if self.data==None: return
        if event.button() == Qt.LeftButton and self.zooming:
            self.zooming=False
            p1=self._point_to_pixel( self.zoom_p1 )
            p2=self._point_to_pixel( self.zoom_p2 )
            self._zoom_update_ranges(p1,p2)
            event.accept()
        elif event.button() == Qt.RightButton and self.selecting:
            self.selecting=False
            #self._zoom_update_ranges(p1,p2)
            self.on_select(event.pos())
            event.accept()

        else:
            QWidget.mousePressEvent(self, event)

    def on_select(self, point):
        if self.data==None: return
        p1=self._point_to_pixel( self.select_p1)
        p2=self._point_to_pixel( self.select_p2)

        # create context menu
        self.popMenu = QtGui.QMenu(self)
        if p1==p2:
           pixtxt="[%d,%d]"%(p1.x(),p1.y())
           pixval="Value:%.3f"%(self.data[p1.x()][p1.y()])
        else:
           pixtxt="[%d,%d] - [%d,%d] (%dx%d)"%(p1.x(),p1.y(),p2.x(),p2.y(), abs(p1.x()-p2.x())+1,abs(p2.y()-p1.y())+1)
           x_min=min((p1.x(),p2.x()))
           x_max=max((p1.x(),p2.x()))+1
           y_min=min((p1.y(),p2.y()))
           y_max=max((p1.y(),p2.y()))+1
           #print x_min,x_max,y_min,y_max
           a=np.mean(self.data[x_min:x_max,y_min:y_max])
           s=np.std(self.data[x_min:x_max,y_min:y_max])
           pixval="Mean:%.3f RMS:%.3f"%(a,s)

        name=self.popMenu.addAction(pixtxt)
        name.setEnabled(False)

        val=self.popMenu.addAction(pixval)
        val.setEnabled(False)

        self.popMenu.addSeparator()
        zoomMenu = QtGui.QMenu("Zoom")
        self.popMenu.addMenu(zoomMenu)
        zoomMenu.addAction(QtGui.QAction('Zoom fit', self,triggered=self.action_zoom_fit))
        zoomMenu.addAction(QtGui.QAction('Zoom in',  self,triggered=lambda :self.action_zoom(1)))
        zoomMenu.addAction(QtGui.QAction('Zoom out', self,triggered=lambda :self.action_zoom(-1)))

        self.popMenu.addSeparator()
        self.popMenu.addMenu(self.cm.generateMenu())
        self.popMenu.addSeparator()

        maskMenu = QtGui.QMenu("Mask")
        self.popMenu.addMenu(maskMenu)
        maskMenu.addAction(QtGui.QAction('Selected', self,triggered=lambda:self.action_mask(UPDATE_RECT)))
        maskMenu.addAction(QtGui.QAction('Columns',  self,triggered=lambda:self.action_mask(UPDATE_COLUMNS)))
        maskMenu.addAction(QtGui.QAction('Rows', self,triggered=lambda:self.action_mask(UPDATE_ROWS)))
        maskMenu.addAction(QtGui.QAction('Outside', self,triggered=lambda:self.action_mask(UPDATE_RECT_OUT)))

        self._popmenu_pos=point
        self.popMenu.exec_(self.mapToGlobal(point))


    def action_mask(self,type):
        p1=self._point_to_pixel( self.select_p1)
        p2=self._point_to_pixel( self.select_p2)
        x_min=min((p1.x(),p2.x()))
        x_max=max((p1.x(),p2.x()))+1
        y_min=min((p1.y(),p2.y()))
        y_max=max((p1.y(),p2.y()))+1
        i=0
        if type==UPDATE_RECT:
            for x in range(x_min,x_max):
                for y in range(y_min,y_max):
                    self.parent.tpx.setPixelMask(x,y,1)
                    i+=1
        elif type==UPDATE_COLUMNS:
            for x in range(x_min,x_max):
                for y in range(0,256):
                    self.parent.tpx.setPixelMask(x,y,1)
                    i+=1
        elif type==UPDATE_ROWS:
            for x in range(0,256):
                for y in range(y_min,y_max):
                    self.parent.tpx.setPixelMask(x,y,1)
                    i+=1
        elif type==UPDATE_RECT_OUT:
            for x in range(0,256):
                for y in range(0,256):
                    if x>=x_min and x<x_max and y>=y_min and y<y_max:
                        pass
                        i+=1
                    else:
                        self.parent.tpx.setPixelMask(x,y,1)


        print "Masking %d pixels"%i
        self.parent.tpx.setPixelConfig()
        self.update()

    def mousePressEvent(self, event):
        if self.data==None: return
        #print event
        if event.button() == Qt.LeftButton:
            #self.moveSlider(event.x())
            self.zooming=True
            self.zoom_p1=event.pos()
            self.zoom_p2=self.zoom_p1
            event.accept()
        elif event.button() == Qt.RightButton:
            #self.moveSlider(event.x())
            self.selecting=True
            self.select_p1=event.pos()
            self.select_p2=self.select_p1
            event.accept()
        else:
            QWidget.mousePressEvent(self, event)

    def paintEvent(self, e):
        if self.data!=None:
            qp = QPainter()
            qp.begin(self)
            self.drawWidget(qp)
            qp.end()

    def setColorMap(self,cm):
        self.cm=cm

    def setData(self,data):
        self.data=data


        firstPixel=QPoint(0,0)
        lastPixel=QPoint(data.shape[0]-1,data.shape[1]-1) #this is after one pixel
        #self.visibleRegion=QRect ( firstPixel,lastPixel )
        self._regenerate_bitmap()
        self._zoom_update_ranges(firstPixel,lastPixel)
        #

    def drawWidget(self, qp):
        margin=5

        font = QFont('Serif', 7, QFont.Light)
        qp.setFont(font)
        metrics = qp.fontMetrics()
        MAX_TEXT_LEN=metrics.width("256")

        size = self.size() # size in pixels of widget we can plot on

        dataPlotSize=QSize(size)

        # size of the field which is ocupied by data plot
        dataPlotSize.setHeight( dataPlotSize.height() -1 -2*margin - MAX_TEXT_LEN )
        dataPlotSize.setWidth( dataPlotSize.width() -1 -2*margin - MAX_TEXT_LEN )

        # number of screen pixels per one data point
        self.pixelsPerPoint=QSizeF( float(dataPlotSize.width())/self.visibleRegion.width(), float(dataPlotSize.height()) /self.visibleRegion.height())
        #print self.pixelsPerPoint
        #dataPlotSize.setHeight( self.pixelsPerPoint.height() * self.visibleRegion.height() )
        #dataPlotSize.setWidth( self.pixelsPerPoint.width() * self.visibleRegion.width()  )

        #cooridanate of fist pixel of data
        self.p0=QPoint(margin + MAX_TEXT_LEN, margin + MAX_TEXT_LEN)

        #draw actual data
        bmpRegion=self.bmp.copy(self.visibleRegion)
#        bmp2=bmpRegion.scaled(bmpRegion.width()*self.pixelsPerPoint.width(), bmpRegion.height()*self.pixelsPerPoint.height())
        bmp2=bmpRegion.scaled(dataPlotSize.width(),dataPlotSize.height())

        qp.drawImage(self.p0 , bmp2 )

        # draw border
        pen = QPen(QColor(20, 20, 20), 1,  Qt.SolidLine)
        qp.setPen(pen)
        qp.setBrush(Qt.NoBrush)
        qp.drawRect(QRect(self.p0,dataPlotSize))

        # draw grid
        if self.pixelsPerPoint.width()> 8 and self.pixelsPerPoint.height()>8:
            for x in range(self.visibleRegion.width()):
                p1=self.p0+QPoint(x*self.pixelsPerPoint.width(),0)
                p2=self.p0+QPoint(x*self.pixelsPerPoint.width(),dataPlotSize.height())
                qp.drawLine(p1,p2)
            for y in range(self.visibleRegion.height()):
                p1=self.p0+QPoint(0,y*self.pixelsPerPoint.height())
                p2=self.p0+QPoint(dataPlotSize.width(),y*self.pixelsPerPoint.height())
                qp.drawLine(p1,p2)

        # draw vertical legend
        singleStrHeight = metrics.height()
        allStrHeight    = singleStrHeight*self.visibleRegion.height()
        vertLabelStep   = max( (int(allStrHeight*2.0/dataPlotSize.height()),1) )

        for y in range(0,self.visibleRegion.height(),vertLabelStep):
            txt=str(self.visibleRegion.top()+y)
            txtw = metrics.width(txt) # text width
            txtp = QPoint( self.p0.x() -txtw -2 , self.p0.y() +(0.5+y)*self.pixelsPerPoint.height() + singleStrHeight/2.0) #text ploting point
            qp.drawText(txtp, txt)

        # draw horizontal legend
        allStrWidth     = MAX_TEXT_LEN*self.visibleRegion.width()
        horizLabelStep  = max( (int(allStrWidth*2.0/dataPlotSize.width()),1) )

        for x in range(0,self.visibleRegion.width(),horizLabelStep):
            txt=str(x+self.visibleRegion.left())
            txtw = metrics.width(txt)
            txtp = QPoint( self.p0.x() - txtw/2 + (0.5+x)*self.pixelsPerPoint.width() , self.p0.y() - singleStrHeight/2.0) #text ploting point
            qp.drawText(txtp, txt)

        if self.zooming:
            brush=QBrush(QColor(0,0,255,128))
            qp.setBrush(brush)
            qp.drawRect( QRect(self.zoom_p1,self.zoom_p2))

        if self.selecting:
            brush=QBrush(QColor(255,0,0,128))
            qp.setBrush(brush)
            qp.drawRect( QRect(self.select_p1,self.select_p2))

import numpy as np
ZOOM_MAX=1
ZOOM_MIN=2
class PixHist(QWidget):
    def __init__(self,parent=None):
        self.parent=parent
        super(PixHist, self).__init__(parent)
        self.setMinimumSize(130, 256)
        self.data=None
        self.max_pos=-1
        self.min_pos=0
        self.zooming=0
        self.edge_min=0
        self.edge_max=1

    def mouseMoveEvent(self, event):
        if self.zooming==ZOOM_MAX:
            self.max_pos=event.y()-self.p0.y()
            if self.max_pos<self.min_pos+20:
                self.max_pos=self.min_pos+20
            if self.max_pos>self.colorBoxSize.height():
                self.max_pos=self.colorBoxSize.height()
        elif self.zooming==ZOOM_MIN:
            self.min_pos=event.y()-self.p0.y()
            if self.min_pos>self.max_pos-20:
                self.min_pos=self.max_pos-20

            if self.min_pos<0:
                self.min_pos=0
        self.update_cm()

    def setColorMap(self,cm):
        self.cm=cm

    def wheelEvent (self,event):
       max_dis=abs(self.max_pos - event.y() + self.p0.y())
       min_dis=abs(self.min_pos - event.y() + self.p0.y())
       delta=-copysign(1,event.delta())
       if min_dis>max_dis:
            self.max_pos+=delta
            if self.max_pos<self.min_pos+20:
                self.max_pos=self.min_pos+20
            if self.max_pos>self.colorBoxSize.height():
                self.max_pos=self.colorBoxSize.height()
       else:
            self.min_pos+=delta
            if self.min_pos>self.max_pos-20:
                self.min_pos=self.max_pos-20

            if self.min_pos<0:
                self.min_pos=0
       self.update_cm()

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            if abs(self.max_pos - event.y() + self.p0.y())<20:
                 #print "zoomin max"
                 self.zooming=ZOOM_MAX
            elif abs(self.min_pos - event.y() + self.p0.y())<20:
                 #print "zoomin min"
                 self.zooming=ZOOM_MIN

            else:
                self.zooming=0
            event.accept()
        elif event.button() == Qt.RightButton:
           popMenu = QtGui.QMenu(self)
           popMenu.addAction(QtGui.QAction('Auto', self,triggered=self.action_auto))

           popMenu.exec_(self.mapToGlobal(event.pos()))
        else:
            QWidget.mousePressEvent(self, event)

    def update_cm(self):
        mmin=float(self.min_pos)/self.colorBoxSize.height()
        mmax=float(self.max_pos)/self.colorBoxSize.height()


        _hmin=self.cm.hmin
        _hmax=self.cm.hmax
        g=_hmax-_hmin

        self.cm.min=_hmin+mmin*g
        self.cm.max=_hmin+mmax*g

        self.cm.generateColorBar()
        self.parent._regenerate_bitmap()
        self.parent.update()
        #print self.cm.min, self.cm.max


    def action_auto(self):
        self.min_pos=0
        self.max_pos=self.colorBoxSize.height()
        self.cm.setHMin(np.min(self.data))
        self.cm.setHMax(np.max(self.data))
        self.update_cm()

    def paintEvent(self, e):
        if self.data!=None:
            qp = QPainter()
            qp.begin(self)
            self.drawWidget(qp)
            qp.end()

    def setData(self,data):
        self.data=data

    def drawWidget(self, qp):
        margin=5
        font = QFont('Serif', 7, QFont.Light)
        qp.setFont(font)
        size = self.size()
        w = size.width()- margin*2
        h = size.height() - margin*2
        self.colorBoxSize=QSize(16,h)
        self.histBoxSize=QSize(100,h)
        self.p0=QPoint(margin,margin)

        BINS=200
        db=float(h)/BINS
        hist_width=64
        color_width=32
        if self.max_pos<0:
            self.max_pos=self.colorBoxSize.height()

        hist,edges=np.histogram(self.data,bins=BINS, range=(self.cm.hmin,self.cm.hmax))

        # print "min",np.min(self.data)
        # print "max",np.max(self.data)
        # print "hmin",edges[0]
        # print "hmax",edges[-1]

        MAX_BIN=max(hist[1:])
        MAX_BIN=max((MAX_BIN,1))
        #print "MAX_BIN",MAX_BIN
        metrics = qp.fontMetrics()
        MAX_TEXT_LEN=metrics.width("256")

        #draw actual data
        # draw collor bar
        bmp2=self.cm.ColorBar.scaled(self.colorBoxSize.width(),self.colorBoxSize.height())
        qp.drawImage(self.p0+QPoint(100,0) , bmp2 )



        # pen = QPen(Qt.transparent)
        # qp.setPen(pen)
        #
        MAX=100
        histImage=QImage(MAX,BINS,QImage.Format_ARGB32)
        qp2=QPainter(histImage)
        qp2.setBackground(QBrush(Qt.white))
        qp2.eraseRect(histImage.rect())
        for b in range(BINS):
            hh=MAX*hist[b]/MAX_BIN
            qp2.drawLine(MAX,b,MAX-hh,b)
        qp2.end()
        self.histImage2=histImage.scaled(self.histBoxSize.width(),self.histBoxSize.height())
        qp.drawImage(self.p0 , self.histImage2 )


        pen = QPen(QColor(0,0,0), 1,  Qt.SolidLine)
        qp.setPen(pen)
        brush=QBrush(Qt.transparent)
        qp.setBrush(brush)
        qp.drawRect(QRect(self.p0,self.histBoxSize))
        qp.drawRect(QRect(self.p0+QPoint(self.histBoxSize.width(),0),self.colorBoxSize))

        font = QFont('Serif', 9, QFont.Light)
        qp.setFont(font)
        metrics = qp.fontMetrics()

        pen = QPen(Qt.red, 3,  Qt.SolidLine)
        qp.setPen(pen)
        qp.drawLine(self.p0+QPoint(0,self.max_pos),self.p0+QPoint(self.histBoxSize.width(),self.max_pos))
        qp.drawLine(self.p0+QPoint(self.histBoxSize.width()/2,self.max_pos),self.p0+QPoint(self.histBoxSize.width(),self.histBoxSize.height()))
        qp.drawLine(self.p0+QPoint(self.histBoxSize.width(),self.histBoxSize.height()),self.p0+QPoint(self.histBoxSize.width() + bmp2.width(), self.histBoxSize.height() ))
        maxstr=n2h(self.cm.max)
        FW=metrics.width(maxstr)
        FH=metrics.height()
        pen = QPen(Qt.red, 2,  Qt.SolidLine)
        qp.setPen(pen)
        brush=QBrush(QColor.fromRgbF(1,1,1,0.8))
        qp.setBrush(brush)
        qp.drawRect(QRect(self.p0+QPoint(0,self.max_pos-2-FH) ,self.p0+QPoint(0+FW+2,self.max_pos-2) ))
        qp.drawText(self.p0+QPoint(2,self.max_pos-4), maxstr)


        pen = QPen(Qt.blue, 3,  Qt.SolidLine)
        qp.setPen(pen)
        qp.drawLine(self.p0+QPoint(0,self.min_pos),self.p0+QPoint(self.histBoxSize.width(),self.min_pos))
        qp.drawLine(self.p0+QPoint(self.histBoxSize.width()/2,self.min_pos),self.p0+QPoint(self.histBoxSize.width(),0))
        qp.drawLine(self.p0+QPoint(self.histBoxSize.width(),0),self.p0+QPoint(self.histBoxSize.width() + bmp2.width(), 0 ))
        #qp.drawRect(QRect(self.p0+QPoint(0,self.max_pos-2-FH) ,self.p0+QPoint(0+FW+2,self.max_pos-2) ))
        minstr=n2h(self.cm.min)
        FW2=metrics.width(minstr)
        pen = QPen(Qt.blue, 2,  Qt.SolidLine)
        qp.drawRect(QRect(self.p0+QPoint(FW+10,self.min_pos+1) ,self.p0+QPoint(FW+14+FW2,self.min_pos+FH+1) ))

        qp.drawText(self.p0+QPoint(FW+12,self.min_pos+FH-1), minstr)


class Ui_BitmapForm(object):
    def setupUi(self, BitmapForm):
        BitmapForm.setObjectName("BitmapForm")
        BitmapForm.resize(663, 435)
        BitmapForm.setFrameShape(QtGui.QFrame.StyledPanel)
        BitmapForm.setFrameShadow(QtGui.QFrame.Raised)
        self.verticalLayout = QtGui.QVBoxLayout(BitmapForm)
        self.verticalLayout.setObjectName("verticalLayout")
        self.horizontalLayout = QtGui.QHBoxLayout()
        self.horizontalLayout.setObjectName("horizontalLayout")
        self.gridLayout = QtGui.QGridLayout()
        self.gridLayout.setObjectName("gridLayout")
        self.verticalSlider = QtGui.QSlider(BitmapForm)
        self.verticalSlider.setEnabled(False)
        self.verticalSlider.setOrientation(QtCore.Qt.Vertical)
        self.verticalSlider.setObjectName("verticalSlider")
        self.gridLayout.addWidget(self.verticalSlider, 1, 0, 1, 1)
        self.horizontalSlider = QtGui.QSlider(BitmapForm)
        self.horizontalSlider.setEnabled(False)
        self.horizontalSlider.setOrientation(QtCore.Qt.Horizontal)
        self.horizontalSlider.setObjectName("horizontalSlider")
        self.gridLayout.addWidget(self.horizontalSlider, 2, 3, 1, 1)
        self.PixBitmap = PixBitmap(BitmapForm)
        self.gridLayout.addWidget(self.PixBitmap, 1, 3, 1, 1)
        self.horizontalLayout.addLayout(self.gridLayout)
        self.PixHist = PixHist(BitmapForm)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Preferred)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.PixHist.sizePolicy().hasHeightForWidth())
        self.PixHist.setSizePolicy(sizePolicy)
        self.PixHist.setObjectName("PixHist")
        self.horizontalLayout.addWidget(self.PixHist)
        self.verticalLayout.addLayout(self.horizontalLayout)

        self.retranslateUi(BitmapForm)
        QtCore.QMetaObject.connectSlotsByName(BitmapForm)

    def retranslateUi(self, BitmapForm):
        BitmapForm.setWindowTitle(QtGui.QApplication.translate("BitmapForm", "Frame", None, QtGui.QApplication.UnicodeUTF8))



class ScrolledPixBitmap(QFrame, Ui_BitmapForm):
    def __init__(self,parent=None):
        super(ScrolledPixBitmap, self).__init__(parent)
        self.setupUi(self)
        self.automax=True
        self.automin=True
        #self.connect(self.horizontalSlider, SIGNAL("valueChanged"), self.horizontalSliderMoved)
        self.horizontalSlider.valueChanged.connect(self.horizontalSliderMoved)
        self.verticalSlider.valueChanged.connect(self.verticalSliderMoved)
        self.cm=ColorMap(self)

    def setColorMap(self,cm):
        self.cm.changeColorMap(cm)

    def setData(self,data):
        self.data=data
        self.PixHist.setColorMap(self.cm)
        self.PixBitmap.setColorMap(self.cm)
        self.PixBitmap.setData(data)
        self.PixHist.setData(data)


    def horizontalSliderMoved(self,pos):
        dx= self.PixBitmap._from[0] - pos
        self.PixBitmap._from[0]=self.PixBitmap._from[0]-dx
        self.PixBitmap._to[0]=self.PixBitmap._to[0]-dx
        self.PixBitmap.update()

    def verticalSliderMoved(self,pos):
        dy= self.PixBitmap._from[1] - pos
        self.PixBitmap._from[1]=self.PixBitmap._from[1]-dy
        self.PixBitmap._to[1]=self.PixBitmap._to[1]-dy
        self.PixBitmap.update()
    def _regenerate_bitmap(self):
        self.PixBitmap._regenerate_bitmap()
        self.PixBitmap.update()
        self.PixHist.update()
    def _update_sliders(self):
        return
        _from=self.PixBitmap._from
        _to=self.PixBitmap._to

        X=_to[0]-_from[0]
        Y=_to[1]-_from[1]

        if X!=self.data.shape[0]:
            self.horizontalSlider.blockSignals(True)
            self.horizontalSlider.setValue(_to[0])
            self.horizontalSlider.setMaximum(self.data.shape[0]-X)
            self.horizontalSlider.setEnabled(True)
            self.horizontalSlider.blockSignals(False)
        else:
            self.horizontalSlider.setEnabled(False)

        if Y!=self.data.shape[1]:
            self.verticalSlider.blockSignals(True)
            self.verticalSlider.setValue(_to[1])
            self.verticalSlider.setMaximum(self.data.shape[1]-Y)
            self.verticalSlider.setEnabled(True)
            self.verticalSlider.blockSignals(False)
        else:
            self.horizontalSlider.setEnabled(False)

        #print "Update",self.PixBitmap._from, self.PixBitmap._to

if __name__=="__main__":
    import numpy as np
    import scipy.ndimage as ndi

    data = np.random.normal(size=(256,256), scale=1)
    data = ndi.gaussian_filter(data, (2, 2))
    app = QApplication(sys.argv)
    spb=ScrolledPixBitmap()
    spb.setData(data)
    spb.cm.setHMin(-0.5)
    spb.cm.setHMax(0.5)
    spb._regenerate_bitmap()
    spb.show()
    app.exec_()
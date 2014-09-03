#!/usr/bin/env python
from PySide.QtCore import *
from PySide.QtGui import *
import numpy as np


class HistogramPlot(QFrame):
    def __init__(self,parent=None):
        QFrame.__init__(self, parent)
        self.data=[[0,1,2],[0,1,1],[1,1,1]]
        self.init(0,100)
        self.cursor=0
    def init(self,xmin,xmax,xstep=1):
        self.xmin=xmin
        self.xmax=xmax
        self.xspan=(xmax-xmin)/xstep
        self.xstep=xstep
        self.data=np.zeros((3,self.xspan))
        self.cursor=0
    def paintEvent(self, e):
#        if self.data:
            qp = QPainter()
            qp.begin(self)
            self.drawWidget(qp)
            qp.end()
    def updateValue(self, color,th,val):
        color=int(color)
        th=int(th)
        val=int(val)
        x=(th-self.xmin)/self.xstep
        self.cursor=x+1

        #print color,th,val
        self.data[color,x]+=val
        self.update()

    def drawWidget(self, qp):
        margin=2
        size = self.size() - QSize(margin*2,margin*2) # size in pixels of widget we can plot on
        self.p0=QPoint(margin,margin)
        qp.setBrush(QColor(255,255,255))
        qp.drawRect(QRect(self.p0,size))

        #grid
        qp.setPen(QColor(200,200,200))
        ystep=float(size.height())/10
        xstep=float(size.width())/6
        for i in range(10):
          p1=self.p0 + QPoint(0,ystep*i)
          p2=self.p0 + QPoint(size.width(),ystep*i)
          qp.drawLine(p1,p2)
        for i in range(6):
          p1=self.p0 + QPoint(xstep*i,0)
          p2=self.p0 + QPoint(xstep*i,size.height())
          qp.drawLine(p1,p2)

        # draw border
        pen = QPen(QColor(20, 20, 20), 1,  Qt.SolidLine)
        qp.setPen(pen)
        qp.setBrush(Qt.transparent)
        qp.drawRect(QRect(self.p0,size))

        dx=float(size.width())/(len(self.data[0])-1)
        #print "dx",dx
        mmax=np.max(self.data)*1.1
        mmax=max((mmax,1.0))
        #while mmax<np.max(self.data):
#            mmax*=10
        #print "mmax",mmax
        dy=float(size.height())/mmax

        for c in range(len(self.data)):
            points=[]
            y1=self.p0.y()+size.height()
            x1=self.p0.x()
            points.append(QPoint(x1,y1))
            for d in range(self.xspan):
                x2=self.p0.x()+dx*(d+1)
                y=self.p0.y()+size.height()-self.data[c][d]*dy
                points.append(QPoint(x1,y))
                points.append(QPoint(x2,y))
                x1=x2

            points.append(QPoint(x2,y1))
            if c==0:
              qp.setPen(QPen(QColor(0, 0, 150), 1,  Qt.SolidLine))
              qp.setBrush(QColor(0,0,220))
            elif c==1:
              qp.setPen(QPen(QColor(150, 0, 0), 1,  Qt.SolidLine))
              qp.setBrush(QColor(220,0,0))
            else:
              qp.setPen(QPen(QColor(0, 150, 0), 1,  Qt.SolidLine))
              qp.setBrush(QColor(0,220,0))
            qp.drawPolygon ( points)

            qp.setPen(QPen(QColor(128, 128, 128), 1,  Qt.SolidLine))

            p1=self.p0+QPoint(dx*self.cursor,0)
            p2=self.p0+QPoint(dx*self.cursor,size.height())
            qp.drawLine(p1,p2)

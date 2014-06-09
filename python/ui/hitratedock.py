#!/usr/bin/env python
from PySide.QtCore import *
from PySide.QtGui import *
from PySide import QtGui
from QDockWidgetClose import QDockWidgetClose

class RateDrawer(QFrame):
    def __init__(self,parent=None):
        QFrame.__init__(self, parent)
        self.data=None

    def paintEvent(self, e):
        if self.data:
            qp = QPainter()
            qp.begin(self)
            self.drawWidget(qp)
            qp.end()

    def drawWidget(self, qp):
        margin=2

        font = QFont('Serif', 7, QFont.Light)
        qp.setFont(font)
#        metrics = qp.fontMetrics()
#        MAX_TEXT_LEN=metrics.width("256")

        size = self.size() - QSize(margin*2,margin*2) # size in pixels of widget we can plot on
        self.p0=QPoint(margin,margin)

#        dataPlotSize=QSize(size)

        # size of the field which is ocupied by data plot
#        dataPlotSize.setHeight( dataPlotSize.height() -1 -2*margin - MAX_TEXT_LEN )
#        qp.drawRect(QRect(self.p0,dataPlotSize))

        # draw border
        pen = QPen(QColor(20, 20, 20), 1,  Qt.SolidLine)
        qp.setPen(pen)
        qp.setBrush(Qt.NoBrush)
        qp.drawRect(QRect(self.p0,size))
        dx=float(size.width())/(len(self.data)-1)
        mmax=1.0
        while mmax<max(self.data):
            mmax*=10

        dy=float(size.height())/mmax
        points=[]
        y1=self.p0.y()+size.height()
        x1=self.p0.x()
        points.append(QPoint(x1,y1))
        for d in range(len(self.data)):
            #print d
            x2=self.p0.x()+dx*d
            y2=self.p0.y()+size.height()-self.data[d]*dy
            points.append(QPoint(x2,y2))
        points.append(QPoint(x2,y1))
        qp.drawPolygon ( points)

class HitRateDock(QDockWidgetClose):
    def __init__(self,parent=None):
        QDockWidgetClose.__init__(self, parent)
        self.setObjectName("dockHitRate")
        self.Contents = QtGui.QWidget()
        self.Contents.setObjectName("dockHitRateContents")
        self.verticalLayout_7 = QtGui.QVBoxLayout(self.Contents)
        self.verticalLayout_7.setObjectName("verticalLayout_7")
        self.labelRate = QtGui.QLabel(self.Contents)
        self.labelRate.setObjectName("labelRate")
        self.verticalLayout_7.addWidget(self.labelRate)
        self.area = RateDrawer(self.Contents)
        self.area.setObjectName("area")

        self.verticalLayout_7.addWidget(self.area,1)
        self.setWidget(self.Contents)
        self.history=[0.0]*60
        self.area.data=self.history
        self.setWindowTitle(QtGui.QApplication.translate("MainWindow", "Hit Rate", None, QtGui.QApplication.UnicodeUTF8))

    def UpdateRate(self,data):
        self.history.pop(0)
        self.labelRate.setText(str(data))
        self.history.append(float(data))
        self.area.update()


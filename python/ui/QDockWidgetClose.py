#!/usr/bin/env python
from PySide.QtCore import *
from PySide.QtGui import *
from MainWnd import Ui_MainWindow
import sys
from SpidrTpx3_engine import *
import numpy as np
import scipy.ndimage as ndi
import random
import os
import time

class QDockWidgetClose(QDockWidget):
    def __init__(self,parent=None):
        QDockWidget.__init__(self, parent)
        self._ec=None
        self.dockLocationChanged.connect(self.changed)
        self.name=None

    def moveEvent(self, evn):
        QDockWidget.moveEvent(self,evn)
        self.changed()

    def changed(self):
        if self.name!=None:
            settings = QSettings()
            settings.beginGroup("DockWidgets")
            settings.setValue(self.name+"_isVisible", int(self.isVisible()))
            settings.setValue(self.name+"_pos", self.pos())
            settings.setValue(self.name+"_size", self.size())
            settings.setValue(self.name+"_floating", int(self.isFloating()))
    def setName(self,n,visibility=True):
        self.name=n
        self.vis=visibility

        settings = QSettings()
        settings.beginGroup("DockWidgets")
        v=int(settings.value(self.name+"_isVisible", self.vis))
        self.setVisible( v )
        self._cb.setChecked(v)

        self.setFloating(int(settings.value(self.name+"_floating", 0)))
        self.resize(settings.value(self.name+"_size", QSize(10, 10)))
        self.move(settings.value(self.name+"_pos", QPoint(200, 200)))
        settings.endGroup()

    def setAssociatedCheckbox(self,cb):
        self._cb=cb

    def closeEvent(self, *args, **kwargs):
        if self._cb:
            self._cb.setChecked(False)
        settings = QSettings()
        settings.beginGroup("DockWidgets")
        print self.name+"_isVisible", 0
        v=int(settings.value(self.name+"_isVisible", 0))
        settings.sync()
        QDockWidget.closeEvent(self,*args, **kwargs)
        self.hide()
        self.changed()


    def toggleVisibility(self):
        if self.isVisible():
            self.hide()
        else:
            self.show()
        self.changed()
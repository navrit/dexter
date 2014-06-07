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

    def setAssociatedCheckbox(self,cb):
        self._cb=cb

    def closeEvent(self, *args, **kwargs):
        if self._cb:
            self._cb.setChecked(False)
        QDockWidget.closeEvent(self,*args, **kwargs)

    def toggleVisibility(self):
        if self.isVisible():
            self.hide()
        else:
            self.show()

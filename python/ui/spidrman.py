#!/usr/bin/env python
import sys
import time
from PySide.QtCore import *
from PySide.QtGui import *
import resources_rc

import os
pdir=os.path.dirname(os.path.abspath(__file__))+"/.."
sys.path.append(pdir+"/tmp")
from tpx3gui import MainWindow

if __name__=="__main__":
    app = QApplication(sys.argv)
    pixmap = QPixmap(":/img/splash.png")
    splash = QSplashScreen(pixmap)
    splash.show()
    app.processEvents()
    splash.update()
    splash.showMessage("")
    form=MainWindow()
#   for i in range(5):
#     s="Loaded modules %s"%("."*i)
#      splash.showMessage(s)
#      time.sleep(0.4)
    time.sleep(1.5)
    form.show()
    form.update()
    splash.finish(form)
    app.exec_()

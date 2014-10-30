#!/usr/bin/env python
from PySide.QtCore import *
from PySide.QtGui import *
from AboutDlgUI import Ui_AboutDialog


class AboutDlg(QDialog, Ui_AboutDialog):
    def __init__(self,parent=None):
        super(AboutDlg, self).__init__(parent)
        self.parent=parent
        self.setupUi(self)
        self.exec_()

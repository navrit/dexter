from PySide.QtCore import *
from PySide.QtGui import *
from SPIDRAboutDlg import  Ui_SPIDRAboutDlg


class SPIDRAboutDlg(QDialog, Ui_SPIDRAboutDlg):
    def __init__(self,parent=None):
        super(SPIDRAboutDlg, self).__init__(parent)
        self.parent=parent
        self.setupUi(self)
        self.setFixedSize(300,130)
        self.labelClass.setText(parent.tpx.getClassVersion())
        self.labelSoft.setText(parent.tpx.getSoftwVersion())
        self.labelFirmware.setText(parent.tpx.getFirmwVersion())
        self.labelLinks.setText(parent.tpx.getLinkSpeed())
        self.exec_()

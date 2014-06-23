from PySide.QtCore import *
from PySide.QtGui import *
from SPIDRAboutDlg import  Ui_SPIDRAboutDlg
from SPIDRSettingsDlg import Ui_SPIDRSettingsDlg


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

class SPIDRSettingsDlg(QDialog, Ui_SPIDRSettingsDlg):
    def __init__(self,parent=None):
        super(SPIDRSettingsDlg, self).__init__(parent)
        self.parent=parent
        self.setupUi(self)
        settings = QSettings()
        settings.beginGroup("SPIDR")
        self.lineIP.setText(settings.value("IP", "192.168.100.10"))
        self.linePort.setText(settings.value("Port", "50000"))
        self.comboDAQ.setCurrentIndex(int(settings.value("DAQ", 0)))
        settings.endGroup()
        self.setFixedSize(180,120)
        self.exec_()
    def accept(self):
        settings = QSettings()
        settings.beginGroup("SPIDR")
        settings.setValue("IP", self.lineIP.text())
        settings.setValue("Port", self.linePort.text())
        settings.setValue("DAQ",self.comboDAQ.currentIndex())
        settings.endGroup()

        QDialog.accept(self)

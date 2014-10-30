from PySide.QtCore import *
from PySide.QtGui import *
from SPIDRSettingsDlgUI import Ui_SPIDRSettingsDlg



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

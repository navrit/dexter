from PySide.QtCore import *
from PySide.QtGui import *
from ProbestationSettingsDlgUI import *



class ProbestationSettingsDlg(QDialog, Ui_ProbestationSettingsDlg):
    def __init__(self,parent=None):
        super(ProbestationSettingsDlg, self).__init__(parent)
        self.parent=parent
        self.setupUi(self)
        settings = QSettings()
        settings.beginGroup("Probestation")
        self.gpib.setText(settings.value("GPIB", "22"))
        settings.endGroup()
        self.setFixedSize(180,80)
        self.exec_()
        
    def accept(self):
        settings = QSettings()
        settings.beginGroup("Probestation")
        settings.setValue("GPIB", self.gpib.text())
        settings.endGroup()

        QDialog.accept(self)

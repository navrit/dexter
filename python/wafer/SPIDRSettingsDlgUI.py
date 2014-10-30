# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'wafer/SPIDRSettingsDlgUI.ui'
#
# Created: Wed Oct 29 13:19:31 2014
#      by: pyside-uic 0.2.13 running on PySide 1.1.2
#
# WARNING! All changes made in this file will be lost!

from PySide import QtCore, QtGui

class Ui_SPIDRSettingsDlg(object):
    def setupUi(self, SPIDRSettingsDlg):
        SPIDRSettingsDlg.setObjectName("SPIDRSettingsDlg")
        SPIDRSettingsDlg.resize(168, 118)
        self.verticalLayout = QtGui.QVBoxLayout(SPIDRSettingsDlg)
        self.verticalLayout.setObjectName("verticalLayout")
        self.formLayout = QtGui.QFormLayout()
        self.formLayout.setObjectName("formLayout")
        self.label = QtGui.QLabel(SPIDRSettingsDlg)
        self.label.setObjectName("label")
        self.formLayout.setWidget(0, QtGui.QFormLayout.LabelRole, self.label)
        self.label_2 = QtGui.QLabel(SPIDRSettingsDlg)
        self.label_2.setObjectName("label_2")
        self.formLayout.setWidget(2, QtGui.QFormLayout.LabelRole, self.label_2)
        self.linePort = QtGui.QLineEdit(SPIDRSettingsDlg)
        self.linePort.setObjectName("linePort")
        self.formLayout.setWidget(2, QtGui.QFormLayout.FieldRole, self.linePort)
        self.lineIP = QtGui.QLineEdit(SPIDRSettingsDlg)
        self.lineIP.setObjectName("lineIP")
        self.formLayout.setWidget(0, QtGui.QFormLayout.FieldRole, self.lineIP)
        self.label_3 = QtGui.QLabel(SPIDRSettingsDlg)
        self.label_3.setObjectName("label_3")
        self.formLayout.setWidget(3, QtGui.QFormLayout.LabelRole, self.label_3)
        self.comboDAQ = QtGui.QComboBox(SPIDRSettingsDlg)
        self.comboDAQ.setObjectName("comboDAQ")
        self.comboDAQ.addItem("")
        self.comboDAQ.addItem("")
        self.comboDAQ.addItem("")
        self.formLayout.setWidget(3, QtGui.QFormLayout.FieldRole, self.comboDAQ)
        self.verticalLayout.addLayout(self.formLayout)
        self.buttonBox = QtGui.QDialogButtonBox(SPIDRSettingsDlg)
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName("buttonBox")
        self.verticalLayout.addWidget(self.buttonBox)

        self.retranslateUi(SPIDRSettingsDlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("accepted()"), SPIDRSettingsDlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("rejected()"), SPIDRSettingsDlg.reject)
        QtCore.QMetaObject.connectSlotsByName(SPIDRSettingsDlg)

    def retranslateUi(self, SPIDRSettingsDlg):
        SPIDRSettingsDlg.setWindowTitle(QtGui.QApplication.translate("SPIDRSettingsDlg", "SPIDR Settings", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("SPIDRSettingsDlg", "IP", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("SPIDRSettingsDlg", "Port", None, QtGui.QApplication.UnicodeUTF8))
        self.linePort.setText(QtGui.QApplication.translate("SPIDRSettingsDlg", "50000", None, QtGui.QApplication.UnicodeUTF8))
        self.lineIP.setText(QtGui.QApplication.translate("SPIDRSettingsDlg", "192.168.100.10", None, QtGui.QApplication.UnicodeUTF8))
        self.label_3.setText(QtGui.QApplication.translate("SPIDRSettingsDlg", "DAQ", None, QtGui.QApplication.UnicodeUTF8))
        self.comboDAQ.setItemText(0, QtGui.QApplication.translate("SPIDRSettingsDlg", "Custom", None, QtGui.QApplication.UnicodeUTF8))
        self.comboDAQ.setItemText(1, QtGui.QApplication.translate("SPIDRSettingsDlg", "SPIDR", None, QtGui.QApplication.UnicodeUTF8))
        self.comboDAQ.setItemText(2, QtGui.QApplication.translate("SPIDRSettingsDlg", "None", None, QtGui.QApplication.UnicodeUTF8))


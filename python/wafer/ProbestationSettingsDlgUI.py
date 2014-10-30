# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'wafer/ProbestationSettingsDlgUI.ui'
#
# Created: Wed Oct 29 13:19:31 2014
#      by: pyside-uic 0.2.13 running on PySide 1.1.2
#
# WARNING! All changes made in this file will be lost!

from PySide import QtCore, QtGui

class Ui_ProbestationSettingsDlg(object):
    def setupUi(self, ProbestationSettingsDlg):
        ProbestationSettingsDlg.setObjectName("ProbestationSettingsDlg")
        ProbestationSettingsDlg.resize(220, 65)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Minimum)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(ProbestationSettingsDlg.sizePolicy().hasHeightForWidth())
        ProbestationSettingsDlg.setSizePolicy(sizePolicy)
        self.verticalLayout = QtGui.QVBoxLayout(ProbestationSettingsDlg)
        self.verticalLayout.setObjectName("verticalLayout")
        self.formLayout = QtGui.QFormLayout()
        self.formLayout.setFieldGrowthPolicy(QtGui.QFormLayout.ExpandingFieldsGrow)
        self.formLayout.setObjectName("formLayout")
        self.label = QtGui.QLabel(ProbestationSettingsDlg)
        self.label.setObjectName("label")
        self.formLayout.setWidget(0, QtGui.QFormLayout.LabelRole, self.label)
        self.gpib = QtGui.QLineEdit(ProbestationSettingsDlg)
        self.gpib.setObjectName("gpib")
        self.formLayout.setWidget(0, QtGui.QFormLayout.FieldRole, self.gpib)
        self.verticalLayout.addLayout(self.formLayout)
        self.buttonBox = QtGui.QDialogButtonBox(ProbestationSettingsDlg)
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName("buttonBox")
        self.verticalLayout.addWidget(self.buttonBox)

        self.retranslateUi(ProbestationSettingsDlg)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("accepted()"), ProbestationSettingsDlg.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("rejected()"), ProbestationSettingsDlg.reject)
        QtCore.QMetaObject.connectSlotsByName(ProbestationSettingsDlg)

    def retranslateUi(self, ProbestationSettingsDlg):
        ProbestationSettingsDlg.setWindowTitle(QtGui.QApplication.translate("ProbestationSettingsDlg", "Probestation Settings", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("ProbestationSettingsDlg", "GPIB Address", None, QtGui.QApplication.UnicodeUTF8))
        self.gpib.setText(QtGui.QApplication.translate("ProbestationSettingsDlg", "22", None, QtGui.QApplication.UnicodeUTF8))


#!/usr/bin/python
# -*- coding: utf-8 -*-

"""
ZetCode PySide tutorial 

This program creates a toolbar.

author: Jan Bodnar
website: zetcode.com 
last edited: August 2011
"""
import signal
import subprocess

import sys
from PySide.QtCore import *
from PySide.QtGui import *
import resources_rc
from WaferProbingUI import Ui_MainWindow
from AboutDlg import *
from SPIDRSettingsDlg import *
from ProbestationSettingsDlg import *


QCoreApplication.setOrganizationName("CERN");
QCoreApplication.setApplicationName("tpx3wafer");

def sigint_handler(*args):
    """Handler for the SIGINT signal."""
    if QMessageBox.question(None, '', "Are you sure you want to quit?",
                            QMessageBox.Yes | QMessageBox.No,
                            QMessageBox.No) == QMessageBox.Yes:

        QApplication.quit()

class MainWindow(QMainWindow, Ui_MainWindow):
    def __init__(self):
        super(MainWindow, self).__init__()
        self.setupUi(self)
        self.show()
        self.actionStartWafer.triggered.connect(self.onStartWafer)
        self.toolStartWafer.clicked.connect(self.onStartWafer)
        self.actionStartOne.triggered.connect(self.onStartOne)
        self.toolStartOne.clicked.connect(self.onStartOne)
        self.toolStop.clicked.connect(self.onStop)
        self.toolProbestation.clicked.connect(self.onProbesation)
        self.actionAbout.triggered.connect(self.onAbout)
        self.actionExit.triggered.connect(self.close)
        self.actionSPIDRSettings.triggered.connect(self.onSPIDRSettings)
        self.actionProbestationSettings.triggered.connect(self.onProbestationSettings)
        self.cursor = QTextCursor(self.textEdit.textCursor())
        self.remove=False
        self.dumytimer = QTimer()
        self.dumytimer.start(100)  #
        self.dumytimer.timeout.connect(lambda: None)  # Let the interpreter run each 100 ms.
    def onProbesation(self):
        print "connect"
    def onAbout(self):
        dlg=AboutDlg()
    def onHelp(self):
        print "X"
    def onBurnFuses(self):
        pass
    def procFinished(self):
        self.appendText("Process finished. Exit code:%d"%self.proc.exitCode())
        self.toolStartOne.setEnabled(True)
        self.toolStop.setEnabled(False)
        self.proc.exitCode()

    def onStop(self):
        self.proc.terminate()
        self.proc.waitForFinished()
        self.toolStartOne.setEnabled(True)
        self.toolStop.setEnabled(False)

    def appendText(self,s):
        msg="<pre>"
        self.textEdit.setTextInteractionFlags(0)

        for line in str(s).splitlines():
#          ll=""
#          for c in line:
#            ll+="%02x "%ord(c)
#          print "\n-----------------\n%s\n%s\n"%(ll, line)
          c="#000000"

          if line.find("ERROR")>=0 or line.find("CRITICAL")>=0: c="#EE0000"
          if line.find("WARNING")>=0: c="#800000"
          if line.find("DEBUG")>=0: c="#888888"
          #line.replace(" ","&nbsp;")
          line='<font color=\"%s\"> %s </font><br>'%(c,line)
          msg+=line
          #print "!%s!"%s
          #self.textEdit.append(s)
        #print ord(s[0]),ord(s[-1]),s
        msg+="</pre>"
        self.cursor.clearSelection()
        if  self.remove:
            self.cursor.movePosition(QTextCursor.End)
            self.cursor.select(QTextCursor.LineUnderCursor)
            self.cursor.removeSelectedText()
            self.cursor.deletePreviousChar()
            self.cursor.select(QTextCursor.LineUnderCursor)
            self.cursor.removeSelectedText()
            self.cursor.movePosition(QTextCursor.End)
            #self.textEdit.setTextCursor(self.cursor)
            self.remove=False

        if ord(s[-1])==13:
            self.remove=True


        self.cursor.movePosition(QTextCursor.End)
        self.textEdit.insertHtml(msg)
        self.textEdit.setTextCursor(self.cursor)

        self.textEdit.verticalScrollBar().setValue(self.textEdit.verticalScrollBar().maximum())

       # self.textEdit.setTextInteractionFlags(QtCore.Qt.TextSelectableByKeyboard|QtCore.Qt.TextSelectableByMouse)

    def procReadStdOutput(self):
        s=self.proc.readAllStandardOutput()
        self.appendText(s)


    def onSPIDRSettings(self):
        dlg=SPIDRSettingsDlg(self)
    def onProbestationSettings(self):
        dlg=ProbestationSettingsDlg(self)

    def onStartOne(self):
        verbose=""
        if self.actionVerbose.isChecked():
            verbose="-v"
        self.toolStartOne.setEnabled(False)
        self.toolStop.setEnabled(True)

        self.proc=QProcess()
        self.proc.setProcessChannelMode(QProcess.MergedChannels)
        self.proc.readyReadStandardOutput.connect(self.procReadStdOutput)
        self.proc.finished.connect(self.procFinished)
        cmd="./tpx3_tests.py %s x test01_supply  test17_efuse test01_bias test02_registers test03_dac_scan test06_config_matrix test04_pixel_kidnapper  test07_clock_phasing test08_noise_scan test04_diagonal_scurves "%(verbose)
        cmd="./tpx3_tests.py %s x test08_noise_scan test04_diagonal_scurves "%(verbose)
        self.proc.start(cmd)
        self.appendText("Starting command:%s"%cmd)
        self.proc.waitForStarted()
    def onStartWafer(self):
        print "Probing"
        self.proc=QProcess()
        self.proc.setProcessChannelMode(QProcess.MergedChannels)
        self.proc.readyReadStandardOutput.connect(self.procReadStdOutput)
        self.proc.finished.connect(self.procFinished)
        self.proc.start("./tpx3_tests.py x test01_bias")

def main():
    signal.signal(signal.SIGINT, sigint_handler)
    #signal.signal(signal.SIGINT, signal.SIG_DFL)
    app = QApplication(sys.argv)
    ex = MainWindow()
    sys.exit(app.exec_())


if __name__ == '__main__':
    main()

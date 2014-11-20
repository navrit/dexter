#!/usr/bin/python
# -*- coding: utf-8 -*-

import signal
import subprocess

import sys
from PySide.QtCore import *
from PySide.QtGui import *
import resources_rc
from PyQt4 import QtCore
from WaferProbingUI import Ui_MainWindow
from AboutDlg import *
from SPIDRSettingsDlg import *
from ProbestationSettingsDlg import *
from probestation import *
import functools

QCoreApplication.setOrganizationName("CERN");
QCoreApplication.setApplicationName("tpx3wafer");

def sigint_handler(*args):
    """Handler for the SIGINT signal."""
    if QMessageBox.question(None, '', "Are you sure you want to quit?",
                            QMessageBox.Yes | QMessageBox.No,
                            QMessageBox.No) == QMessageBox.Yes:

        QApplication.quit()

class ProbeStationThread(QThread):
    done = QtCore.Signal(object)
    def __init__(self, ps):
        QtCore.QThread.__init__(self)
        self.ps = ps
        self.opdone=False
    def probestationError(self,msg):
        msgBox = QMessageBox()
        msgBox.setText("Probestation error")
        msgBox.setInformativeText(msg)
        msgBox.setIcon(QMessageBox.Critical)
        msgBox.exec_()

    def run(self):
        self.opdone=False
        try:
             self.cmd()
             #x,y=self.ps.ReadMapPosition()
             self.done.emit('')
        except ProbeStationException as ex:
             self.probestationError(str(ex))
        self.opdone=True

class ProbingThread(QThread):
    done = QtCore.Signal(object)
    def __init__(self, parent):
        QtCore.QThread.__init__(self)
        self.parent = parent

    def probeCmd(self,cmd):
        self.parent.psthread.cmd=cmd
        self.parent.psthread.start()
        while True:
            time.sleep(0.01)
            if not self.parent.psthread.isRunning(): break
        while True:
            time.sleep(0.01)
            if self.parent.psthread.opdone: break
        print "moved"
        time.sleep(0.01)

    def run(self):
        for i in range(3):
            print i
            time.sleep(0.1)
            self.probeCmd(self.parent.ps.StepNextDie)

        return

class MainWindow(QMainWindow, Ui_MainWindow):
    def __init__(self):
        super(MainWindow, self).__init__()
        self.setupUi(self)
        self.show()

        self.toolStartWafer.clicked.connect(self.onStartWafer)
        self.toolStartOne.clicked.connect(self.onStartOne)
        self.toolStop.clicked.connect(self.onStop)
        self.toolProbestation.clicked.connect(self.onProbesation)
        self.toolChipInfo.clicked.connect(self.onChipInfo)
        self.toolBurn.clicked.connect(self.onBurnFuses)

        self.actionStartWafer.triggered.connect(self.onStartWafer)
        self.actionStartOne.triggered.connect(self.onStartOne)


        self.toolProbeNext.clicked.connect(self.onProbeNext)
        self.toolProbeFirst.clicked.connect(self.onProbeFirst)
        self.toolProbeAlign.clicked.connect(self.onProbeAlign)
        self.toolProbeContact.clicked.connect(self.onProbeContact)
        self.toolGoTo.clicked.connect(self.onProbeGoTo)

        self.actionAbout.triggered.connect(self.onAbout)
        self.actionExit.triggered.connect(self.close)
        self.actionSPIDRSettings.triggered.connect(self.onSPIDRSettings)
        self.actionProbestationSettings.triggered.connect(self.onProbestationSettings)
        self.cursor = QTextCursor(self.textEdit.textCursor())
        self.remove=False
        self.dumytimer = QTimer()
        self.dumytimer.start(100)  #
        self.dumytimer.timeout.connect(lambda: None)  # Let the interpreter run each 100 ms.
        self.ps=None

        self.probing_thread=ProbingThread(self)
        self.probing_thread.finished.connect(self.onProbingDone)

    def onProbeNext(self):
        if self.ps:
            self._probestationSetEnable(False)
            self.psthread.cmd=self.ps.StepNextDie
            self.psthread.start()

    def _probeGoToXY(self,x,y):
        print x,y
        if self.ps:
            self._probestationSetEnable(False)
            self.psthread.cmd=functools.partial( self.ps.GoToXY, x, y)
            self.psthread.start()

    def onProbeGoTo(self):
                # create context menu
        self.popMenu = QtGui.QMenu(self)
        X2LETER=['-','A','B','C','D','E','F','G','H','I','J','K','L','M']

        for x in range(1,14):
            colmenu = QtGui.QMenu("%s"%X2LETER[x])
            self.popMenu.addMenu(colmenu)

            for YM in range(1,11+1):
                y=12-YM

                if y in (1,) :     MINX,MAXX=6,8
                if y in (11,) :    MINX,MAXX=5,9
                if y in (2,) :     MINX,MAXX=4,10
                if y in (3,10) :   MINX,MAXX=3,11
                if y in (4,8,9) :  MINX,MAXX=2,12
                if y in (5,6,7) :  MINX,MAXX=1,13
                if x<MINX or x>MAXX: continue

                name=self.xy2dieName(x,y)#"%s%d"%(X2LETER[x-1],Y)
                callback=  functools.partial( self._probeGoToXY, x, y)
                colmenu.addAction(QtGui.QAction(name, self,triggered=callback))
        self.popMenu.exec_(self.mapToGlobal(self.toolGoTo.pos()+QPoint(24,40)))


    def probestationError(self,msg):
        msgBox = QMessageBox()
        msgBox.setText("Probestation error")
        msgBox.setInformativeText(msg)
        msgBox.setIcon(QMessageBox.Critical)
        msgBox.exec_()

    def _probestationSetEnable(self,s):
        self.toolProbeNext.setEnabled(s)
        self.toolProbeFirst.setEnabled(s)
        self.toolProbeAlign.setEnabled(s)
        self.toolProbeContact.setEnabled(s)
        self.toolProbestation.setEnabled(s)
        self.toolGoTo.setEnabled(s)
        if not s:
            self.chipName.setText('---')

    def onProbeStationDone(self,s):
        diename=self.xy2dieName(self.ps.x,self.ps.y)
        self.chipName.setText(diename)
        self._probestationSetEnable(True)
        if self.ps.z==self.ps.ALIGN:
             self.toolProbeContact.setEnabled(True)
             self.toolProbeAlign.setEnabled(False)
        else:
             self.toolProbeContact.setEnabled(False)
             self.toolProbeAlign.setEnabled(True)

    def getDieName(self):
        chip=self.chipName.text()
        if chip=='---':chip="unknown"
        return "W%d_%s"%(int(self.waferNumber.text()),chip)

    def onProbeFirst(self):
        if self.ps:
            self._probestationSetEnable(False)
            self.psthread.cmd=self.ps.StepFirstDie
            self.psthread.start()

    def onProbeAlign(self):
        if self.ps:
            self._probestationSetEnable(False)
            self.psthread.cmd=self.ps.Align
            self.psthread.start()

    def xy2dieName(self,x,y):
        XLUT=('-','A','B','C','D','E','F','G','H','I','J','K','L','M')
        return "%s%d"%(XLUT[x],y)

    def onProbeContact(self):
        if self.ps:
            self._probestationSetEnable(False)
            self.psthread.cmd=self.ps.Contact
            self.psthread.start()


    def onProbesation(self):
        if self.ps!=None:
            self.ps=None
            self.toolProbeNext.setEnabled(False)
            self.toolProbeFirst.setEnabled(False)
            self.toolProbeAlign.setEnabled(False)
            self.toolProbeContact.setEnabled(False)
            self.chipName.setText('---')

        else:
            settings = QSettings()
            settings.beginGroup("Probestation")
            address=int(settings.value("GPIB", "22"))
            settings.endGroup()
            try:
               self.ps=ProbeStation(address=address,logname="probestation.log")
               self.ps.connect()
               x,y=self.ps.ReadMapPosition()
               self.psthread=ProbeStationThread(self.ps)
               self.psthread.done.connect(self.onProbeStationDone)

               self.chipName.setText(self.xy2dieName(x,y))
               self.toolProbeNext.setEnabled(True)
               self.toolProbeFirst.setEnabled(True)
               self.toolGoTo.setEnabled(True)
               if self.ps.getChuckPosition()==self.ps.ALIGN:
                   self.toolProbeContact.setEnabled(True)
                   self.toolProbeAlign.setEnabled(False)
               else:
                   self.toolProbeContact.setEnabled(False)
                   self.toolProbeAlign.setEnabled(True)

            except ProbeStationException as ex:
                self.probestationError(str(ex))

            
       
    def onAbout(self):
        dlg=AboutDlg()
    def onHelp(self):
        print "X"

    def onBurnFuses(self):
        if self.ps==None:
            msgBox = QMessageBox()
            msgBox.setText("ERROR")
            msgBox.setInformativeText("Probestation is not connected. Unable to burn CHIP ID.")
            msgBox.setIcon(QMessageBox.Critical)
            msgBox.exec_()
        else:
           if QMessageBox.question(None, '', "Going to burn CHIP ID : %s"%self.getDieName(),
                                QMessageBox.Yes | QMessageBox.No,
                                QMessageBox.No) == QMessageBox.Yes:
               print "OK"




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

    def procFinished(self):
        self.appendText("Process finished. Exit code:%d"%self.proc.exitCode())
        self.toolStartOne.setEnabled(True)
        self.toolStartWafer.setEnabled(True)
        self.toolChipInfo.setEnabled(True)
        self.toolStop.setEnabled(False)


    def startCmd(self,cmd):
        self.toolStartOne.setEnabled(False)
        self.toolStartWafer.setEnabled(False)
        self.toolChipInfo.setEnabled(False)
        self.toolStop.setEnabled(True)
        print cmd
        self.proc=QProcess()
        self.proc.setProcessChannelMode(QProcess.MergedChannels)
        self.proc.readyReadStandardOutput.connect(self.procReadStdOutput)
        self.proc.finished.connect(self.procFinished)
        self.proc.start(cmd)
        self.appendText("Starting command:%s"%cmd)
        self.proc.waitForStarted()



    def onStop(self):
        self.proc.terminate()
        self.proc.waitForFinished()

    def onChipInfo(self):
        verbose=""
        if self.actionVerbose.isChecked():
            verbose="-v"
        cmd="./tpx3_tests.py %s %s test01_supply test17_efuse "%(verbose,self.getDieName())
        self.startCmd(cmd)



    def onStartOne(self):
        verbose=""
        if self.actionVerbose.isChecked():
            verbose="-v"
        cmd="./tpx3_tests.py %s x test01_supply  test17_efuse test01_bias test02_registers test03_dac_scan test06_config_matrix test04_pixel_kidnapper  test07_clock_phasing test08_noise_scan test04_diagonal_scurves "%(verbose)
        cmd="./tpx3_tests.py %s x test08_noise_scan test04_diagonal_scurves "%(verbose)
        self.startCmd(cmd)

    def onProbingDone(self):
        print "probing done"
        self._probestationSetEnable(True)
        self.toolStartOne.setEnabled(True)
        self.toolStartWafer.setEnabled(True)
        self.toolChipInfo.setEnabled(True)
        self.toolStop.setEnabled(False)

    def onStartWafer(self):
        print "Probing"
        self.toolStartOne.setEnabled(False)
        self.toolStartWafer.setEnabled(False)
        self.toolChipInfo.setEnabled(False)
        self.toolStop.setEnabled(True)
        self._probestationSetEnable(False)
        self.probing_thread.start()
        while True:
            time.sleep(0.001)
            if self.probing_thread.isRunning(): break
        print "started"

def main():
    signal.signal(signal.SIGINT, sigint_handler)
    #signal.signal(signal.SIGINT, signal.SIG_DFL)
    app = QApplication(sys.argv)
    ex = MainWindow()
    sys.exit(app.exec_())


if __name__ == '__main__':
    main()

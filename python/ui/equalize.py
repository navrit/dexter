#!/usr/bin/env python
from PySide.QtCore import *
from PySide.QtGui import *
from EqualizeForm import Ui_EqualizeForm
import time


class EqualizeThread(QThread):
    def __init__(self, parent=None):
        QThread.__init__(self, parent)
        self.parent=parent
        self.abort = False

    def stop(self):
        self.abort = True
        self.parent.buttonEqualize.setEnabled(True)

    def run(self):
        self.abort=False
        total_hits=0
        last_time=time.time()
        for i in range(100):
            if self.abort:
                return
            time.sleep(0.05)
            #self.parent.progressBar.setValue(i)
            self.emit(SIGNAL("progress(int)"), i)
        self.parent.buttonEqualize.setEnabled(True)
        self.parent.buttonEqualize.setText("Done")

class EqualizeDlg(QDialog, Ui_EqualizeForm):
    def __init__(self,parent=None):
        super(EqualizeDlg, self).__init__(parent)
        self.setupUi(self)
        self.buttonDirectory.clicked.connect(self.onDir)
        self.buttonCancel.clicked.connect(self.onCancel)
        self.buttonEqualize.clicked.connect(self.onEqualize)
        self.EqualizeThread = EqualizeThread(self)
        QObject.connect(self.EqualizeThread, SIGNAL("progress(int)"),self.progressBar, SLOT("setValue(int)"), Qt.QueuedConnection)
        self.exec_()

    def onEqualize(self):
        self.buttonEqualize.setEnabled(False)
        self.progressBar.setMaximum(100)
        self.EqualizeThread.start()

    def onCancel(self):
        if self.EqualizeThread.isRunning():
            self.EqualizeThread.stop()
        else:
            self.close()

    def onDir(self):
        print "s"
        directory = QFileDialog.getExistingDirectory(self,
                                          self.tr("QFileDialog.getExistingDirectory()"),
                                          self.lineDirectory.text(),
                                          QFileDialog.DontResolveSymlinks | QFileDialog.ShowDirsOnly)
        if directory!="":
            self.lineDirectory.setText(directory)

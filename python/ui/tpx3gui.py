#!/usr/bin/env python
from PySide.QtCore import *
from PySide.QtGui import *
from MainWnd import Ui_MainWindow
import sys
import numpy as np
import scipy.ndimage as ndi
import random
import os
import time

from tpx3 import *
from equalize import EqualizeDlg

from kutils import n2h
from about import AboutDlg

QCoreApplication.setOrganizationName("CERN");
QCoreApplication.setApplicationName("t3g");
import scipy.ndimage as ndi
import random
import os
import time
from hitratedock import HitRateDock


class MainWindow(QMainWindow, Ui_MainWindow):
    def __init__(self,parent=None):
        super(MainWindow, self).__init__(parent)
        self.setupUi(self)
        self.tpx=None
        self.genConfigPolarity.currentIndexChanged['QString'].connect(self.gcrChanged)
#        self.genConfigAckCmd.currentIndexChanged['QString'].connect(self.gcrChanged)
        self.genConfigFastLo.currentIndexChanged['QString'].connect(self.gcrChanged)
        self.genConfigGrayCnt.currentIndexChanged['QString'].connect(self.gcrChanged)
        self.genConfigMode.currentIndexChanged['QString'].connect(self.gcrChanged)
     #   self.genConfigSelectTP.currentIndexChanged['QString'].connect(self.gcrChanged)
        self.genConfigTimeroverflow .currentIndexChanged['QString'].connect(self.gcrChanged)
        #self.TPSource.currentIndexChanged['QString'].connect(self.gcrChanged)
        self.comboShutterType.currentIndexChanged.connect(self.onComboShutterTypeChanged)
        self.connect(self.buttonShutter ,SIGNAL("clicked()"), self.shutterOnOff)
        self.outputMask0.stateChanged.connect(self.outputMaskChanged)
        self.outputMask1.stateChanged.connect(self.outputMaskChanged)
        self.outputMask2.stateChanged.connect(self.outputMaskChanged)
        self.outputMask3.stateChanged.connect(self.outputMaskChanged)
        self.outputMask4.stateChanged.connect(self.outputMaskChanged)
        self.outputMask5.stateChanged.connect(self.outputMaskChanged)
        self.outputMask6.stateChanged.connect(self.outputMaskChanged)
        self.outputMask7.stateChanged.connect(self.outputMaskChanged)
        self.devid=0
        self.shutter=0
        self.all_dacs=["dac_ibias_preamp_on","dac_ibias_preamp_off","dac_vpreamp_ncas","dac_ibias_ikrum","dac_vfbk",\
                     "dac_vthresh","dac_ibias_discs1_on","dac_ibias_discs1_off",\
                     "dac_ibias_discs2_on","dac_ibias_discs2_off","dac_ibias_pixeldac","dac_ibias_tpbufin",\
                     "dac_ibias_tpbufout","dac_vtp_coarse","dac_vtp_fine"]
        #"dac_vthresh_fine","dac_vthresh_coarse"
        self.connect(self.buttonDefaults ,SIGNAL("clicked()"), self.defaults)
        self.genConfigTP.stateChanged.connect(self.TPEnableChanged)
        self.TPSource.currentIndexChanged.connect(self.TPSourceChanged)
        #self.sliderThreshold.valueChanged.connect(self.thresholdSliderMoved)
        #self.groupBox.setVisible(False)

        for dn in self.all_dacs:
            dac=getattr(self,dn)
            tpx_dn="TPX3_"+dn[4:].upper()
            print dn,tpx_dn
            dac.valueChanged.connect(lambda val,dacn=int(eval(tpx_dn)):self.onDacChanged(dacn,val))

        self.actionConnectDemo.triggered.connect(self.onConnectDemo)

        self.actionConnectSPIDR.triggered.connect(self.onConnectSPIDR)
        self.actionClose.triggered.connect(QCoreApplication.instance().quit)

        self.dockHitRate = HitRateDock(self)
        self.addDockWidget(Qt.DockWidgetArea(1), self.dockHitRate)
        self.actionTestpulses.triggered.connect(self.dockTP.toggleVisibility)
        self.actionOthers.triggered.connect(self.dockOthers.toggleVisibility)
        self.actionDACs.triggered.connect(self.dockDACs.toggleVisibility)
        self.actionGeneral.triggered.connect(self.dockGeneral.toggleVisibility)
        self.actionShutter.triggered.connect(self.dockShutter.toggleVisibility)
        self.actionHitRate.triggered.connect(self.dockHitRate.toggleVisibility)
        self.actionDemoConfig.triggered.connect(self.dockDemoConfig.toggleVisibility)
        self.actionOutputs.triggered.connect(self.dockOutputs.toggleVisibility)

        self.dockGeneral.setAssociatedCheckbox(self.actionGeneral)
        self.dockDACs.setAssociatedCheckbox(self.actionDACs)
        self.dockTP.setAssociatedCheckbox(self.actionTestpulses)
        self.dockOthers.setAssociatedCheckbox(self.actionOthers)
        self.dockShutter.setAssociatedCheckbox(self.actionShutter)
        self.dockDemoConfig.setAssociatedCheckbox(self.actionDemoConfig)
        self.dockHitRate.setAssociatedCheckbox(self.actionHitRate)
        self.dockOutputs.setAssociatedCheckbox(self.actionOutputs)

        self.dockGeneral.setName("General",1)
        self.dockDACs.setName("DACs",0)
        self.dockTP.setName("TP",1)
        self.dockOthers.setName("Others",0)
        self.dockShutter.setName("Shutter",1)
        self.dockDemoConfig.setName("DemoConfig",0)
        self.dockHitRate.setName("HitRate",0)
        self.dockOutputs.setName("Outputs",0)
        self.tabsMain.currentChanged.connect(self.onTabPageChange)

        settings = QSettings()

        settings.beginGroup("MainWindow")
        self.resize(settings.value("size", QSize(400, 400)))
        self.move(settings.value("pos", QPoint(200, 200)))
        settings.endGroup()

        settings.setValue("runs", int(settings.value("runs", 0))+1)
        self.actionEqualize.triggered.connect(self.onEqualize)
        self.actionAbout.triggered.connect(self.onAbout)

        self.actionMaskAll.triggered.connect(self.onActionMaskAll)
        self.actionUnmaskAll.triggered.connect(self.onActionUnmaskAll)
        self.actionMaskLoad.triggered.connect(self.onActionMaskLoad)
        self.actionMaskSave.triggered.connect(self.onActionMaskSave)

        self.actionTrimsLoad.triggered.connect(self.onActionTrimsLoad)
        self.actionTrimsSave.triggered.connect(self.onActionTrimsSave)
        self.actionTrimsReset.triggered.connect(self.onActionTrimsReset)

        self.actionLoadConfiguration.triggered.connect(self.onActionLoadConfiguration)
        self.actionSaveConfiguration.triggered.connect(self.onActionSaveConfiguration)

    def onAbout(self):
        dlg=AboutDlg()

    def onActionMaskAll(self):
        pass

    def onActionUnmaskAll(self):
        pass

    def onActionMaskLoad(self):
        pass

    def onActionMaskSave(self):
        pass

    def onActionTrimsLoad(self):
        pass

    def onActionTrimsSave(self):
        pass

    def onActionTrimsReset(self):
        pass

    def onActionLoadConfiguration(self):
        settings=QSettings()
        confdir=settings.value("ConfigDirectory",'.')
        dialog = QFileDialog(self,self.tr("Load Timepix3 configuration"),confdir)
        dialog.setNameFilter(self.tr("Timepix3 configuration file (*.t3x)"))
        dialog.setFileMode(QFileDialog.ExistingFile)
        if dialog.exec_():
            fileNames = dialog.selectedFiles()
            dir=dialog.directory().path()
            settings.setValue("ConfigDirectory",dir)
            settings.sync()
            self.tpx.loadConfiguration(fileNames[0])
            self.viewerDACs._regenerate_bitmap()
            self.viewerMask._regenerate_bitmap()
            self.updateDacs()

    def onActionSaveConfiguration(self):
        settings=QSettings()
        confdir=settings.value("ConfigDirectory",'.')
        dialog = QFileDialog(self,self.tr("Save Timepix3 configuration"),confdir)
        dialog.setNameFilter(self.tr("Timepix3 configuration file (*.t3x)"))
        dialog.setFileMode(QFileDialog.AnyFile)
        dialog.setAcceptMode(QFileDialog.AcceptSave)
        if dialog.exec_():
            fileNames = dialog.selectedFiles()
            dir=dialog.directory().path()
            settings.setValue("ConfigDirectory",dir)
            settings.sync()
            self.tpx.saveConfiguration(fileNames[0])

    def onTabPageChange(self):
        if self.tabsMain.currentIndex()==0:
            self.viewerTOT._regenerate_bitmap()
        if self.tabsMain.currentIndex()==1:
            self.viewerCounts._regenerate_bitmap()

        if self.tabsMain.currentIndex()==2:
            self.viewerDACs._regenerate_bitmap()
        if self.tabsMain.currentIndex()==3:
            self.viewerMask._regenerate_bitmap()


    def daqThreadrefreshDisplay(self,data):
        if self.tabsMain.currentIndex()==0:
            self.viewerTOT._regenerate_bitmap();
        if self.tabsMain.currentIndex()==1:
            self.viewerCounts._regenerate_bitmap();

    def TPEnableChanged(self):
        print "Changed"
        self.gcrChanged()

    def onEqualize(self):
        dlg=EqualizeDlg(self)

    def TPSourceChanged(self):
        if self.TPSource.currentIndex()==0:
            self.TPInternalDetails.setVisible(True)
        else:
            self.TPInternalDetails.setVisible(False)
        self.dockTP.adjustSize()

    def onComboShutterTypeChanged(self,i):
        self.tpx.shutterOff()
        self.shutter=0
        self.updateShutter()

    def shutterOnOff(self):
        if self.shutter:
            self.tpx.shutterOff()
            self.shutter=0
        else:
            self.comboShutterType.setEnabled(False)
            self.spinShutterCount.setEnabled(False)
            self.spinShutterFreq.setEnabled(False)
            self.spinShutterLenght.setEnabled(False)

            if self.comboShutterType.currentIndex()==0:
                self.tpx.setShutterConfig(0, 1, 0)
            else:
                self.tpx.setShutterConfig(self.spinShutterLenght.value(), self.spinShutterFreq.value(), self.spinShutterCount.value())
            self.shutter=1
            self.tpx.shutterStart()
        self.updateShutter()

    def updateShutter(self):
        if self.tpx.isConnected():
            self.buttonShutter.setEnabled(True)
            if not self.shutter:
                self.buttonShutter.setText("On")
                self.comboShutterType.setEnabled(True)

                if self.comboShutterType.currentIndex()==0:
                    self.spinShutterCount.setEnabled(False)
                    self.spinShutterLenght.setEnabled(False)
                    self.spinShutterFreq.setEnabled(False)
                else:
                    self.spinShutterCount.setEnabled(True)
                    self.spinShutterFreq.setEnabled(True)
                    self.spinShutterLenght.setEnabled(True)

            else:
                self.buttonShutter.setText("Off")
                self.comboShutterType.setEnabled(False)
                self.spinShutterCount.setEnabled(False)
                self.spinShutterLenght.setEnabled(False)
                self.spinShutterFreq.setEnabled(False)

        else:
            self.buttonShutter.setEnabled(False)
            self.comboShutterType.setEnabled(False)
            self.spinShutterCount.setEnabled(False)
            self.spinShutterFreq.setEnabled(False)
            self.spinShutterLenght.setEnabled(False)

    def thresholdSliderMoved(self):
        nth=self.sliderThreshold.value()
        self.labelThreshold.setText(str(nth))
        self.setThreshold(nth)


    def updateDacWithoutSignal(self,dacname,value):
        dac=getattr(self,"dac_"+dacname.lower())
        tpx_dn="TPX3_"+dacname.upper()
        oldState = dac.blockSignals(True)
        dac.setProperty("value", value)
        self.tpx.setDac(eval(tpx_dn),value)
        dac.blockSignals(oldState)


    def defaults(self):
        self.tpx.resetPixels()
        self.tpx.setDacsDflt()
        self.updateDacWithoutSignal("IBIAS_IKRUM",15)
        self.tpx.setDac(TPX3_VTP_COARSE,50)
        self.tpx.setDac(TPX3_VTP_FINE,112)
        self.tpx.setDac(TPX3_IBIAS_DISCS1_ON,128)
        self.tpx.setDac(TPX3_IBIAS_DISCS2_ON,32)
        self.tpx.setDac(TPX3_IBIAS_PREAMP_ON,128)
        self.tpx.setDac(TPX3_IBIAS_PIXELDAC,128)
        self.tpx.setDac(TPX3_VTHRESH_COARSE,5)
        self.tpx.setDac(TPX3_VFBK,164)
        self.tpx.setDac(TPX3_VTHRESH_FINE,256)
        self.tpx.setPllConfig( TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK \
                         | TPX3_PHASESHIFT_DIV_8 | TPX3_PHASESHIFT_NR_1 \
                         | 0x14<<TPX3_PLLOUT_CONFIG_SHIFT )

        self.tpx.setGenConfig( TPX3_ACQMODE_TOA_TOT | TPX3_GRAYCOUNT_ENA | TPX3_FASTLO_ENA)
        self.tpx.resetPixelConfig()
        self.tpx.load_equalization('x.cod',\
                      maskname='x.msk')
        self.tpx.setPixelMask(95,108,1)
        self.tpx.setPixelMask(153,85,1)
        self.tpx.setPixelMask(161,108,1)
        self.tpx.setPixelMask(45,132,1)
        self.tpx.setPixelMask(132,45,1)
        self.tpx.setPixelConfig()
        self.tpx.setDac(TPX3_VTHRESH,1150)
        self.tpx.datadrivenReadout()

        print "Done", np.sum(self.tpx.matrixMask)

    def gcrChanged(self,index=False):
        gcr=0
        gcr+=self.genConfigPolarity.currentIndex()
        if self.genConfigFastLo.currentIndex() : gcr+=TPX3_FASTLO_ENA
        if self.genConfigGrayCnt.currentIndex() : gcr+=TPX3_GRAYCOUNT_ENA
        if self.genConfigTP.isChecked() : gcr+=TPX3_TESTPULSE_ENA
        self.tpx.setGenConfig(gcr)
        self.updateGcr()

    def updateGcr(self):
        if self.tpx.isConnected():
            gcr=self.tpx.getGenConfig()
            print "GCR 0x%08X"%gcr
            self.genConfigPolarity.setEnabled(True)
            self.genConfigPolarity.setCurrentIndex(gcr&TPX3_POLARITY_EMIN)
#            self.genConfigMode.setEnabled(False)
#            self.genConfigMode.setCurrentIndex(gcr&TPX3_ACQMODE_MASK>>1)
            print "gcr&TPX3_FASTLO_ENA>>6",(gcr&TPX3_FASTLO_ENA)>>6
            self.genConfigFastLo.setCurrentIndex((gcr&TPX3_FASTLO_ENA)>>6)
            self.genConfigFastLo.setEnabled(True)
            self.genConfigGrayCnt.setCurrentIndex((gcr&TPX3_GRAYCOUNT_ENA)>>3)
            self.genConfigGrayCnt.setEnabled(True)
            self.genConfigTP.setChecked ((gcr&TPX3_TESTPULSE_ENA))

    def outputMaskChanged(self):
        om=0
        if self.outputMask0.isChecked(): om+=1<<0
        if self.outputMask1.isChecked(): om+=1<<1
        if self.outputMask2.isChecked(): om+=1<<2
        if self.outputMask3.isChecked(): om+=1<<3
        if self.outputMask4.isChecked(): om+=1<<4
        if self.outputMask5.isChecked(): om+=1<<5
        if self.outputMask6.isChecked(): om+=1<<6
        if self.outputMask7.isChecked(): om+=1<<7
        self.tpx.setOutputMask(om)
        self.updateOutputLinks()

    def updateOutputLinks(self):
        cnf=self.tpx.getOutBlockConfig()
        self.outputMask0.setEnabled(True)
        self.outputMask1.setEnabled(True)
        self.outputMask2.setEnabled(True)
        self.outputMask3.setEnabled(True)
        self.outputMask4.setEnabled(True)
        self.outputMask5.setEnabled(True)
        self.outputMask6.setEnabled(True)
        self.outputMask7.setEnabled(True)
        self.outputMask0.setChecked(cnf & (1<<0))
        self.outputMask1.setChecked(cnf & (1<<1))
        self.outputMask2.setChecked(cnf & (1<<2))
        self.outputMask3.setChecked(cnf & (1<<3))
        self.outputMask4.setChecked(cnf & (1<<4))
        self.outputMask5.setChecked(cnf & (1<<5))
        self.outputMask6.setChecked(cnf & (1<<6))
        self.outputMask7.setChecked(cnf & (1<<7))





    def onDacChanged(self, n,val):
        self.tpx.setDac(n,val)

    def updateDacs(self):
        for dn in self.all_dacs:
            dac=getattr(self,dn)
            tpx_dn="TPX3_"+dn[4:].upper()
            dval=self.tpx.getDac(eval(tpx_dn))
            #dac.setValue(dval)
            oldState = dac.blockSignals(True)
            dac.setProperty("value", dval)
            dac.blockSignals(oldState)
            dac.setEnabled(True)
            maxval=self.tpx.dacMax(eval(tpx_dn))
            dac.setMaximum(maxval)
        #self.sliderThreshold.setEnabled(True)

    def readoutChanged(self):
        self.tpx.pauseReadout()

        if self.radioReadoutDataDriven.isChecked():
            self.tpx.datadrivenReadout()
        elif self.radioReadoutSeq.isChecked():
            self.tpx.sequentialReadout(32)
        else:
            pass
    def updateMenu(self):
        if self.tpx.isConnected():
            self.actionSaveConfiguration.setEnabled(True)
            self.actionLoadConfiguration.setEnabled(True)
            self.actionEqualize.setEnabled(True)
            self.menuTrims.setEnabled(True)
            self.menuMask.setEnabled(True)
            self.actionEqualize.setEnabled(True)

    def updateDisplays(self):
        self.updateGcr()
        self.updateShutter()
        self.updateOutputLinks()
        self.updateDacs()
        self.updateMenu()

    def initAfterConnect(self):
        self.shutter=0

    def closeEvent(self, event):
        if self.shutter:
            self.tpx.stopAutoTrigger()
            self.shutter=0

        settings=QSettings();
        settings.beginGroup("MainWindow");
        settings.setValue("size", self.size());
        settings.setValue("pos", self.pos());
        settings.endGroup();

    #def connectOrDisconnect(self):
    def onComboDemoTypeChanged(self,e):
        self.tpx.setMode(e)
    def onConnectDemo(self):
                    self.tpx=DummyTPX3()
                    self.spinDemoGenRate.valueChanged.connect(self.onSpinDemoGenRateChanged)
                    self.comboDemoType.currentIndexChanged['int'].connect(self.onComboDemoTypeChanged)
                    self.onSpinDemoGenRateChanged()

                    self.spinDemoGenRate.setEnabled(True)
                    self.tpx.daqThread.updateRate.sig.connect(self.dockHitRate.UpdateRate)
                    self.tpx.daqThread.refreshDisplay.sig.connect(self.daqThreadrefreshDisplay)

                    self.viewerTOT.setData(self.tpx.matrixTOT)
                    self.viewerTOT.cm.setHMin(0)
                    self.viewerTOT.cm.setHMax(128)
                    self.viewerTOT.tpx=self.tpx

                    self.viewerMask.setData(self.tpx.matrixMask)
                    self.viewerMask.cm.setHMin(0)
                    self.viewerMask.cm.setHMax(1)
                    self.viewerMask.tpx=self.tpx

                    self.viewerDACs.setData(self.tpx.matrixDACs)
                    self.viewerDACs.cm.setHMin(0)
                    self.viewerDACs.cm.setHMax(15)
                    self.viewerDACs.tpx=self.tpx

                    self.viewerCounts.setData(self.tpx.matrixCounts)
                    self.viewerCounts.cm.setHMin(0)
                    self.viewerCounts.cm.setHMax(200)
                    self.viewerCounts.tpx=self.tpx

                    self.tpx.daqThread.start()

    def onSpinDemoGenRateChanged(self):
        self.tpx.setRate(self.spinDemoGenRate.value())

    def onConnectSPIDR(self):
        if self.tpx:
            pass
        else:
            ip_list="192.168.100.10".split('.')
            port="50000"
            try:
                self.tpx=TPX3(daq="custom")
                s=""
                if self.tpx.isConnected():
                    s="<font color='green'> %s<font>"%self.tpx.connectionStateString()
                    self.initAfterConnect()
                    self.updateDisplays()
                    self.connectrionMessage.setText(s)
                    self.tpx.daqThread.updateRate.sig.connect(self.dockHitRate.UpdateRate)
                    self.tpx.daqThread.refreshDisplay.sig.connect(self.daqThreadrefreshDisplay)

                    self.viewerTOT.setData(self.tpx.matrixTOT)
                    self.viewerTOT.cm.setHMin(0)
                    self.viewerTOT.cm.setHMax(100)
                    self.viewerTOT.tpx=self.tpx

                    self.viewerMask.setData(self.tpx.matrixMask)
                    self.viewerMask.cm.setHMin(0)
                    self.viewerMask.cm.setHMax(1)
                    self.viewerMask.tpx=self.tpx

                    self.viewerDACs.setData(self.tpx.matrixDACs)
                    self.viewerDACs.cm.setHMin(0)
                    self.viewerDACs.cm.setHMax(15)
                    self.viewerDACs.tpx=self.tpx

                    self.viewerCounts.setData(self.tpx.matrixCounts)
                    self.viewerCounts.cm.setHMin(0)
                    self.viewerCounts.cm.setHMax(200)
                    self.viewerCounts.tpx=self.tpx

                    self.tpx.daqThread.start()

            except RuntimeError as n:
                self.tpx=None
                self.connectrionMessage.setText( "<font color='red'>Unconected</font>")
                msgBox = QMessageBox()
                msgBox.setText(str(n))
                msgBox.exec_()

if __name__=="__main__":
    app = QApplication(sys.argv)
    form=MainWindow()
    app.processEvents()
    form.show()
    app.exec_()

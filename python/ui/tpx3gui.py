#!/usr/bin/env python
from PySide.QtCore import *
from PySide.QtGui import *
from MainWnd import Ui_MainWindow
import sys
from SpidrTpx3_engine import *
import numpy as np
import scipy.ndimage as ndi
import random
import os
import time



class DaqThread(QThread):
    def __init__(self, parent=None):
        QThread.__init__(self, parent)
        self.parent=parent
        self.abort = False
        self.data=None
    def stop(self):
        #self.mutex.lock()
        self.abort = True
        #self.condition.wakeOne()
        #self.mutex.unlock()
        #self.wait()
    def run(self):

        total_hits=0
        last_time=time.time()
        while True:
            if self.abort:
                return

            next_frame=self.parent.spidrDaq.getSample(1024*128,1)
            self.data*=0.98

            if next_frame:
                hits=self.parent.spidrDaq.sampleSize()/8
                now=time.time()
                dt=float(now-last_time)
                last_time=now
                rate=float(hits)/dt
                total_hits+=hits
                self.parent.labelRate.setText("Rate : <b>%.1f</b> Hz (total hits %d)"%(rate,total_hits))

                while True:
                    r,x,y,data,tstp=self.parent.spidrDaq.nextPixel()
                    if not r: break
                    data>>=4
                    data&=0x2FF
                    #print x,y,data
                    self.data[x][y]+=data
                    #print x,y,data
                # if 0:
                #    for i in range(10):
                #     px=random.randint(2,253)
                #     py=random.randint(2,253)
                #     amp=random.randint(50,500)
                #     if self.data!=None:
                #         self.data[px][py]+=amp
                #         self.data[px-1][py-1]+=amp/3
                #         self.data[px-1][py+1]+=amp/3
                #         self.data[px+1][py-1]+=amp/3
                #         self.data[px+1][py+1]+=amp/3
                #         self.data[px-1][py]+=amp/2
                #         self.data[px+1][py]+=amp/2
                #         self.data[px][py+1]+=amp/2
                #         self.data[px][py-1]+=amp/2

            if self.parent:
               self.parent.updateViewer()
            #self.daqThread.data=

class MainWindow(QMainWindow, Ui_MainWindow):
    def __init__(self,parent=None):
        super(MainWindow, self).__init__(parent)
        self.setupUi(self)
        self.spidrController=None
        self.connect(self.buttonConnect,SIGNAL("clicked()"), self.connectOrDisconnect)
        self.genConfigTP.currentIndexChanged['QString'].connect(self.gcrChanged)
        self.genConfigPolarity.currentIndexChanged['QString'].connect(self.gcrChanged)
        self.genConfigAckCmd.currentIndexChanged['QString'].connect(self.gcrChanged)
        self.genConfigFastLo.currentIndexChanged['QString'].connect(self.gcrChanged)
        self.genConfigGrayCnt.currentIndexChanged['QString'].connect(self.gcrChanged)
        self.genConfigMode.currentIndexChanged['QString'].connect(self.gcrChanged)
        self.genConfigSelectTP.currentIndexChanged['QString'].connect(self.gcrChanged)
        self.genConfigTimeroverflow .currentIndexChanged['QString'].connect(self.gcrChanged)
        self.genConfigTpSource.currentIndexChanged['QString'].connect(self.gcrChanged)
        self.connect(self.buttonShutter ,SIGNAL("clicked()"), self.shutterOnOff)
        self.outputMask0.stateChanged.connect(self.outputMaskChanged)
        self.outputMask1.stateChanged.connect(self.outputMaskChanged)
        self.outputMask2.stateChanged.connect(self.outputMaskChanged)
        self.outputMask3.stateChanged.connect(self.outputMaskChanged)
        self.outputMask4.stateChanged.connect(self.outputMaskChanged)
        self.outputMask5.stateChanged.connect(self.outputMaskChanged)
        self.outputMask6.stateChanged.connect(self.outputMaskChanged)
        self.outputMask7.stateChanged.connect(self.outputMaskChanged)
        self.connect(self.radioReadoutSeq ,SIGNAL("clicked()"), self.readoutChanged)
        self.connect(self.radioReadoutDataDriven ,SIGNAL("clicked()"), self.readoutChanged)
        self.connect(self.radioReadoutOff ,SIGNAL("clicked()"), self.readoutChanged)
        self.devid=0
        self.shutter=0
        self.all_dacs=["dac_ibias_preamp_on","dac_ibias_preamp_off","dac_vpreamp_ncas","dac_ibias_ikrum","dac_vfbk",\
                     "dac_vthresh_fine","dac_vthresh_coarse","dac_ibias_discs1_on","dac_ibias_discs1_off",\
                     "dac_ibias_discs2_on","dac_ibias_discs2_off","dac_ibias_pixeldac","dac_ibias_tpbufin",\
                     "dac_ibias_tpbufout","dac_vtp_coarse","dac_vtp_fine"]
        self.connect(self.buttonConfigure ,SIGNAL("clicked()"), self.matrixConfigure)
        self.connect(self.buttonDefaults ,SIGNAL("clicked()"), self.defaults)

        self.sliderThreshold.valueChanged.connect(self.thresholdSliderMoved)


        for dn in self.all_dacs:
            dac=getattr(self,dn)
            tpx_dn="TPX3_"+dn[4:].upper()
            dac.valueChanged.connect(lambda :self.onDacChanged(eval(tpx_dn)))
        self.daqThread = DaqThread(self)

    def load_equalization(self,fname,maskname=""):
        def load(fn):
          f=open(fn,"r")
          ret=[]
          for l in f.readlines():
            ll=[]
            for n in l.split():
              n=int(n)
              ll.append(n)
            ret.append(ll)
          f.close()
          return ret
        eq=load(fname)
        if maskname : mask=load(maskname)
        self.spidrController.resetPixelConfig()
        for x in range(256):
          for y in range(256):
              self.spidrController.setPixelThreshold(x,y,eq[y][x])
              if maskname and mask[y][x]:
                self.spidrController.setPixelMask(x,y,mask[y][x])

    def thresholdSliderMoved(self):
        nth=self.sliderThreshold.value()
        self.labelThreshold.setText(str(nth))
        self.setThreshold(nth)

    def setThreshold(self,dac_value=1000):
        """ Xavi's treshold """
        i=0
        coarse_found=0
        fine_found=352
        for coarse in range(16):
           for fine in range(352,512,1):
              if dac_value==i:
                 coarse_found=coarse
                 fine_found=fine
              i+=1
#        self.spidrController.setDac(self.devid,TPX3_VTHRESH_COARSE,coarse_found)
#        self.spidrController.setDac(self.devid,TPX3_VTHRESH_FINE,fine_found)
        self.updateDacWithoutSignal("VTHRESH_COARSE",coarse_found)
        self.updateDacWithoutSignal("VTHRESH_FINE",fine_found)

    def updateDacWithoutSignal(self,dacname,value):
        dac=getattr(self,"dac_"+dacname.lower())
        tpx_dn="TPX3_"+dacname.upper()
        oldState = dac.blockSignals(True)
        dac.setProperty("value", value)
        self.spidrController.setDac(self.devid,eval(tpx_dn),value)
        dac.blockSignals(oldState)


    def matrixConfigure(self):
        print os.getcwd()

    def defaults(self):
        self.spidrController.resetPixels(self.devid)
        self.spidrController.setDacsDflt(self.devid)
        self.updateDacWithoutSignal("IBIAS_IKRUM",15)
        self.spidrController.setDac(self.devid,TPX3_VTP_COARSE,50)
        self.spidrController.setDac(self.devid,TPX3_VTP_FINE,112)
        self.spidrController.setDac(self.devid,TPX3_IBIAS_DISCS1_ON,128)
        self.spidrController.setDac(self.devid,TPX3_IBIAS_DISCS2_ON,32)
        self.spidrController.setDac(self.devid,TPX3_IBIAS_PREAMP_ON,128)
        self.spidrController.setDac(self.devid,TPX3_IBIAS_PIXELDAC,128)
        self.spidrController.setDac(self.devid,TPX3_VTHRESH_COARSE,5)
        self.spidrController.setDac(self.devid,TPX3_VFBK,164)
        self.spidrController.setDac(self.devid,TPX3_VTHRESH_FINE,256)
        self.spidrController.setPllConfig( self.devid,TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK \
                         | TPX3_PHASESHIFT_DIV_8 | TPX3_PHASESHIFT_NR_1 \
                         | 0x14<<TPX3_PLLOUT_CONFIG_SHIFT )

        self.spidrController.setGenConfig( self.devid,TPX3_ACQMODE_TOA_TOT | TPX3_GRAYCOUNT_ENA | TPX3_FASTLO_ENA)
        self.spidrController.resetPixelConfig()
        self.load_equalization('../calib/eq_codes.dat',\
                      maskname='../calib/eq_mask.dat')
        self.spidrController.setPixelMask(95,108,1)
        self.spidrController.setPixelMask(85,153,1)
        self.spidrController.setPixelMask(108,161,1)
        self.spidrController.setPixelMask(45,132,1)
        self.spidrController.setPixelConfig(self.devid)
        self.spidrController.setDecodersEna(True)
        self.setThreshold(1150)
        print "Done"
    def gcrChanged(self,index):
        gcr=0
        gcr+=self.genConfigPolarity.currentIndex()
        self.updateGcr()

    def updateGcr(self):
        if self.spidrController.isConnected():
            r,gcr=self.spidrController.getGenConfig(self.devid)
            if r:
                self.genConfigValue.setText("0x%04X"%gcr)
                self.genConfigPolarity.setEnabled(True)
                self.genConfigPolarity.setCurrentIndex(gcr&TPX3_POLARITY_EMIN)
                self.genConfigMode.setEnabled(True)
                self.genConfigMode.setCurrentIndex(gcr&TPX3_ACQMODE_MASK>>1)
                self.genConfigTP.setEnabled(True)
                self.genConfigTP.setCurrentIndex( gcr&TPX3_TESTPULSE_ENA != 0 )

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
        self.spidrController.setOutputMask(self.devid,om)
        self.updateOutputLinks()

    def updateOutputLinks(self):
        r,cnf=self.spidrController.getOutBlockConfig(self.devid)
        if r:
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
            self.outputMaskTxt.setText("0x%02X"%(cnf&TPX3_OUTPORT_MASK))
            print cnf

    def updateShutter(self):
        if self.spidrController.isConnected():
            self.buttonShutter.setEnabled(True)
            if not self.shutter:
                self.buttonShutter.setText("On")
            else:
                self.buttonShutter.setText("Off")
        else:
            self.buttonShutter.setEnabled(False)

    def shutterOnOff(self):
        if self.shutter:
            self.spidrController.stopAutoTrigger()
            self.shutter=0
        else:
            self.spidrController.startAutoTrigger()
            self.shutter=1
        self.updateShutter()

    def onDacChanged(self, n):
        print('DAC %d changed to '%(n))
        val=0
        self.spidrController.setDac(n,val)

    def updateDacs(self):
        for dn in self.all_dacs:
            dac=getattr(self,dn)
            tpx_dn="TPX3_"+dn[4:].upper()
            r,dval=self.spidrController.getDac(self.devid,eval(tpx_dn))
            if r:
                #dac.setValue(dval)
                oldState = dac.blockSignals(True)
                dac.setProperty("value", dval)
                dac.blockSignals(oldState)
                #print dn,dval
                dac.setEnabled(True)
                maxval=self.spidrController.dacMax(eval(tpx_dn))
                dac.setMaximum(maxval)
        self.sliderThreshold.setEnabled(True)

    def readoutChanged(self):
        self.spidrController.pauseReadout()

        if self.radioReadoutDataDriven.isChecked():
            self.spidrController.datadrivenReadout()
        elif self.radioReadoutSeq.isChecked():
            self.spidrController.sequentialReadout(32)
        else:
            pass
    def updateReadout(self):
        self.radioReadoutDataDriven.setEnabled(True)
        self.radioReadoutSeq.setEnabled(True)
        self.radioReadoutOff.setEnabled(True)

    def updateDisplays(self):
        self.updateGcr()
        self.updateShutter()
        self.updateOutputLinks()
        self.updateDacs()
        self.updateReadout()
    def initAfterConnect(self):
        self.spidrController.setTriggerConfig(4,0,1,0)
        self.shutter=0

    def closeEvent(self, event):
        if self.shutter:
            self.spidrController.stopAutoTrigger()
            self.shutter=0

    def updateViewer(self):
        self.viewer._regenerate_bitmap();

    def connectOrDisconnect(self):
        if self.spidrController:
            pass
        else:
            ip_list=self.ipAddress.text().split('.')
            port=self.port.text()
            self.spidrController=SpidrController(int(ip_list[0]),int(ip_list[1]),int(ip_list[2]),int(ip_list[3]),int(port))
            s=""
            if self.spidrController.isConnected():
                s="<font color='green'> %s<font>"%self.spidrController.connectionStateString()
                self.initAfterConnect()
                self.updateDisplays()
                self.spidrDaq=SpidrDaq(self.spidrController)
                if self.spidrDaq.errorString():
                    print self.spidrDaq.errorString()
                self.spidrDaq.setSampling(True)
                self.spidrDaq.setSampleAll(True )
                self.matrix = np.zeros( shape=(256,256))
                self.viewer.setData(self.matrix)
                self.viewer.cm.min=0
                self.viewer.cm.max=50
                self.daqThread.start()
                self.daqThread.data=self.matrix

            else:
                s="<font color='red'> %s : %s<font>"%(self.spidrController.connectionStateString(),self.spidrController.connectionErrString())
                self.spidrController=None
            self.connectrionMessage.setText(s)

if __name__=="__main__":
    app = QApplication(sys.argv)

    form=MainWindow()
    form.show()
    app.exec_()
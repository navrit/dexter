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



QCoreApplication.setOrganizationName("CERN");
QCoreApplication.setApplicationName("t3g");
import scipy.ndimage as ndi
import random
import os
import time
from hitratedock import HitRateDock

class MySignal(QObject):
    sig = Signal(str)
#
# class SpidrControllerEx(SpidrController):
#     def getAdcEx(self,measurements):
#         ret,val=self.getAdc(self.id,measurements)
#         val=float(val)/measurements
#         val=1.5*val/4096
#         return val

class DummyDaq:
    def __init__(self):
        self.name="Dummy"
        self.n=0
        self.x=0
        self.y=0
        self.data=[]
        self.rate=10
        self.n=1.0
    def errorString(self):
        return ""

    def setSampling(self,b=False):
        pass

    def setSampleAll(self,b=False):
        pass

    def getSample2(self,n,timeout):
        return self.getSample(n,timeout)
    
    def getSample(self,n,timeout):

        def line(x0, y0, x1, y1):
            "Bresenham's line algorithm"
            dx = abs(x1 - x0)
            dy = abs(y1 - y0)
            x, y = x0, y0
            sx = -1 if x0 > x1 else 1
            sy = -1 if y0 > y1 else 1
            if dx > dy:
                err = dx / 2.0
                while x != x1:
                    yield (x, y)
                    err -= dy
                    if err < 0:
                        y += sy
                        err += dx
                    x += sx
            else:
                err = dy / 2.0
                while y != y1:
                    yield (x, y)
                    err -= dx
                    if err < 0:
                        x += sx
                        err += dy
                    y += sy
            yield (x, y)
        time.sleep(float(timeout)/1e3)
        self.n+=(float(self.rate)/100)

        while int(self.n)>0:

            t=random.randint(0,1)


            if t==0:
                px=random.randint(2,253)
                py=random.randint(2,253)
                amp=random.randint(10,1000)

                self.data.append( (px,py,amp))
                self.data.append( (px+1,py,amp/2))
                self.data.append( (px-1,py,amp/2))
                self.data.append( (px,py-1,amp/2))
                self.data.append( (px,py+1,amp/2))

                self.data.append( (px+2,py,amp/5))
                self.data.append( (px-2,py,amp/5))
                self.data.append( (px,py-2,amp/5))
                self.data.append( (px,py+2,amp/5))

                self.data.append( (px+1,py+1,amp/4))
                self.data.append( (px+1,py-1,amp/4))
                self.data.append( (px-1,py+1,amp/4))
                self.data.append( (px-1,py-1,amp/4))
            elif t==1:
                px1=random.randint(2,253)
                py1=random.randint(2,253)
                py2=py1+60-random.randint(0,40)
                if py2>255:py2=255
                px2=px1+10-random.randint(0,20)
                px2=max((min((px2,255)) , 0))
                amp=random.randint(50,500)
                for x,y in line(px1,py1,px2,py2):
                  self.data.append( (y,x,amp))
                  amp+=random.randint(-10,40)

            self.n-=1

        if len(self.data)>0:
            return True
        else:
            return False

    def sampleSize(self):

        return len(self.data)*8

    def nextPixel(self):
        if len(self.data)>0:
            x,y,d=self.data.pop(0)
            return True,x,y,d<<4,0
        else:
            return False,0,0,0,0


class Rate():
    def __init__(self,refresh=0.02,updateRateSignal=None,refreshDisplaySignal=None):
        self.refresh=refresh
        self.total_events=0
        self.new_events=0
        self.last_ref_time=time.time()
        self.last_s_time=self.last_ref_time
        self.updateRateSignal=updateRateSignal
        self.refreshDisplaySignal=refreshDisplaySignal
    def processed(self, events):
        now=time.time()
        self.new_events+=events

        # refresh
        dt=now-self.last_ref_time
        if dt>self.refresh and self.refreshDisplaySignal:
            self.last_ref_time=now
            self.refreshDisplaySignal.sig.emit("Now")
        # report rate
        dt=now-self.last_s_time
        if dt>1.0 and self.updateRateSignal:
            rate=self.new_events/dt
            self.total_events+=self.new_events
            self.last_s_time=now
            self.new_events=0
            self.updateRateSignal.sig.emit("%.3f"%rate)

class DaqThread(QThread):
    def __init__(self, parent=None):
        QThread.__init__(self)
        self.parent=parent
        self.abort = False
        self.data=None
        self.updateRate = MySignal()
        self.refreshDisplay = MySignal()
        self.rate=Rate(refresh=0.05,updateRateSignal=self.updateRate, refreshDisplaySignal=self.refreshDisplay)
    def stop(self):
        #self.mutex.lock()
        self.abort = True
        #self.condition.wakeOne()
        #self.mutex.unlock()
        #self.wait()
    def __del__(self):
        print "Wating ..."
        self.wait()

    def run(self):
#        total_hits=0
#        last_time=time.time()
#        ref_last=time.time()
        print "Starting data taking thread"
        #prev_ref=0
        #msg=""
        self.abort=False
        while True:
            if self.abort:
                return

            self.data*=0.98
        #if 0:
            low_values_indices = self.data < 1.0  # Where values are low
            self.data[low_values_indices] = 0  # All low values set to 0

            next_frame=self.parent.tpx.getSample(1024,10)
            #next_frame=self.parent.spidrDaq.getSample(100,10)
            self.rate.processed(0)
            #print next_frame
            if next_frame:
               time.sleep(0.005)

               #hits=self.parent.spidrDaq.sampleSize()/8
#               print hits
               hits=0
               while True:
                   r,x,y,data,tstp=self.parent.tpx.nextPixel()
                   if not r: break
                   data>>=4
                   data&=0x2FF
#                   print x,y,data
                   self.data[x][y]+=data
                   hits+=1
               self.rate.processed(hits)


class MainWindow(QMainWindow, Ui_MainWindow):
    def __init__(self,parent=None):
        super(MainWindow, self).__init__(parent)
        self.setupUi(self)
        self.tpx=None
#        self.connect(self.buttonConnect,SIGNAL("clicked()"), self.connectOrDisconnect)
     #   self.genConfigTP.currentIndexChanged['QString'].connect(self.gcrChanged)
        self.genConfigPolarity.currentIndexChanged['QString'].connect(self.gcrChanged)
#        self.genConfigAckCmd.currentIndexChanged['QString'].connect(self.gcrChanged)
        self.genConfigFastLo.currentIndexChanged['QString'].connect(self.gcrChanged)
        self.genConfigGrayCnt.currentIndexChanged['QString'].connect(self.gcrChanged)
        self.genConfigMode.currentIndexChanged['QString'].connect(self.gcrChanged)
     #   self.genConfigSelectTP.currentIndexChanged['QString'].connect(self.gcrChanged)
        self.genConfigTimeroverflow .currentIndexChanged['QString'].connect(self.gcrChanged)
        #self.TPSource.currentIndexChanged['QString'].connect(self.gcrChanged)
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
                     "dac_vthresh_fine","dac_vthresh_coarse","dac_ibias_discs1_on","dac_ibias_discs1_off",\
                     "dac_ibias_discs2_on","dac_ibias_discs2_off","dac_ibias_pixeldac","dac_ibias_tpbufin",\
                     "dac_ibias_tpbufout","dac_vtp_coarse","dac_vtp_fine"]
        self.connect(self.buttonDefaults ,SIGNAL("clicked()"), self.defaults)
        self.TPInternalDetails.setVisible(False)
        self.genConfigTP.stateChanged.connect(self.TPEnableChanged)
        self.TPSource.currentIndexChanged.connect(self.TPSourceChanged)
        #self.sliderThreshold.valueChanged.connect(self.thresholdSliderMoved)
        #self.groupBox.setVisible(False)

        for dn in self.all_dacs:
            dac=getattr(self,dn)
            tpx_dn="TPX3_"+dn[4:].upper()
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


        settings = QSettings()

        settings.beginGroup("MainWindow")
        self.resize(settings.value("size", QSize(400, 400)))
        self.move(settings.value("pos", QPoint(200, 200)))
        settings.endGroup()

        settings.setValue("runs", int(settings.value("runs", 0))+1)
        self.action_Equalize.triggered.connect(self.onEqualize)

        self.daqThread = DaqThread(self)
        self.daqThread.updateRate.sig.connect(self.dockHitRate.UpdateRate)
        self.daqThread.refreshDisplay.sig.connect(self.daqThreadrefreshDisplay)


    def daqThreadrefreshDisplay(self,data):
        self.updateViewer()

    def TPEnableChanged(self):
        print "Changed"

    def onEqualize(self):
        dlg=EqualizeDlg(self)


    def TPSourceChanged(self):
        if self.TPSource.currentIndex()==0:
            self.TPInternalDetails.setVisible(True)
        else:
            self.TPInternalDetails.setVisible(False)
        self.dockTP.adjustSize()

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
        self.tpx.resetPixelConfig()
        for x in range(256):
          for y in range(256):
              self.tpx.setPixelThreshold(x,y,eq[y][x])
              if maskname and mask[y][x]:
                self.tpx.setPixelMask(x,y,mask[y][x])

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
#        self.tpx.setDac(TPX3_VTHRESH_COARSE,coarse_found)
#        self.tpx.setDac(TPX3_VTHRESH_FINE,fine_found)
        self.updateDacWithoutSignal("VTHRESH_COARSE",coarse_found)
        self.updateDacWithoutSignal("VTHRESH_FINE",fine_found)

    def updateDacWithoutSignal(self,dacname,value):
        dac=getattr(self,"dac_"+dacname.lower())
        tpx_dn="TPX3_"+dacname.upper()
        oldState = dac.blockSignals(True)
        dac.setProperty("value", value)
        self.tpx.setDac(eval(tpx_dn),value)
        dac.blockSignals(oldState)


    def matrixConfigure(self):
        print os.getcwd()

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
        self.load_equalization('../calib/eq_codes.dat',\
                      maskname='../calib/eq_mask.dat')
        self.tpx.setPixelMask(95,108,1)
        self.tpx.setPixelMask(153,85,1)
        self.tpx.setPixelMask(161,108,1)
        self.tpx.setPixelMask(45,132,1)
        self.tpx.setPixelMask(132,45,1)
        self.tpx.setPixelConfig()
        self.setThreshold(1150)
        self.tpx.datadrivenReadout()
        print "Done"

    def gcrChanged(self,index):
        gcr=0
        gcr+=self.genConfigPolarity.currentIndex()
        self.updateGcr()

    def updateGcr(self):
        if self.tpx.isConnected():
            gcr=self.tpx.getGenConfig()
#                self.genConfigValue.setText("0x%04X"%gcr)
            self.genConfigPolarity.setEnabled(True)
            self.genConfigPolarity.setCurrentIndex(gcr&TPX3_POLARITY_EMIN)
            self.genConfigMode.setEnabled(True)
            self.genConfigMode.setCurrentIndex(gcr&TPX3_ACQMODE_MASK>>1)
            self.genConfigTP.setChecked (gcr&TPX3_TESTPULSE_ENA != 0 )

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
        print cnf

    def updateShutter(self):
        if self.tpx.isConnected():
            self.buttonShutter.setEnabled(True)
            if not self.shutter:
                self.buttonShutter.setText("On")
            else:
                self.buttonShutter.setText("Off")
        else:
            self.buttonShutter.setEnabled(False)

    def shutterOnOff(self):
        if self.shutter:
            self.tpx.shutterOff()
            self.shutter=0
        else:
            self.tpx.shutterOn()
            self.shutter=1
        self.updateShutter()

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
            #print dn,dval
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

    def updateDisplays(self):
        self.updateGcr()
        self.updateShutter()
        self.updateOutputLinks()
        self.updateDacs()

    def initAfterConnect(self):
        self.shutter=0

    def closeEvent(self, event):
        if self.shutter:
            self.tpx.stopAutoTrigger()
            self.shutter=0

    def updateViewer(self):
        self.viewer._regenerate_bitmap();

    def closeEvent(self,event):
        settings=QSettings();
        settings.beginGroup("MainWindow");
        settings.setValue("size", self.size());
        settings.setValue("pos", self.pos());
        settings.endGroup();

    #def connectOrDisconnect(self):

    def onConnectDemo(self):
                self.spidrDaq=DummyDaq()
                if self.spidrDaq.errorString():
                    print self.spidrDaq.errorString()
                self.spidrDaq.setSampling(True)
                self.spidrDaq.setSampleAll(True )
                self.matrix = np.zeros( shape=(256,256))
                self.viewer.setData(self.matrix)
                self.viewer.cm.min=0
                self.viewer.cm.max=1024
                self.daqThread.data=self.matrix
                self.daqThread.start()
                self.spinDemoGenRate.valueChanged.connect(self.onSpinDemoGenRateChanged)
                self.spinDemoGenRate.setEnabled(True)
    def onSpinDemoGenRateChanged(self):
        self.spidrDaq.rate=self.spinDemoGenRate.value()

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

                    self.matrix = np.zeros( shape=(256,256))
                    self.viewer.setData(self.matrix)
                    self.viewer.cm.min=0
                    self.viewer.cm.max=50
                    self.daqThread.data=self.matrix
                    self.daqThread.start()
                    self.connectrionMessage.setText(s)

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

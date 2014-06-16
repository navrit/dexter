#!/usr/bin/env python
from PySide.QtCore import *
from PySide.QtGui import *
from EqualizeForm import Ui_EqualizeForm
import time
import sys
import socket
import getpass
from tpx3 import *
import numpy as np
from kutils import *

class MySignal(QObject):
    sig = Signal(str)



def _equalize(scans):
  bestValue=np.zeros(scans[0].shape)
  bestCode=np.zeros(scans[0].shape, dtype=int)
  maskPixels=np.zeros(scans[0].shape, dtype=int)

  avr=[[],[]]
  avrmean=[]
  X,Y=scans[0].shape
  for si,s in enumerate(scans):
    for x in range(X):
        for y in range(Y):
            if s[x][y]>0:
               avr[si].append(s[x][y])
    avrmean.append(np.average(avr[si]))

  target=np.average(avrmean)
  print "TARGET=%.2f"%(target)
  X,Y=scans[0].shape
  for x in range(X):
    for y in range(Y):
      step=(abs(scans[0][x][y]-scans[1][x][y]))/15
      bc=0
      bv=scans[0][x][y]
      for i in range(16):
        if abs(bv-target) > abs((step*i+scans[0][x][y])-target):
          bc=i
          bv=step*i+scans[0][x][y]
      bestValue[x][y]=bv
      bestCode[x][y]=bc
      if abs(bestValue[x][y]-target) > step :
        maskPixels[x][y]=1
        bestValue[x][y]=0
        bestCode[x][y]=0
      else:
        maskPixels[x][y]=0
  return bestValue,bestCode,maskPixels


class EqualizeThread(QThread):
    def __init__(self, parent=None,tpx=None):
        QThread.__init__(self, parent)
        self.parent=parent
        self.abort = False
        self.tpx=tpx
        self.done = MySignal()

    def stop(self):
        self.abort = True
        self.parent.buttonEqualize.setEnabled(True)

    def log(self,s):
        self.parent.textEdit.append(s)

    def run(self):
        _id=0
        fbase=self.parent.lineDirectory.text() +QDir.separator()+self.parent.lineFileName.text()

        self.log("Starting...")

        self.tpx.resetPixels()
        self.tpx.setDacsDflt()

        self.tpx.setDac(TPX3_IBIAS_IKRUM,15)
        self.tpx.setDac(TPX3_IBIAS_DISCS1_ON,128)
        self.tpx.setDac(TPX3_IBIAS_DISCS2_ON,32)
        self.tpx.setDac(TPX3_IBIAS_PREAMP_ON,128)
        self.tpx.setDac(TPX3_IBIAS_PIXELDAC,128)
        self.tpx.setDac(TPX3_VTHRESH_COARSE,7)
        self.tpx.setDac(TPX3_VTHRESH_FINE,256)

        self.tpx.setGenConfig( TPX3_ACQMODE_EVT_ITOT | TPX3_GRAYCOUNT_ENA)
        self.tpx.setPllConfig( (TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK | TPX3_PHASESHIFT_DIV_8 | TPX3_PHASESHIFT_NR_16 | 0x14<<TPX3_PLLOUT_CONFIG_SHIFT) )
        self.tpx.setCtprBits(0)
        self.tpx.setCtpr()

        self.tpx.resetPixelConfig()
        self.tpx.setPixelMask(ALL_PIXELS,ALL_PIXELS,1)
        self.tpx.setPixelThreshold(ALL_PIXELS,ALL_PIXELS,8)
        self.tpx.pauseReadout()
        self.tpx.setPixelConfig()
        self.log("Matrix configured ...")

        self.tpx.sequentialReadout(tokens=4)
        #self.tpx.flush_udp_fifo(0x71FF000000000000)#flush until load matrix

        self.tpx.setSenseDac(TPX3_VTHRESH_COARSE)
        vthcorse=self.tpx.getAdcEx(1)
        time.sleep(0.001)
        vthcorse=self.tpx.getAdcEx(8)
        self.log("TPX3_VTHRESH_COARSE code=7 voltage=%.1f mV"%(1000.0*vthcorse))
        if self.abort:  return

        best_vfbk_val=0
        best_vfbk_code=0
        self.tpx.setSenseDac(TPX3_VFBK)
        for code in range(64,192):
            self.tpx.setDac(TPX3_VFBK,code)
            time.sleep(0.001)
            vfbk=self.tpx.getAdcEx(8)
            if abs(vfbk-vthcorse)<abs(best_vfbk_val-vthcorse):
                best_vfbk_val=vfbk
                best_vfbk_code=code
        self.tpx.setDac(TPX3_VFBK,best_vfbk_code)
        vfbk=self.tpx.getDacVoltage(TPX3_VFBK)
        self.log("TPX3_VFBK code=%d voltage=%.1f mV"%(best_vfbk_code,(1000.0*vfbk)))
        if self.abort:  return

        best_vthfine_val=0
        best_vthfine_code=0
        self.tpx.setSenseDac(TPX3_VTHRESH_FINE)
        for code in range(200,300):
          self.tpx.setDac(TPX3_VTHRESH_FINE,code)
          time.sleep(0.001)
          vthfine=self.tpx.getAdcEx(8)
          if abs(vfbk-vthfine)<abs(best_vthfine_val-vthfine):
            best_vthfine_val=vthfine
            best_vthfine_code=code
        self.tpx.setDac(TPX3_VTHRESH_FINE,best_vthfine_code)
        vthfine=self.tpx.getDacVoltage(TPX3_VTHRESH_FINE)
        self.log("TPX3_VTHRESH_FINE code=%d voltage=%.1f mV"%(best_vthfine_code,(1000.0*vthfine)))
        if self.abort:  return

        self.tpx.setShutterLen(500)
        self.tpx.sequentialReadout(tokens=4)

        avr=[]
        std=[]
        ThrFrom=0
        ThrTo=512
        ThrStep=self.parent.spinTHLStep.value()
        steps=(ThrTo-ThrFrom)/ThrStep*2
        threshold=10
        step=1

        self.parent.progressBar.setMaximum(steps)
        scans=[]
        for cdac in (0,15):
              logdir="./0x%0X/"%cdac
              #self.log("DACs %0x (logdir:%s)"%(cdac,logdir))
              #self.mkdir(logdir)

              self.log("DAC=0x%X"%cdac)

              self.tpx.resetPixelConfig()
              self.tpx.setPixelMask(ALL_PIXELS,ALL_PIXELS,0)
              self.tpx.setPixelThreshold(ALL_PIXELS,ALL_PIXELS,cdac)

              self.tpx.pauseReadout()
              self.tpx.setPixelConfig()
              self.tpx.sequentialReadout()
              self.tpx.flush_udp_fifo(0x71FF000000000000)#flush until load matrix


              res=np.zeros((256,256))
              to_mask=0
              pixels=0
              for i in range(ThrFrom,ThrTo,ThrStep):
                    if self.abort:
                       return
                    step+=1
                    self.emit(SIGNAL("progress(int)"), step)
                    self.tpx.setDac(TPX3_VTHRESH_FINE,i)
                    self.tpx.openShutter()
                    r=self.tpx.getFrame()
                    #print "Get frame",r
                    ppp=0
                    while True:
                       r,x,y,data,etoa=self.tpx.nextPixel()
                       if not r: break
                       ppp+=1
                       event_counter=(data>>4)&0x3ff
                       if  event_counter >= threshold and not res[x][y]:
                          res[x][y]=i
                          pixels+=1
                          self.tpx.setPixelMask(x,y,1)
                          to_mask+=1
                    print i,ppp

                    if to_mask>7500:
                      print ("Masking %d pixels"%to_mask)
                      self.tpx.pauseReadout()
                      self.tpx.setPixelConfig()
                      self.tpx.sequentialReadout()
                      self.tpx.flush_udp_fifo(0x71FF000000000000)#flush until load matrix
                      to_mask=0

              print "Pixels colected",pixels

              #ThrFinder by threshold only Count>100
              thr_level_array=[]

              for col in range(256):
                for row in range(256):
                  if res[col][row] > 0:
                    thr_level_array.append(res[col][row])
              self.log("          MEAN   = %.2f"%(np.mean(thr_level_array)))
              self.log("          STDEV  = %.2f"%(np.std(thr_level_array)))

              if self.parent.checkStoreDetails.isChecked():
                  fn=fbase+".d%Xm"%cdac
                  self.log("Storing output to %s"%fn)
                  f=open(fn,"w")
                  for col in range(256):
                      for row in range(256):
                        f.write("%d "%(res[row][col]))
                      f.write("\n")
                  f.close()
              scans.append(res)

        bestValue,bestCode,maskPixels=_equalize(scans)


        self.tpx.resetPixelConfig()
        self.tpx.setPixelMask(ALL_PIXELS,ALL_PIXELS,1)
        self.tpx.setPixelThreshold(ALL_PIXELS,ALL_PIXELS,0xf)

        for x in range(256):
           for y in range(256):
               self.tpx.setPixelMask(x,y,maskPixels[x][y])
               self.tpx.setPixelThreshold(x,y,bestCode[x][y])

        self.tpx.pauseReadout()
        self.tpx.resetPixels()
        self.tpx.setPixelConfig()
        self.tpx.sequentialReadout()
        self.tpx.setDac(TPX3_VTHRESH_FINE,380)
        vfbk_code=164
        self.tpx.setDac(TPX3_VFBK,vfbk_code)
        self.tpx.setDac(TPX3_VTHRESH_FINE,300)
        self.tpx.setPllConfig( TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK \
                         | TPX3_PHASESHIFT_DIV_8 | TPX3_PHASESHIFT_NR_1 \
                         | 0x14<<TPX3_PLLOUT_CONFIG_SHIFT )

        self.tpx.setShutterLen(10000)

        if self.parent.checkMaskNoisy.isChecked():
            self.log("Masking noisy pixels")
            vfbk=self.tpx.getDacVoltage(TPX3_VFBK)
            self.log("TPX3_VFBK code=%d voltage=%.1f mV"%(vfbk_code,(1000.0*vfbk)))
            noisy_pixels=[]
            for th in range(414,415):
                self.tpx.setDac(TPX3_VTHRESH_FINE,th)
                vthfine=self.tpx.getDacVoltage(TPX3_VTHRESH_FINE)
                dV=vthfine-vfbk
                print th,vthfine,dV
                self.tpx.openShutter(sleep=True)
                r=self.tpx.getFrame()
                print "frame"
                while True:
                   r,x,y,data,etoa=self.tpx.nextPixel()
                   if not r: break
                   noisy_pixels.append((x,y))
                print noisy_pixels

        for x,y in noisy_pixels:
            self.tpx.setPixelMask(x,y,1)
            self.log("Masking pixel(%d,%d)"%(x,y))

        self.tpx.pauseReadout()
        self.tpx.resetPixels()
        self.tpx.setPixelConfig()
        self.tpx.sequentialReadout()


#                self.log("dV=%.1f mV"%(dV*1000))

        self.tpx.setGenConfig( TPX3_ACQMODE_TOA_TOT | TPX3_GRAYCOUNT_ENA | TPX3_FASTLO_ENA)


        if self.parent.checkStoreTxt.isChecked():
            # Equalization
            self.log("Results stored to TXT file")
            fn=fbase+".inf"
            self.log("    -> registers & info : %s"%fn)

            f=open(fn,"w")
            f.write("time %s\n"% get_date_time())
            f.write("user %s\n"% get_user_name())
            f.write("host %s\n"% get_host_name())
            f.write("ibias_ikrum %s\n"%self.tpx.getDac(TPX3_IBIAS_IKRUM) )
            f.write("vfbk %s\n"%self.tpx.getDac(TPX3_VFBK))

            fn=fbase+".bl"
            self.log("    -> basline : %s"%fn)
            np.savetxt(fn,np.transpose(bestValue),fmt="%.2f")

            fn=fbase+".cod"
            self.log("    -> DAC codes : %s"%fn)
            np.savetxt(fn,np.transpose(bestCode),fmt="%d")

            fn=fbase+".msk"
            self.log("    -> registers & info : %s"%fn)
            np.savetxt(fn,np.transpose(maskPixels),fmt="%d")


        if self.parent.checkStoreXML.isChecked():
            fn=fbase+".t3x"
            self.tpx.saveConfiguration(fn)
            self.log("Results stored to XML file : %s"%fn)

#        self.tpx.resetPixelConfig()
#        self.load_equalization('../calib/eq_codes.dat',\
#                      maskname='../calib/eq_mask.dat')
#        self.setThreshold(1150)
        self.tpx.datadrivenReadout()
        self.tpx.daqThread.start()
        self.done.sig.emit("")

class EqualizeDlg(QDialog, Ui_EqualizeForm):
    def __init__(self,parent=None):
        super(EqualizeDlg, self).__init__(parent)
        self.parent=parent
        self.setupUi(self)
        self.buttonDirectory.clicked.connect(self.onDir)
        self.buttonCancel.clicked.connect(self.onCancel)
        self.buttonEqualize.clicked.connect(self.onEqualize)
        self.EqualizeThread=None
        self.exec_()



    def onEqualize(self):
        if self.EqualizeThread==None:
            self.buttonEqualize.setEnabled(False)
            self.parent.tpx.daqThread.stop()
            self.parent.tpx.daqThread.wait()
            self.EqualizeThread = EqualizeThread(self,tpx=self.parent.tpx)
            QObject.connect(self.EqualizeThread, SIGNAL("progress(int)"),self.progressBar, SLOT("setValue(int)"), Qt.QueuedConnection)
            self.EqualizeThread.done.sig.connect(self.EqualizeThreadDone)
            self.EqualizeThread.start()
        else:
            self.close()

    def EqualizeThreadDone(self):
        self.buttonEqualize.setText("Done")
        self.buttonEqualize.setEnabled(True)

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

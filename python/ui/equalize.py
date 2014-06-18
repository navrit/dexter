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
  return bestValue,bestCode,maskPixels,target


class EqualizeThread(QThread):
    def __init__(self, parent=None,tpx=None):
        QThread.__init__(self, parent)
        self.parent=parent
        self.abort = False
        self.tpx=tpx
        self.done = MySignal()
        self.logsignal=MySignal()

    def stop(self):
        self.abort = True
        self.parent.buttonEqualize.setEnabled(True)

    def log(self,s):
        self.logsignal.sig.emit(s)


    def tuneDacForVoltage(self,dac,voltage):
        best_vfbk_val=0
        best_vfbk_code=0
        self.tpx.setSenseDac(dac)

        self.tpx.setDac(dac,0)
        vmin=self.tpx.getAdcEx(16)

        MAX_CODE=self.tpx.dacMax(dac)
        self.tpx.setDac(dac,MAX_CODE)
        vmax=self.tpx.getAdcEx(16)

        slope=(vmax-vmin)/MAX_CODE
#        print 0,vmin
#        print MAX_CODE,vmax
#        print "slope [V/LSB]",slope

        guess_code=int((voltage-vmin)/slope)
#        print "guess_code",guess_code

        best_val=0
        best_code=0
        for code in range(guess_code-16,guess_code+16):
            self.tpx.setDac(dac,code)
            time.sleep(0.001)
            vol=self.tpx.getAdcEx(16)
            if abs(vol-voltage)<abs(best_val-voltage):
                best_val=vol
                best_code=code

        self.tpx.setDac(dac,best_code)
        return best_code,best_val


    def run(self):
        def _threshold_scan(res, ThrFrom=0, ThrTo=512, ThrStep=1, threshold=10):
              to_mask=0
              pixels=0
              for i in range(int(ThrFrom),int(ThrTo),int(ThrStep)):
                    if self.abort:
                       return
#                    step+=1
#                    self.emit(SIGNAL("progress(int)"), step)
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

        self.tpx.setDac(TPX3_VTHRESH_FINE,256)

        genConfig_register=TPX3_ACQMODE_EVT_ITOT
        if self.parent.checkUseTP.isChecked():
            genConfig_register|=TPX3_TESTPULSE_ENA
        self.tpx.setGenConfig( genConfig_register )
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

        self.log("DC Operating point")

        vthcorse_code=7
        self.tpx.setDac(TPX3_VTHRESH_COARSE,vthcorse_code)
        vthcorse=self.tpx.getDacVoltage(TPX3_VTHRESH_COARSE)
        self.log("  VTHRESH_COARSE code=%3d voltage=%.1f [mV]"%(vthcorse_code,1000.0*vthcorse))
        if self.abort:  return

        vfbk_code,vfbk=self.tuneDacForVoltage(TPX3_VFBK,vthcorse)
        self.log("  VFBK           code=%3d voltage=%.1f [mV]"%(vfbk_code,(1000.0*vfbk)))

        vthfine_code,vthfine=self.tuneDacForVoltage(TPX3_VTHRESH_FINE,vfbk)
        self.log("  VTHFINE        code=%3d voltage=%.1f [mV]"%(vthfine_code,(1000.0*vthfine)))

        if self.abort:  return
        shutter_length=500
        spacing=1
        useTP=self.parent.checkUseTP.isChecked()
        if useTP:spacing=int(self.parent.spinTPSpacing.value())

        if useTP:
            TPperiod=0x4
            TESTPULSES=int(self.parent.spinTPNumber.value())
            self.tpx.setTpPeriodPhase(TPperiod,0)
            self.tpx.setTpNumber(TESTPULSES)
            shutter_length=int(((2*(64*TPperiod+1)*TESTPULSES)/40) + 100)

            coarse_code=100
            self.tpx.setDac(TPX3_VTP_COARSE,coarse_code)
            coarse=self.tpx.getDacVoltage(TPX3_VTP_COARSE)
            self.log( "Test pulse settings")
            self.log( "  VTP_COARSE     code=%3d voltage=%.1f [mV]"%(coarse_code,coarse*1000.0))

            target_e=self.parent.spinTPAmplitude.value()
            target_mV=float(target_e)/1000/20
            vthfine_goal= coarse+target_mV

            vthfine_code,vthfine=self.tuneDacForVoltage(TPX3_VTP_FINE,vthfine_goal)
            self.log("  VTP_FINE       code=%3d voltage=%.1f [mV]"%(vthfine_code,(1000.0*vthfine)))


            electrons=1000.0*20*(vthfine-coarse)
            self.log( "  Electrons ~%d (dV=%.1f mV)"%(electrons,1000.0*(vthfine-coarse)))

        self.tpx.setShutterLen(shutter_length)
        self.tpx.sequentialReadout(tokens=4)

        avr=[]
        std=[]
        ThrFrom=0
        ThrTo=512
        ThrStep=self.parent.spinTHLStep.value()

        steps=(ThrTo-ThrFrom)/ThrStep*2*spacing*spacing
        step=1

        self.parent.progressBar.setMaximum(steps)
        scans=[]
        for cdac in (0,15):
              logdir="./0x%0X/"%cdac
              self.log("DAC=0x%X"%cdac)

              res=np.zeros((256,256))
              for seq in range(spacing*spacing):
                  if spacing*spacing>1: self.log("  Seq %d/%d"%(seq+1,spacing*spacing))
                  self.tpx.resetPixelConfig()
                  self.tpx.setPixelMask(ALL_PIXELS,ALL_PIXELS,1)
                  self.tpx.setPixelThreshold(ALL_PIXELS,ALL_PIXELS,cdac)
                  self.tpx.setCtprBits(0)

                  for x in range(256):
                    if useTP:  self.tpx.setCtprBit(x,1)
                    for y in range(256):
                      self.tpx.setPixelTestEna(x,y, testbit=False)
                      if x%spacing==int(seq/spacing) and y%spacing==seq%spacing:
                          self.tpx.setPixelMask(x,y,0)
                          self.tpx.setPixelTestEna(x,y, testbit=useTP)
                  if useTP:self.tpx.setCtpr()
                  self.tpx.pauseReadout()
                  self.tpx.setPixelConfig()
                  self.tpx.sequentialReadout()
                  self.tpx.flush_udp_fifo(0x71FF000000000000)#flush until load matrix

                  threshold=100
                  if useTP:threshold=TESTPULSES/2



                  _threshold_scan(res, ThrFrom, ThrTo, ThrStep, threshold)
                  if self.abort:
                       return
#              print "Pixels colected",pixels

              #ThrFinder by threshold only Count>100
              thr_level_array=[]

              for col in range(256):
                for row in range(256):
                  if res[col][row] > 0:
                    thr_level_array.append(res[col][row])
              self.log("  MEAN   : %.2f [LSB]"%(np.mean(thr_level_array)))
              self.log("  STDEV  : %.2f [LSB]"%(np.std(thr_level_array)))
              pix=np.count_nonzero(thr_level_array)
              self.log("  Pixels : %d (bad : %d)"%(pix, 256*256-pix))

              if self.parent.checkStoreDetails.isChecked():
                  fn=fbase+".d%Xm"%cdac
                  self.log("  Storing output to '%s'"%fn)
                  np.savetxt(fn,np.transpose(res),fmt="%.0f")
              scans.append(res)

        bestValue,bestCode,maskPixels,target=_equalize(scans)


        if self.parent.checkMeasure.isChecked():
            self.log("Measuring")
            spacing=int(self.parent.spinMeasureSpacing.value())

            if useTP:
                self.tpx.setCtprBits(0)
                self.tpx.setCtpr()

            res=np.zeros((256,256))
            for seq in range(spacing*spacing):
                if spacing*spacing>1: self.log("  Seq %d/%d"%(seq+1,spacing*spacing))
                self.tpx.resetPixelConfig()
                self.tpx.setPixelMask(ALL_PIXELS,ALL_PIXELS,1)
                for x in range(256):
                  for y in range(256):
                    self.tpx.setPixelTestEna(x,y, testbit=False)
                    self.tpx.setPixelThreshold(x,y,bestCode[x][y])
                    if x%spacing==int(seq/spacing) and y%spacing==seq%spacing:
                        self.tpx.setPixelMask(x,y,maskPixels[x][y])
                self.tpx.pauseReadout()
                self.tpx.setPixelConfig()
                self.tpx.sequentialReadout(tokens=4)
                self.tpx.flush_udp_fifo(0x71FF000000000000)#flush until load matrix

                #_threshold_scan(res, target-100, target+100, 2,threshold=10)
                _threshold_scan(res, 0, 512, 2,threshold=10)

            if self.parent.checkStoreDetails.isChecked():
                fn=fbase+".blm"
                self.log("  Storing output to '%s'"%fn)
                np.savetxt(fn,np.transpose(res),fmt="%.2f")

            thr_level_array=[]
            for col in range(256):
              for row in range(256):
                if res[row][col] > 0 and not maskPixels[row][col]:
                   thr_level_array.append(res[row][col])
            self.log("  MEAN   : %.2f [LSB]"%(np.mean(thr_level_array)))
            self.log("  STDEV  : %.2f [LSB]"%(np.std(thr_level_array)))
            pix=np.count_nonzero(thr_level_array)
            self.log("  Pixels : %d (bad : %d)"%(pix, 256*256-pix))



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
            self.log("  VFBK code=%d voltage=%.1f mV"%(vfbk_code,(1000.0*vfbk)))
            noisy_pixels=[]
            for th in range(414,415):
                self.tpx.setDac(TPX3_VTHRESH_FINE,th)
                vthfine=self.tpx.getDacVoltage(TPX3_VTHRESH_FINE)
                dV=vthfine-vfbk
#                print th,vthfine,dV
                self.tpx.openShutter(sleep=True)
                r=self.tpx.getFrame()
#                print "frame"
                while True:
                   r,x,y,data,etoa=self.tpx.nextPixel()
                   if not r: break
                   noisy_pixels.append((x,y))
                print noisy_pixels

        self.log("Masking pixel(%d,%d)"%(x,y))
        for x,y in noisy_pixels:
            self.tpx.setPixelMask(x,y,1)
            self.log("  (%d,%d)"%(x,y))

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
            self.log("    -> basline          : %s"%fn)
            np.savetxt(fn,np.transpose(bestValue),fmt="%.2f")

            fn=fbase+".cod"
            self.log("    -> DAC codes        : %s"%fn)
            np.savetxt(fn,np.transpose(bestCode),fmt="%d")

            fn=fbase+".msk"
            self.log("    -> mask             : %s"%fn)
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
        self.checkUseTP.clicked.connect(self.onUseTP)
        self.EqualizeThread=None
        #self.checkUseTP.setChecked(True)
        self.onUseTP()
        self.exec_()


    def onUseTP(self):
        state=self.checkUseTP.isChecked()
        self.labelTPAmp.setEnabled(state)
        self.labelTPNumber.setEnabled(state)
        self.labelTPSpacing.setEnabled(state)
        self.spinTPAmplitude.setEnabled(state)
        self.spinTPNumber.setEnabled(state)
        self.spinTPSpacing.setEnabled(state)
    def onEqualize(self):
        if self.EqualizeThread==None:
            self.buttonEqualize.setEnabled(False)
            self.parent.tpx.daqThread.stop()
            self.parent.tpx.daqThread.wait()
            self.EqualizeThread = EqualizeThread(self,tpx=self.parent.tpx)
            QObject.connect(self.EqualizeThread, SIGNAL("progress(int)"),self.progressBar, SLOT("setValue(int)"), Qt.QueuedConnection)
            self.EqualizeThread.done.sig.connect(self.EqualizeThreadDone)
            self.EqualizeThread.logsignal.sig.connect(self.onLog)
            self.EqualizeThread.start()
        else:
            self.close()
    def onLog(self,m):
        self.textEdit.append(m)

    def EqualizeThreadDone(self):
        self.buttonEqualize.setText("Done")
        self.buttonEqualize.setEnabled(True)

    def onCancel(self):
        if self.EqualizeThread and self.EqualizeThread.isRunning():
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

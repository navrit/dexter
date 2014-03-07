#!/usr/bin/env python
import  SpidrTpx3
import logging
from SpidrTpx3_engine import *
import random
import Gnuplot, Gnuplot.funcutils
import time
import sys
from struct import *
import cython
import numpy as np


    
def main():
  spidrctrl=SpidrController( 192,168,100,10,50000)
  if  not spidrctrl.isConnected() :
    logging.critical("Unable to connect %s (%s)"%(spidrctrl.ipAddressString(), spidrctrl.connectionErrString()))
    raise RuntimeError("Unable to connect")
  device_nr = 0

  spidrdaq=SpidrDaq (spidrctrl ,0)

  # Sample 'frames' as well as write pixel data to file
  spidrdaq.setSampling( True )
  spidrdaq.setSampleAll( True )

  if not spidrctrl.resetPixels( device_nr ):
    logging.error( "###resetPixels" )

  # Pixel configuration
  spidrctrl.resetPixelConfig()
  spidrctrl.setPixelTestEna( ALL_PIXELS, ALL_PIXELS ,1)
  if not spidrctrl.setPixelConfig( device_nr ) :
    logging.error( "###setPixelConfig" )

  #DACs configuration

  if not spidrctrl.setDacsDflt( device_nr ):
    logging.error( "###setDacsDflt" );
  if not spidrctrl.setDac( device_nr, TPX3_VTHRESH_COARSE, 9 ):
    logging.error( "###setDac" );


  # Test pulse and CTPR configuration

  # Timepix3 test pulse configuration
  if not spidrctrl.setTpPeriodPhase( device_nr, 100, 0 ) :
    logging.error( "###setTpPeriodPhase" );

  if not spidrctrl.setTpNumber( device_nr, 1 ):
    logging.error( "###setTpNumber" );

  # Enable test-pulses for (some or all) columns
  for col in range(256):
#    if  (col & 1) == 1 :
     spidrctrl.setCtprBit( col ,1)

  if not spidrctrl.setCtpr( device_nr ) :
    logging.error( "###setCtpr" );


  # Configure the shutter trigger
  trig_mode      = 4      # SPIDR_TRIG_AUTO;
  trig_length_us = 10000  # 10 ms
  trig_freq_hz   = 10      # Hz
  trig_count     = 1     
  if not spidrctrl.setTriggerConfig( trig_mode, trig_length_us,trig_freq_hz, trig_count ):
    logging.error( "###setTriggerConfig" );

  # SPIDR-TPX3 and Timepix3 timers
  if not spidrctrl.restartTimers():
    logging.error( "###restartTimers" )

  if not spidrctrl.setGenConfig( device_nr,
                               TPX3_POLARITY_HPLUS |
                               TPX3_ACQMODE_TOA_TOT |
                               TPX3_GRAYCOUNT_ENA |
                               TPX3_TESTPULSE_ENA |
                               TPX3_FASTLO_ENA |
                               TPX3_SELECTTP_DIGITAL ):
    logging.error( "###setGenCfg" );
  # Set Timepix3 into acquisition mode
  if not spidrctrl.datadrivenReadout():
    logging.error( "###ddrivenReadout" );

  # Start triggers
  if not spidrctrl.startAutoTrigger():
    logging.error( "###startAutoTrigger" )


  framecnt = 0
  total_size = 0
  total_pixcnt = 0
  next_frame = True
  pix=np.zeros( (256,256) )

  while  next_frame:
     next_frame = spidrdaq.getSampleMin( 256*256*8, 2*256*256*8, 1000 );
     if next_frame :
       print "y", type(next_frame)
       framecnt+=1
       size  = spidrdaq.sampleSize();
       frame = spidrdaq.sampleData();
       pixcnt = 0;
       while True:
         (r, x, y, pixdata, timestamp )=spidrdaq.nextPixel()
#         print (r, x, y, pixdata, timestamp )
         if not r: break
         pix[x][y] = True;
         pixcnt+=1
       total_pixcnt += pixcnt
       total_size += size
       if  pixcnt > 0 :
           print "Sample " , framecnt , " size=" , size , " (total=", total_size , "): " , pixcnt ," pixels" 
     else:
          print "### Timeout -> finish after " , framecnt
          print "   samples, " , total_pixcnt
          print "   pix, bytes r=" , spidrdaq.bytesReceivedCount()
          print "   s=" , spidrdaq.bytesSampledCount()
          print "   w=" , spidrdaq.bytesWrittenCount()
  # Close the shutter
  if not spidrctrl.closeShutter():
    logging.error( "###closeShutter" );

  if not spidrctrl.pauseReadout():
    logging.error( "###pauseReadout" );

  print "Total_pixcnt",total_pixcnt

if __name__=="__main__":
  main()


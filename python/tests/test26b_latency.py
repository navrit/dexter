from .tpx3_test import *
from .dac_defaults import dac_defaults
import time


class TPX3ConfigMatrix:

    def __init__(self, tpx, test):
        self.tpx = tpx
        self.test = test

    def do_matrix_config(self):
        self.tpx.resetPixelConfig()
        for x in range(256):
            for y in range(256):
                self.tpx.setPixelThreshold(x, y, 0x0F)
                self.tpx.setPixelMask(x, y, 0)
                self.tpx.setPixelTestEna(x, y, 1)

        self.tpx.setPixelConfig()


class test_tuomas_debug(tpx3_test):

    """Test for debugging the python scripts"""

    def _execute(self, **keywords):
        self.TPX3_COLUMNS = 1
        tpx = self.tpx
        try:
            print "Starting the test"
            dac_defaults(self.tpx)
            tpx.setPllConfig(
                (TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK |
                 TPX3_PHASESHIFT_DIV_16 | TPX3_PHASESHIFT_NR_16 | 0x14 <<
                 TPX3_PLLOUT_CONFIG_SHIFT))
            self.tpx.setOutputMask(0xFF)
            tpx.shutterOff()
            tpx.resetPixels()
            self.sample_temp(1, "Shutter OFF")
            config = TPX3ConfigMatrix(tpx, self)
            config.do_matrix_config()
            self.tpx.setCtprBits(0)
            self.tpx.t0Sync()

            genConfig_register = TPX3_ACQMODE_TOA_TOT | TPX3_FASTLO_ENA
            genConfig_register |= TPX3_TESTPULSE_ENA | TPX3_GRAYCOUNT_ENA
            genConfig_register |= TPX3_SELECTTP_DIGITAL
            self.tpx.setGenConfig(genConfig_register)
            # self.tpx.datadrivenReadout()

            self.tpx.pauseReadout()
            self.tpx.sequentialReadout(tokens=1)
            self.tpx.resetPixels()
            self.inject_testpulses(10)

        finally:
            print "Cleaning up the test"
            self.cleanup()

    def sample_temp(self, nsamples=10, msg=""):
        for i in range(nsamples):
            temperature = self.tpx.getTpix3Temp()
            print "%s Timepix3 temperature is %d" % (msg, temperature)
            time.sleep(1)

    def inject_testpulses(self, npulses):
        finish = 0
        events = 0

        # Make everything ready for TP injection
        shutter_length = 1000
        TESTPULSES = npulses
        period = 0x4
        self.tpx.setTpPeriodPhase(period, 0)
        self.tpx.setTpNumber(TESTPULSES)
        shutter_length = int(((2*(64*period+1)*TESTPULSES)/40) + 100)
        self.tpx.setShutterLen(shutter_length)

        # Inject TPs to one column at time
        for x in range(self.TPX3_COLUMNS):
            self.tpx.setCtprBit(x, 1)
            self.tpx.setCtpr()
            self.tpx.openShutter()
            self.get_and_process_packets()
            self.get_and_process_packets()
            # self.tpx.closeShutter()
            self.tpx.setCtprBit(x, 0)

    def cleanup(self):
        print "cleanup() called"

    def get_and_process_packets(self):
        finish = 0
        events = 0
        data = self.tpx.get_frame()
        print "Received %d packets." % (len(data))
        for pck in data:
            if pck.isData():
                events += 1
            else:
                if pck.isEoR():
                    finish = 1
                else:
                    print "Got packet"
        print "Finished the injection. %d events received." % (events)

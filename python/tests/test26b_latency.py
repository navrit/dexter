from .tpx3_test import *
from .dac_defaults import dac_defaults
import time

from tuomas import *


class test26b_latency(tpx3_test):

    """Test for measuring the latency of the output packets"""

    def _execute(self, **keywords):
        self.npulses = 100
        try:
            self.logging.info("Starting latency measurement test")
            conf_daq = TPX3ConfBeforeDAQ(self)
            conf_daq.do_config()
            self.sample_temp(1, "Shutter OFF")
            conf_matrix = TPX3ConfMatrixTPEnable(self)
            conf_matrix.do_config()
            self.conf_matrix = conf_matrix
            self.tpx.setCtprBits(0)
            self.inject_testpulses(1)

        finally:
            print "Cleaning up the test"
            self.cleanup()

    def sample_temp(self, nsamples=10, msg=""):
        for i in range(nsamples):
            temperature = self.tpx.getTpix3Temp()
            print "%s Timepix3 temperature is %d" % (msg, temperature)

    def inject_testpulses(self, npulses):
        conf_tp = TPX3ConfTestPulses(self)
        conf_tp.set_tp_config(period=0x0E, npulses=self.npulses)
        conf_tp.set_column_mask(self.get_ctpr_mask(everyNcols = 8))
        conf_tp.do_config()
        self.num_tp_enabled = self.conf_matrix.num_tp_enabled(mask = self.ctpr_mask)
        #self.logging.info("Injecting hits to column %d" % (x))
        data = self.tpx.get_N_packets(1024)
        data_driven_seq = TPX3DataDrivenSeq(self, 1000)
        data_driven_seq.do_seq()
        self.data = data_driven_seq.get_data()
        self.events = data_driven_seq.num_packets()

    def cleanup(self):
        """ Called after the test to extract results and cleanup."""
        print "cleanup() called"
        expected = self.npulses * self.num_tp_enabled
        efficiency = float(self.events) / expected
        self.logging.info("Total of %d event packets received" % (self.events))
        self.logging.info("Expected number %d" % (expected))
        self.logging.info("Efficiency: %f" %(efficiency))
        self.data_to_file()

    def data_to_file(self):
        """ Prints collected toa/tot data into separate files"""

        data = ['toa', 'tot']
        for field in data:
            hist = self.data[field]
            fname = open(self.fname + "/hist_" + field + ".csv", "w")
            sorted(hist, key=int)
            for key in hist.keys():
                fname.write("%d, %d\n" % (key, hist[key]))
            fname.close()

    def get_ctpr_mask(self, everyNcols):
        ctpr_mask = list(range(256))
        tmp_count = everyNcols
        for i in range(256):
            if tmp_count == 1:
                ctpr_mask[i] = 1
                tmp_count = everyNcols
            else:
                ctpr_mask[i] = 0
                tmp_count -= 1
        self.ctpr_mask = ctpr_mask
        return ctpr_mask

from .tpx3_test import *
from .dac_defaults import dac_defaults
import time

from tuomas import *


class test26b_latency(tpx3_test):

    """Test for measuring the latency of the output packets"""

    def __init2__(self, meas_power, **keywords):
        self.debug = False
        self.temps = dict()
        self.args = dict()
        self.legal_args = ['npulses', 'npixels', 'ncols']
        self.defaults = {'npulses': 100, 'npixels': 4, 'ncols': 4}
        self.tp_period = 0x0F
        self.clk_freq_mhz = 40.0
        self.meas_power = meas_power
        for arg in self.legal_args:
            if arg in keywords:
                self.args[arg] = int(keywords[arg])
                print "Set arg %s to %s from command line" % (arg, str(keywords[arg]))
            else:
                self.args[arg] = self.defaults[arg]
        if meas_power is True:
            self.sampler = CurrentTempSampler(self, meas_power=True)
            self.sampler.verbose = False

    def _execute(self, **keywords):
        self.__init2__(meas_power=True, **keywords)
        try:
            self.logging.info("Starting latency measurement test")
            conf_daq = TPX3ConfBeforeDAQ(self)
            conf_daq.do_config()

            conf_matrix = TPX3ConfMatrixTPEnable(self)
            conf_matrix.set_pixel_tp_mask(everyNpixels=self.args['npixels'])
            conf_matrix.do_config()
            self.conf_matrix = conf_matrix
            self.pixel_tp_mask = conf_matrix.get_pixel_tp_mask()

            self.tpx.shutterOn()
            self.sampler.sample_cur_and_temp("Start of DAQ, shutter open", "", 10)
            self.tpx.shutterOff()

            self.tpx.setCtprBits(0)
            self.inject_testpulses(1)

        finally:
            print "Cleaning up the test"
            tpx3_cleanup_seq = TPX3CleanupSeq(self)
            tpx3_cleanup_seq.do_seq()
            self.cleanup()
            self.report()

    def inject_testpulses(self, npulses):
        conf_tp = TPX3ConfTestPulses(self)
        conf_tp.set_tp_config(
            period=self.tp_period,
            npulses=self.args['npulses'])
        conf_tp.set_column_mask(
            self.get_ctpr_mask(
                everyNcols=self.args['ncols']))
        conf_tp.do_config()
        self.num_tp_enabled = self.conf_matrix.num_tp_enabled(
            mask=self.ctpr_mask)
        data = self.tpx.get_N_packets(1024)
        print "Flushed out %d packets" % (len(data))

        data_driven_seq = TPX3DataDrivenSeq(self, 1000)
        data_driven_seq.set_callback(
            'before_shutter_on',
            self.sampler.sample_cur_and_temp)
        data_driven_seq.set_callback(
            'after_shutter_on',
            self.sampler.sample_cur_and_temp)
        data_driven_seq.set_callback(
            'get_packets',
            self.sampler.sample_cur_and_temp)
        self.daq_seq = data_driven_seq
        data_driven_seq.analyzer.set_ctpr_mask(self.ctpr_mask)
        data_driven_seq.analyzer.set_pixel_tp_mask(self.pixel_tp_mask)

        data_driven_seq.do_seq()
        self.data = data_driven_seq.get_data()
        self.events = data_driven_seq.num_packets()

    def cleanup(self):
        """ Called after the test to extract results and cleanup."""
        print "cleanup() called"
        expected = self.args['npulses'] * self.num_tp_enabled
        if expected > 0:
            efficiency = float(self.events) / expected
        else:
            efficiency = -1
        self.logging.info("Total of %d event packets received" % (self.events))
        self.logging.info("Expected number %d" % (expected))
        self.logging.info("Efficiency: %f" % (efficiency))
        self.logging.info("Calc. rate: %f Mpackets/s" % (self.get_rate()))
        self.logging.info(self.daq_seq.analyzer.report())
        self.data_to_file()

    def report(self):
        if self.meas_power is True:
            self.sampler.results_to_files()

    def get_rate(self):
        return self.clk_freq_mhz * 1.0 / \
            (2*64*self.tp_period) * self.num_tp_enabled

    def data_to_file(self):
        """ Prints collected toa/tot data into separate files"""

        data = ['toa', 'tot', 'col', 'row']
        for field in data:
            hist = self.data[field]
            fname = open(self.fname + "/hist_" + field + ".csv", "w")
            sorted(hist, key=int)
            for key in hist.keys():
                fname.write("%d, %d\n" % (key, hist[key]))
            fname.close()

        fname = open(self.fname + "/plot_temp.csv", "w")
        for temp in self.temps:
            fname.write("%d,%d\n" % (temp, self.temps[temp]))
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
        if self.debug is True:
            print ctpr_mask
        return ctpr_mask

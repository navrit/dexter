from .tpx3_test import *

from tuomas import *


class test26b_latency(tpx3_test):

    """Test for measuring the latency of the output packets"""

    def __init2__(self, meas_power, **keywords):
        self.debug = False
        self.temps = dict()
        self.args = dict()
        self.legal_args = ['npulses', 'npixels', 'ncols']
        self.defaults = {'npulses': 1000, 'npixels': 300, 'ncols': 300}
        self.tp_period = 0x0F
        self.clk_freq_mhz = 40.0
        self.meas_power = meas_power
        self.data = dict()

        for arg in self.legal_args:
            if arg in keywords:
                self.args[arg] = int(keywords[arg])
                print "Set arg %s to %s from command line" % (arg, str(keywords[arg]))
            else:
                self.args[arg] = self.defaults[arg]
        if meas_power is True:
            self.sampler = CurrentTempSampler(
                self,
                meas_power=True,
                meas_temp=False)
            self.sampler.verbose = False

    def _execute(self, **keywords):
        self.__init2__(meas_power=True, **keywords)
        self.logging.info("Starting latency measurement test")
        conf_daq = TPX3ConfBeforeDAQ(self)
        conf_daq.do_config()

        conf_matrix = TPX3ConfMatrixTPEnable(self, mask=0)
        conf_matrix.set_pixel_tp_mask(everyNpixels=self.args['npixels'])
        conf_matrix.do_config()
        self.conf_matrix = conf_matrix
        self.pixel_tp_mask = conf_matrix.get_pixel_tp_mask()

        self.check_timer_offset()

        self.tpx.shutterOn()
        if self.meas_power is True:
            self.sampler.sample_cur_and_temp(
                "Start of DAQ, shutter open",
                "",
                10)
        self.tpx.shutterOff()

        self.tpx.setCtprBits(0)
        self.inject_testpulses(1)

        print "Cleaning up the test"
        tpx3_cleanup_seq = TPX3CleanupSeq(self)
        tpx3_cleanup_seq.do_seq()
        self.cleanup()
        self.report()
        self.data_to_file()

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
        print "Flushed out %s packets" % (len(data))

        data_driven_seq = TPX3DataDrivenSeq(self, 1000)

        # If power cons. is measured, some callbacks are needed
        if self.meas_power is True:
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

    def get_num_expected(self):
        """ Returns the expected amount of packets. """
        self.num_tp_enabled = self.conf_matrix.num_tp_enabled(
            mask=self.ctpr_mask)
        expected = self.args['npulses'] * self.num_tp_enabled
        return expected

    def cleanup(self):
        """ Called after the test to extract results and cleanup."""
        print "cleanup() called"
        #expected = self.args['npulses'] * self.num_tp_enabled
        expected = self.get_num_expected()
        print "npulses: %d, num_tp_enabled: %d" % (self.args['npulses'], self.num_tp_enabled)
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

        data = ['toa', 'tot', 'col', 'row', 'latency']
        for field in data:
            if field in self.data:
                hist = self.data[field]
                fname = "hist_" + field + ".csv"
                self.hist_to_file(hist, fname)

        self.hist_to_file(self.temps, "plot_temp.csv")
        self.hist_to_file(self.offset_hist, "timer_offset.csv")

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

    def check_timer_offset(self):
        """ Checks the timer offset between TPX3 and the DAQ FPGA"""
        proc = TPX3PacketProcessor()

        # Filter is used to select which packets are sent to PC
        eth,cpu=self.tpx.getHeaderFilter()
        self.tpx.setHeaderFilter(eth|0x0C90,cpu)

        nsamples = 200
        offset_hist = dict()
        for i in range(nsamples):

            r, lo, hi = self.tpx.ctrl.getTimer(self.tpx.id)

            data = self.tpx.get_N_packets(1024)
            for d in data:
                if proc.is_timer_resp(d):
                    header = proc.get_header(d)
                    #print "Got header %4x"%(header)
                    if header == 0x44:
                        offset = proc.get_timer_offset(d)
                    #    print ">> Timer offset is %s"%(offset)

                        if offset in offset_hist:
                            offset_hist[offset] += 1
                        else:
                            offset_hist[offset] = 1
            if i % (nsamples/10) == 0:
                print "."

        self.offset_hist = offset_hist
        print "%s timer samples were taken" % (nsamples)

    def hist_to_file(self, hist, fname):
        fname = self.fname + "/" + fname
        fout = open(fname, "w")
        sorted(hist, key=int)
        for key in sorted(hist.keys()):
            fout.write("%d, %d\n" % (key, hist[key]))
        fout.close()


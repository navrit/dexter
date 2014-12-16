from tpx3_test import *
import time
import logging
from SpidrTpx3_engine import *
import numpy
from dac_defaults import dac_defaults
import sys

from tuomas import *


class test26_vdd_meas(tpx3_test):

    """Test for measuring digital power consumption in different scenarios."""

    def init2(self, meas_power=False):
        """ Inits the object. Called in _execute before tests """
        self.verbose = 1
        self.target_thr_v = 0.00
        self.clk_period = 40.0
        self.daq = TPX3DataAcqSeq(self)
        self.meas_power = meas_power
        if self.meas_power is True:
            self.sampler = CurrentTempSampler(self, meas_power=True)

    def _msg(self, msg):
        if self.verbose:
            self.logging.info(msg)

    def sample_cur_and_temp(self, msg="", ind="", nsamples=0):
        """ Reads voltage/current from the gpib device and samples also temperature
        of Timepix3. Num. of samples can be set for each func call. """
        if self.meas_power is False:
            return
        else:
            self.sampler.sample_cur_and_temp(msg, ind, nsamples)
            return

    def _execute(self, **keywords):
        try:
            self.init2(meas_power=True)
            tpx_config = TPX3ConfAndReadPower(
                self.tpx,
                self,
                meas_power=False)  # Create a class for configuring
            self.sample_cur_and_temp("Before pixel reset")
            tpx_config.do_config()

            self.sample_cur_and_temp("After the configuration")

            # At this point we should have Timepix3 configured, start the DAQ

            # ToA/ToT mode characterisation
            if self.meas_power is True:
                self.sampler.set_tag("\t# DAQ_TOA_TOT")
            tpx_daq = self.daq
            tpx_daq.do_daq()
            #time.sleep(2)

            # ToA only mode characterisation
            # tpx_daq.do_daq(TPX3_ACQMODE_TOA)

            # Event count / iToT characterisation
            if self.meas_power is True:
                self.sampler.set_tag("\t# DAQ_EVT_ITOT")
            tpx_daq.do_seq_daq(TPX3_ACQMODE_EVT_ITOT, 1.0, 20)

        finally:
            self.cleanup()

        self.report()

    def cleanup(self):
        """ Ensures that the chip is restored to the same state after each test
        run"""
        tpx3_cleanup_seq = TPX3CleanupSeq(self)
        tpx3_cleanup_seq.do_seq()

    def report(self):
        """ Prints the report of the measurements to files"""
        if self.meas_power is True:
            self.sampler.results_to_files()

        self.daq.records_to_files()
        self.daq.report()


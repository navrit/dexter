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
        self.currents = []
        self.db = CurTempRecord()
        self.verbose = 1
        self.samples = 10
        self.csv = CSVRecordWithTime()
        self.csv.set_columns(("Time", "Current", "Temperature"))
        self.target_thr_v = 0.00
        self.clk_period = 40.0
        self.temp_lower = -25
        self.temp_upper = 150
        self.daq = TPX3DataAcqSeq(self)
        self.meas_power = meas_power
        if self.meas_power is True:
            if not self.create_and_connect_gpib_dev():
                self.logging.info("ERROR. Couldn't create/connect GPIB")
                return

    def _msg(self, msg):
        if self.verbose:
            self.logging.info(msg)

    def store_measured_values(self, msg, cur, temp):
        """ Stores measured current and temperature"""
        self.db.add(msg, cur, temp)

    def create_and_connect_gpib_dev(self):
        """ Creates GPIB device and opens a connection"""
        gpib_addr = 10
        gpib_dev = GPIBdev(gpib_addr)
        if not gpib_dev.connect():
            self.logging.info("No connection to GPIB device. Exiting...")
            return False
        self.gpib_dev = gpib_dev
        print "GPIB device was created properly."
        return True

    def sample_cur_and_temp(self, msg="", ind="", nsamples=0):
        """ Reads voltage/current from the gpib device and samples also temperature
        of Timepix3. Num. of samples can be set for each func call. """
        if self.meas_power is False:
            return

        self._msg(ind + msg)

        if nsamples == 0:
            nsamples = self.samples

        for i in range(nsamples):
            temperature = 999

            while not self.temp_within_limits(temperature):
                line = self.gpib_dev.qr("read?")
                vol, cur, x, y, z = map(float, line.split(","))
                temperature = self.tpx.getTpix3Temp()
                if self.temp_within_limits(temperature):
                    self.currents.append(cur)
                    self._msg(
                        ind + "\tVDD Voltage: %8.5f  Current: %8.5f" %
                        (vol, cur))
                    self._msg(ind + "\tTemperature: %8.5f" % (temperature))
                    self.store_measured_values(msg, cur, temperature)
                    self.csv.add((cur, temperature))
                else:
                    self.logging.info("Warning. Sample rejected. Temp out of limits:\
            %8.5f" % (temperature))

    def temp_within_limits(self, temperature):
        """ Checks that the temperature is within reasonable limits """
        if temperature > self.temp_upper or temperature < self.temp_lower:
            return 0
        else:
            return 1

    def _execute(self, **keywords):
        try:
            self.init2(meas_power = True)
            tpx_config = TPX3ConfAndReadPower(
                self.tpx,
                self,
                False)  # Create a class for configuring
            self.sample_cur_and_temp("Before pixel reset")
            tpx_config.do_config()

            self.sample_cur_and_temp("After the configuration")

            # At this point we should have Timepix3 configured, start the DAQ

            # ToA/ToT mode characterisation
            tpx_daq = self.daq
            # tpx_daq.do_daq()
            time.sleep(2)

            # ToA only mode characterisation
            # tpx_daq.do_daq(TPX3_ACQMODE_TOA)

            # Event count / iToT characterisation
            tpx_daq.do_seq_daq(TPX3_ACQMODE_EVT_ITOT, 1.0, 20)

        finally:
            self.cleanup()

        self.report()

    def cleanup(self):
        """ Ensures that the chip is restored to the same state after each test
        run"""
        self.tpx.shutterOff()
        self.tpx.resetPixels()

    def report(self):
        """ Prints the report of the measurements to files"""
        fresult = open(self.fname + "/current_temp.dat", "w")
        fresult.write(self.db.to_string())
        fresult.close()

        fresult = open(self.fname + "/current_temp.csv", "w")
        fresult.write(self.csv.to_string())
        fresult.close()

        self.daq.records_to_files()



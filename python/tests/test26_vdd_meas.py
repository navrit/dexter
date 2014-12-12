from tpx3_test import *
import time
import logging
from SpidrTpx3_engine import *
import numpy
from dac_defaults import dac_defaults
import sys

from tuomas import *


class TPX3BasicConfig:

    """Class for hiding some basic configuration code"""

    def __init__(self, tpx, test, read_power=True):
        self.tpx = tpx
        self.test = test
        self.read_power = read_power  # If specified, reads power to GPIB

    def set_clk_period_and_phase(self, period, phase_div, phase_num):
        self.tpx.setPllConfig(
            (TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK | phase_div |
             phase_num | 0x14 << TPX3_PLLOUT_CONFIG_SHIFT))

    def do_read_power(self, msg, indent=""):
        """ Reads power through GPIB if the read_power flag is True"""
        if self.read_power:
            test.sample_cur_and_temp(msg, indent)

    def do_config(self):
        """ Configures Timepix3 for DACQ"""
        tpx = self.tpx
        test = self.test
        clk_period = test.clk_period
        phase_shift = TPX3_PHASESHIFT_DIV_2

        self.tpx.shutterOff()
        tpx.resetPixels()
        dac_defaults(tpx)

        # The number of phases doesn't really affect the power consumption...
        # clk_phase_nums = [TPX3_PHASESHIFT_NR_16, TPX3_PHASESHIFT_NR_8,
        #    TPX3_PHASESHIFT_NR_4, TPX3_PHASESHIFT_NR_2, TPX3_PHASESHIFT_NR_1]

        clk_phase_nums = [TPX3_PHASESHIFT_NR_16]  # So use 16 phases always

        clk_phase_divs = [TPX3_PHASESHIFT_DIV_2, TPX3_PHASESHIFT_DIV_4,
                          TPX3_PHASESHIFT_DIV_8, TPX3_PHASESHIFT_DIV_16]
        clk_periods = [20, 40, 80, 160]

        for phase_num in clk_phase_nums:
            test.logging.info("### Number of phases %x ###" % (phase_num))
            # Check power cons. with all clock freqs, ToA on and off
            for clk_period in clk_periods:
                phase_div, phases = self.get_phase_div(clk_period)
                self.set_clk_period_and_phase(clk_period, phase_div, phase_num)
                test.logging.info("\t### phase_div %x ###" % (phase_div))
                genConfig_register = TPX3_ACQMODE_TOA_TOT | TPX3_FASTLO_ENA
                self.tpx.setGenConfig(genConfig_register)
                self.do_read_power(
                    "Clk: %d" %
                    (clk_period) +
                    " ToA counter OFF",
                    "\t\t")
                genConfig_register = TPX3_ACQMODE_TOA_TOT | TPX3_FASTLO_ENA | TPX3_GRAYCOUNT_ENA
                self.tpx.setGenConfig(genConfig_register)
                self.do_read_power(
                    "Clk: %d" %
                    (clk_period) +
                    " ToA counter ON",
                    "\t\t")

        clk_period = test.clk_period
        phase_div, phase = self.get_phase_div(clk_period)
        self.set_clk_period_and_phase(
            clk_period,
            phase_div,
            TPX3_PHASESHIFT_NR_16)
        self.do_read_power("Going back to %d MHz" % (clk_period), "\t")

        # Output mask has negligible impact on power consumption
        #output_masks = [0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF]
        output_masks = [0xFF]
        for output_mask in output_masks:
            self.tpx.setOutputMask(0xFF)
            self.do_read_power("Output mask: " + "%x" % (output_mask), "\t")

        self.tpx.setCtprBits(0)
        self.tpx.setCtpr()
        self.set_mask_all_pixels(1)
        self.do_read_power("After masking all pixels", "\t")
        self.tpx.datadrivenReadout()
        self.do_read_power("In data-driven readout mode", "\t")

        self.configure_all_pixels()

        self.tpx.resetTimer()
        self.tpx.t0Sync()
        self.tpx.openShutter(sleep=False)
        self.tpx.shutterOn()
        self.do_read_power("After opening the shutter", "\t")

        # At this point, the shutter is open and the chip is ready to take data

    def get_phase_div(self, clk_period):
        phase_shift = 0
        phases = 0.0
        if clk_period == 20:
            phase_shift = TPX3_PHASESHIFT_DIV_16
            phases = 16.0
        elif clk_period == 40:
            phase_shift = TPX3_PHASESHIFT_DIV_8
            phases = 16.0
        elif clk_period == 80:
            phase_shift = TPX3_PHASESHIFT_DIV_4
            phases = 8.0
        elif clk_period == 160:
            phase_shift = TPX3_PHASESHIFT_DIV_2
            phases = 4.0
        return (phase_shift, phases)

    def set_mask_all_pixels(self, mask_bit):
        self.tpx.resetPixelConfig()
        for x in range(256):
            for y in range(256):
                self.tpx.setPixelThreshold(x, y, 0x0F)
                self.tpx.setPixelMask(x, y, mask_bit)
        self.tpx.setPixelConfig()

    def configure_all_pixels(self):
        """ Configures the pixel matrix based on existing threshold values. """
        test = self.test
        fbase = "calib/tmp_W2G4_ik5/FineTune/100mV/"
        bname = self.tpx.readName()
        test.logging.info("Chip name    : %s" % bname)
        self.tpx.loadDACs(directory=fbase, chipname=bname)

        codes_eq = numpy.loadtxt(fbase+"/eq_codes_finestep_%s.dat" % bname, int)
        mask = numpy.loadtxt(fbase+"/eq_mask_finestep_%s.dat" % bname, int)

        fbase2 = "calib/tmp_W2G4_ik5/tot_toa_calibration/100mV/"
        dt = numpy.dtype(numpy.float32)
        a_tot = numpy.fromfile(
            fbase2 +
            "/a_tot_bin.dat",
            dtype=dt).reshape(
            (256,
             256))
        b_tot = numpy.fromfile(
            fbase2 +
            "/b_tot_bin.dat",
            dtype=dt).reshape(
            (256,
             256))
        c_tot = numpy.fromfile(
            fbase2 +
            "/c_tot_bin.dat",
            dtype=dt).reshape(
            (256,
             256))
        t_tot = numpy.fromfile(
            fbase2 +
            "/t_tot_bin.dat",
            dtype=dt).reshape(
            (256,
             256))
        c_toa = numpy.fromfile(
            fbase2 +
            "/c_toa_bin.dat",
            dtype=dt).reshape(
            (256,
             256))
        t_toa = numpy.fromfile(
            fbase2 +
            "/t_toa_bin.dat",
            dtype=dt).reshape(
            (256,
             256))
        d_toa = numpy.fromfile(
            fbase2 +
            "/d_toa_bin.dat",
            dtype=dt).reshape(
            (256,
             256))

        self.tpx.resetPixelConfig()
        for x in range(256):
            for y in range(256):
                self.tpx.setPixelThreshold(x, y, codes_eq[y][x])
                self.tpx.setPixelMask(x, y, mask[y][x])

        self.tpx.setPixelConfig()
        self.tpx.datadrivenReadout()

        thr_dac = 1050
        self.tpx.setThreshold(thr_dac)
        target_thr_v = self.test.target_thr_v

        for i in range(100):
            thr_dac = self.tpx.trackThreshold(
                target_thr_v=target_thr_v,
                thr_dac=thr_dac)

        # flush any remaing data
        for i in range(3):
            data = self.tpx.get_N_packets(1024*64)


class test26_vdd_meas(tpx3_test):

    """Test for measuring digital power consumption in different scenarios."""

    def init2(self, meas_power = False):
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
        self.daq = TPX3DataAcq(self.tpx, self)
        self.meas_power = meas_power
        if self.meas_power is True:
            if not self.create_and_connect_gpib_dev():
                self.logging.info("ERROR")
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
            self.init2()
            tpx_config = TPX3BasicConfig(
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


class TPX3DataAcq:

    """ Class for doing data acquisitions with Timepix3. The mode and length in
    seconds can be chosen. Must be created within a test class derived from
    tpx3_test. """

    def __init__(self, tpx, test):
        self.tpx = tpx
        self.test = test
        self.target_thr_v = test.target_thr_v
        self.records = dict()
        self.finish = False
        self.temp = 999
        self.anim = ['|', '/', '-', '\\', '|', '/', '-', '\\']
        self.thr_dac = 0
        self.mode_str = "UNKNOWN"

    def do_daq(self, mode=TPX3_ACQMODE_TOA_TOT, max_time=20.0):
        """ Does DAQ in specified mode. Runs for max time specified. """
        csv_record = CSVRecordWithTime()
        csv_record.set_columns(('Time', 'Events', 'Rate', 'Temp'))

        target_thr_v = self.target_thr_v
        time_start = time.time()
        time_elapsed = 0.0
        event_counter = 0
        self.init_daq_run(mode)

        self.tpx.shutterOff()
        genConfig_register = mode | TPX3_FASTLO_ENA | TPX3_GRAYCOUNT_ENA
        self.tpx.setGenConfig(genConfig_register)
        self.tpx.shutterOn()

        self.test.logging.info(
            "Starting Timepix3 data_driven DAQ, mode: %s" %
            (self.mode_str))

        while not self.finish:
            time_elapsed = time.time() - time_start
            time_temp = time.time() - self.time_last_temp
            self.sample_temperature(time_temp)

            if time_elapsed > max_time:
                self._finish_daq()
            else:
                data = self.tpx.get_N_packets(1024)
                event_counter += self.process_data(data)

                self.count_rate_and_record_data(
                    event_counter,
                    time_elapsed,
                    csv_record)
        self.records["DAQ_" + self.mode_str] = csv_record

    def do_seq_daq(
            self,
            mode=TPX3_ACQMODE_EVT_ITOT,
            ro_period=1.0,
            max_time=20.00):
        """ Does DAQ and sequential readouts with specified rate """
        csv_record = CSVRecordWithTime()
        csv_record.set_columns(('Time', 'Events', 'Rate', 'Temp'))

        time_last_readout = time.time()
        time_start = time.time()
        time_elapsed = 0.0
        event_counter = 0
        target_thr_v = self.target_thr_v
        self.init_daq_run(mode)

        self.tpx.shutterOff()
        genConfig_register = mode | TPX3_FASTLO_ENA | TPX3_GRAYCOUNT_ENA
        self.tpx.setGenConfig(genConfig_register)
        self.tpx.sequentialReadout()
        self.tpx.shutterOn()

        self.test.logging.info(
            "Starting Timepix3 seq. DAQ, mode: %s" %
            (self.mode_str))

        while not self.finish:
            time_since_last_readout = time.time() - time_last_readout
            time_elapsed = time.time() - time_start
            time_temp = time.time() - self.time_last_temp

            self.sample_temperature(time_temp)

            if time_elapsed > max_time:
                self._finish_daq()
            else:
                if time_since_last_readout > ro_period:
                    time_last_readout = time.time()

                    # Read out all data
                    self.tpx.shutterOff()
                    time.sleep(0.001)
                    data = self.tpx.get_frame()
                    event_counter += self.process_data(data)
                    time.sleep(0.001)
                    self.tpx.shutterOn()
                    self.count_rate_and_record_data(event_counter, time_elapsed,
                                                    csv_record)

        self.records["DAQ_" + self.mode_str] = csv_record

    def records_to_files(self):
        """ Prints data during DAQ to files """
        for record in self.records.keys():
            fname = open(self.test.fname + "/" + record + ".csv", "w")
            fname.write(self.records[record].to_string())
            fname.close()

    def _finish_daq(self):
        """ Cleans up the chip and closes the shutter at the of DAQ run """
        self.tpx.shutterOff()
        time.sleep(0.001)
        data = self.tpx.get_frame()
        self.finish = True

    def sample_temperature(self, time_temp):
        """ Samples temperature (and current) """
        if time_temp > 1.0:
            #self.thr_dac =self.tpx.trackThreshold(target_thr_v=target_thr_v,thr_dac=thr_dac,adc=16)
            self.time_last_temp = time.time()
            self.temp = self.tpx.getTpix3Temp()
            print "\n"
            self.test.sample_cur_and_temp(
                "DAQ mode %s" %
                (self.mode_str),
                "",
                1)
            self.nsample += 1

    def process_data(self, data):
        """ Processes a bunch of packets given as input """
        event_counter = 0
        for pck in data:
            if pck.isData():
                if self.mode_str == 'EVT_ITOT':
                    event_counter += pck.event_counter
                else:
                    event_counter += 1
            else:
                if pck.isEoR():
                    self.finish = 1
        return event_counter

    def get_mode_string(self, mode):
        """ Returns a string corresponding to the current op. mode """
        mode_str = "ERROR"
        if mode == TPX3_ACQMODE_TOA:
            mode_str = "TOA"
        elif mode == TPX3_ACQMODE_TOA_TOT:
            mode_str = "TOA_TOT"
        elif mode == TPX3_ACQMODE_EVT_ITOT:
            mode_str = "EVT_ITOT"
        else:
            print "Unknown operation mode"
        return mode_str

    def count_rate_and_record_data(
            self,
            event_counter,
            time_elapsed,
            csv_record):
        if time_elapsed > 0:
            hitRate = event_counter/time_elapsed
        else:
            hitRate = 0

        print "%c %s Time: %.3f s %d hits %.2f hits/s %d %.2f C" % (13,
                                                                    self.anim[event_counter % len(self.anim)], time_elapsed, event_counter, hitRate,
                                                                    self.thr_dac, self.temp),
        sys.stdout.flush()
        csv_record.add((event_counter, hitRate, self.temp))

    def init_daq_run(self, mode):
        self.time_last_temp = time.time()
        self.finish = False
        self.thr_dac = 0
        self.temp = 0
        self.nsample = 1
        self.mode_str = self.get_mode_string(mode)

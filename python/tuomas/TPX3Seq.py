
from tests.tpx3_test import *
from tuomas.TPX3PacketAnalysis import TPX3TestPulsePacketAnalyzer
from tuomas.TPX3Utils import CSVRecordWithTime
import time


class TPX3SeqBase:
    """Base class for command sequences to control Timepix3"""

    def __init__(self, test):
        self.tpx = test.tpx
        self.test = test

    def do_seq(self):
        pass

class TPX3DataDrivenSeq(TPX3SeqBase):

    def __init__(self, test, rpt_interval = 10):
        TPX3SeqBase.__init__(self,test)
        self.events = 0
        self.TIMEOUT = 60
        self.rpt_interval = rpt_interval
        self.rpt_count = rpt_interval
        self.analyzer = TPX3TestPulsePacketAnalyzer(test)

    def get_data(self, data_type=""):
        if data_type == "":
            return self.analyzer.get_data()
        else:
            return self.analyzer.get_data(data_type)

    def num_packets(self):
        return self.events

    def do_seq(self):
        self.tpx.datadrivenReadout()
        self.tpx.shutterOn()
        self.get_and_process_packets()
        self.tpx.shutterOff()

    def get_and_process_packets(self):
        finished = False
        time_start = time.time()
        time_elapsed = 0

        while not finished:
            data = self.tpx.get_N_packets(2048)
            self.analyzer.analyze_packets(data)
            time_elapsed = time.time() - time_start
            self.test.sample_temp(1, "DURING DAQ")
            if self.rpt_count == 0:
                self.test.sample_temp(1, "DURING DAQ")
                self.test.logging.info("Packet count: %d" % (self.events))
                self.rpt_count = self.rpt_interval
            else:
                self.rpt_count -= 1
            if time_elapsed > self.TIMEOUT:
                self.test.logging.info("TIMEOUT %d reached" % (self.TIMEOUT))
                finished = True
            finished |= self.analyzer.is_finished()

        time_total =  time.time() - time_start
        self.events = self.analyzer.num_events()
        print "Finished the injection. %d events received." % (self.events)
        print "Total rate: %f packets/s" %(float(self.events)/time_total)

class TPX3DataAcqSeq(TPX3SeqBase):

    """ Class for doing data acquisitions with Timepix3. The mode and duration in
    seconds can be chosen. Must be created within a test class derived from
    tpx3_test. """

    def __init__(self, test):
        TPX3SeqBase.__init__(self, test)
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


from tests.tpx3_test import *
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
        self.data = dict()
        self.data['toa'] = dict()
        self.data['tot'] = dict()
        self.events = 0
        self.TIMEOUT = 60
        self.rpt_interval = rpt_interval
        self.rpt_count = rpt_interval

    def get_data(self, data_type=""):
        if data_type == "":
            return self.data
        elif data_type in self.data:
            return self.data[data_type]
        else:
            print "Unknown data type %s given" % (data_type)


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

            for pck in data:
                if pck.isData():
                    self.process_packet(pck)
                else:
                    if pck.isEoR():
                        self.test.logging.info("Finished due to End-of-Readout")
                        finished = True
                    else:
                        pass
                        # print "Got packet %d" %(pck.type)
            time_elapsed = time.time() - time_start
            if self.rpt_count == 0:
                self.test.sample_temp(1, "DURING DAQ")
                self.test.logging.info("Packet count: %d" % (self.events))
                self.rpt_count = self.rpt_interval
            else:
                self.rpt_count -= 1
            if time_elapsed > self.TIMEOUT:
                self.logging.info("TIMEOUT %d reached" % (self.TIMEOUT))
                finished = True

        time_total =  time.time() - time_start
        print "Finished the injection. %d events received." % (self.events)
        print "Total rate: %f packets/s" %(float(self.events)/time_total)

    def process_packet(self, pck):
        if pck.toa in self.data['toa']:
            self.data['toa'][pck.toa] += 1
        else:
            self.data['toa'][pck.toa] = 1
        if pck.tot in self.data['tot']:
            self.data['tot'][pck.tot] += 1
        else:
            self.data['tot'][pck.tot] = 1
        self.events += 1

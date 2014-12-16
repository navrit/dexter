

class TPX3TestPulsePacketAnalyzer:
    """ This class can be used when doing measurements with testpulses. It checks
    that the packets are received from correct addresses."""

    def __init__(self, test, check = True, max_errors = 10):
        self.error_checking = check
        self.finished = False
        self.test = test
        self.data = dict()
        self.fields = ['toa', 'tot', 'col', 'row']
        for field in self.fields:
            self.data[field] = dict()
        self.events = 0
        self.analyses = 0
        self.non_data_eor = 0
        self.row_errors = 0
        self.col_errors = 0
        self.packet_errors = 0
        self.name = "TPX3PacketAnalyzer"
        self.max_errors = max_errors

    def num_events(self):
        return self.events

    def get_data(self, data_type = ""):
        if data_type == "":
            return self.data
        elif data_type in self.data:
            return self.data[data_type]
        else:
            print "Unknown data type %s given" % (data_type)
            return None

    def analyze_packets(self, data):
        """ Given list of packets, analyzes them and extracts data from them"""
        self.analyses += 1
        for pck in data:
            if pck.isData():
                self.process_packet(pck)
            elif pck.isEoR():
                self.finished = True
            else:
                self.non_data_eor += 1

    def set_pixel_tp_mask(self, mask):
        self.pixel_tp_mask = mask

    def set_ctpr_mask(self, mask):
        self.ctpr_mask = mask

    def is_finished(self):
        return self.finished

    def process_packet(self, pck):
        """ Records all information in one packet"""
        if self.is_expected(pck) or self.error_checking is False:
            if pck.toa in self.data['toa']:
                self.data['toa'][pck.toa] += 1
            else:
                self.data['toa'][pck.toa] = 1
            if pck.tot in self.data['tot']:
                self.data['tot'][pck.tot] += 1
            else:
                self.data['tot'][pck.tot] = 1
            if pck.col in self.data['col']:
                self.data['col'][pck.col] += 1
            else:
                self.data['col'][pck.col] = 1
            if pck.row in self.data['row']:
                self.data['row'][pck.row] += 1
            else:
                self.data['row'][pck.row] = 1
            self.events += 1
        else:
            if self.packet_errors < self.max_errors:
                self.test.logging.error(
                    "%s Unexpected packet received from %d, %d" %
                    (self.name, pck.col, pck.row))
            self.packet_errors += 1
            if self.packet_errors == self.max_errors:
                self.test.logging.error(
                    "%s Max. error count %d reached." %
                    (self.name, self.max_errors))

    def is_expected(self, pck):
        """ Returns 1 if the packet is expected based on TP masks"""
        if self.ctpr_mask[pck.col] == 0:
            self.col_errors += 1
            return False
        if self.pixel_tp_mask[pck.row] == 0:
            self.row_errors += 1
            return False
        return True

    def report(self):
        """ Returns a string containing end of measurement report """
        result = "Packet analysis report\n"
        result += "\tTotal events: %d\n" % (self.events)
        result += "\tTotal errors: %d\n" % (self.packet_errors)
        return result

    def data_to_files(self):
        for field in self.fields:
            hist = self.data[field]
            fname = open(self.test.fname + "/hist_" + field + ".csv", "w")
            sorted(hist, key=int)
            for key in hist.keys():
                fname.write("%d, %d\n" % (key, hist[key]))
            fname.close()

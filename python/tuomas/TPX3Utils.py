import time
import gpib


class GPIBdev:

    """GPIB class for reading a device."""

    def __init__(self, address=22):
        self.address = address

    def connect(self):
        present = gpib.listener(0, self.address)
        if not present:
            return False
        self.dev = gpib.dev(0, self.address)
        return True

    def wr(self, m):
        m = m.rstrip()
        gpib.write(self.dev, m+"\r\n")

    def rd(self, l=256):
        m = gpib.read(self.dev, l)
        return m

    def qr(self, m, l=256):
        self.wr(m)
        return self.rd(l)


class CurTempRecord:

    """ Data class for storing current-temperature value pairs. Supports multiple
    sampling of a value, then calculating average of the samples. """

    def __init__(self):
        self.db = dict()
        self.num_entries = 0
        self.order = []  # Keeps the order in which data was added

    def add(self, tag, cur, temp):
        """ Adds one current/temp pair with specified message. If the tag has
        already been added, appends values for that message. """
        if tag not in self.db:
            self.db[tag] = dict()
            self.db[tag]['Current'] = []
            self.db[tag]['Temperature'] = []
            self.order.append(tag)
        self.db[tag]['Current'].append(cur)
        self.db[tag]['Temperature'].append(temp)
        ++self.num_entries

    def get_avg(self, tag, value):
        """ Returns the average of sampled value (current or temperature). """
        values = self.db[tag][value]
        value_sum = 0
        for point in values:
            value_sum += point
        return value_sum / len(values)

    def to_string(self):
        """ Returns a string representation of the object. """
        result = "# Current/temperature values sampled\n"
        for item in self.order:
            tag = item
            cur_avg = self.get_avg(tag, 'Current')
            temp_avg = self.get_avg(tag, 'Temperature')

            result += "\t## " + tag + "\n" + "\tCurrent: %8.5f, " % (cur_avg) +\
                "Temperature: %8.5f" % (temp_avg)
            result += "\n\n"
        return result


class CSVRecordWithTime:

    """ Class for storing data as CSV tables. Samples the time automatically when data
    is added to the record. """

    def __init__(self):
        self.num_entries = 0
        self.entries = []
        self.columns = None
        self.start_time = time.time()
        self.times = []
        self.add_time = True
        self.tags = dict()

    def add(self, new_tuple, tag=""):
        """ Adds a tuple of values to the CSV record """
        self.num_entries += 1
        time_now = time.time()
        self.entries.append(new_tuple)
        self.times.append(time_now)
        if len(tag) > 0:
            self.tags[time_now] = tag

    def set_columns(self, col_tuple):
        self.columns = col_tuple

    def to_string(self):
        """ Returns a string representation of this CSV record """
        result = ""
        index = 0

        for col in self.columns:
            result += col + ","
        result += "\n"

        for entry in self.entries:
            time_this_index = self.times[index]
            if self.add_time:
                result += str(time_this_index - self.start_time)
            for value in entry:
                result += "," + str(value)
            if time_this_index in self.tags:
                result += "%s" % self.tags[time_this_index]
            result += "\n"
            index += 1
        return result


class CurrentTempSampler:

    """ A class for sampling Timepix3 current and temperature values. The number
    of samples can be specified and the class averages the value from these
    samples. """

    def __init__(self, test, meas_power=False, meas_temp=True):
        self.test = test
        self.tpx = test.tpx
        self.currents = []
        self.db = CurTempRecord()
        self.verbose = False
        self.samples = 10
        self.csv = CSVRecordWithTime()
        self.csv.set_columns(("Time", "Current", "Temperature"))
        self.temp_outside_limits = 0

        self.TEMP_LOWER = -25
        self.TEMP_UPPER = 150
        self.GPIB_ADDR = 10
        self.meas_power = meas_power
        self.meas_temp = meas_temp
        self.tag = ""

        if self.meas_power is True:
            if not self._create_and_connect_gpib_dev():
                self.test.logging.info("ERROR. Couldn't create/connect GPIB")
                return
        else:
            self.test.logging.warning("Current sampling is disabled.")

    def _create_and_connect_gpib_dev(self):
        """ Creates GPIB device and opens a connection"""
        gpib_addr = self.GPIB_ADDR
        gpib_dev = GPIBdev(gpib_addr)
        if not gpib_dev.connect():
            self.test.logging.info("No connection to GPIB device. Exiting...")
            return False
        self.gpib_dev = gpib_dev
        print "GPIB device was created properly."
        return True

    def set_tag(self, tag):
        """ Adds a tag to the sampler which is appended to the results"""
        self.tag = tag

    def sample_cur_and_temp(self, msg="", ind="", nsamples=0):
        """ Reads voltage/current from the gpib device and samples also temperature
        of Timepix3. Num. of samples can be set for each func call. """

        self._msg(ind + msg)

        if nsamples == 0:
            nsamples = self.samples

        for i in range(nsamples):
            temperature = 999

            while not self._temp_within_limits(temperature):
                (vol, cur, x, y, z) = 0, 0, 0, 0, 0
                if self.meas_power is True:
                    line = self.gpib_dev.qr("read?")
                    vol, cur, x, y, z = map(float, line.split(","))

                if self.meas_temp is True:
                    temperature = self.tpx.getTpix3Temp()
                else:
                    temperature = 0

                if self._temp_within_limits(temperature):
                    self.currents.append(cur)
                    self._msg(
                        ind + "\tVDD Voltage: %8.5f  Current: %8.5f" %
                        (vol, cur))
                    self._msg(ind + "\tTemperature: %8.5f" % (temperature))
                    self._store_measured_values(msg, cur, temperature)
                    self.csv.add((cur, temperature, self.tag))
                else:
                    self.test.logging.info("Warning. Sample rejected. Temp out of limits:\
            %8.5f" % (temperature))

    def _temp_within_limits(self, temperature):
        """ Checks that the temperature is within reasonable limits """
        if temperature > self.TEMP_UPPER or temperature < self.TEMP_LOWER:
            self.temp_outside_limits += 1
            return 0
        else:
            return 1

    def get_db(self):
        return self.db

    def get_csv(self):
        return self.csv

    def _store_measured_values(self, msg, cur, temp):
        """ Stores measured current and temperature"""
        self.db.add(msg, cur, temp)

    def _msg(self, msg):
        if self.verbose is True:
            self.test.logging.info(msg)

    def results_to_files(self):
        """ Prints the sampled values into files."""
        fresult = open(self.test.fname + "/current_temp.dat", "w")
        db = self.get_db()
        fresult.write(db.to_string())
        fresult.close()

        fresult = open(self.test.fname + "/current_temp.csv", "w")
        csv = self.get_csv()
        fresult.write(csv.to_string())
        fresult.close()

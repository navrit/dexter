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

    def add(self, new_tuple):
        """ Adds a tuple of values to the CSV record """
        self.num_entries += 1
        self.entries.append(new_tuple)
        self.times.append(time.time())

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
            if self.add_time:
                result += str(self.times[index] - self.start_time)
            for value in entry:
                result += "," + str(value)
            result += "\n"
            index += 1
        return result

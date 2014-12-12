
from tests.tpx3_test import *
from tests.dac_defaults import dac_defaults


class TPX3ConfBase:

    """Base class for configurations of Timepix3"""

    def __init__(self, test, debug=False):
        self.tpx = test.tpx
        self.test = test
        self.debug = debug

    def do_config(self):
        """Override this in your derived class"""
        print "do_config() not implemented in the base class"


class TPX3ConfMatrixTPEnable(TPX3ConfBase):

    """Configures test pulse for all pixels, unmasks them and sets the DACS."""

    def __init__(self, test):
        TPX3ConfBase.__init__(self, test)
        self.conf_data = list(range(256))
        for x in range(256):
            self.conf_data[x] = list(range(256))
            for y in range(256):
                self.conf_data[x][y] = dict()
                self.conf_data[x][y]['mask'] = 1
                self.conf_data[x][y]['tp'] = 0
                self.conf_data[x][y]['dac'] = 0

    def get_x_y(self, x, y, field):
        if field in self.conf_data[x][y]:
            return self.conf_data[x][y][field]

    def set_x_y(self, x, y, field, value):
        self.conf_data[x][y][field] = value

    def num_tp_enabled(self, mask):
        """Returns the number of pixels having TP enabled"""
        result = 0
        for x in range(256):
            for y in range(256):
                if self.get_x_y(x, y, 'tp'):
                    if mask[x] == 1:
                        result += 1
        return result

    def do_config(self):
        self.tpx.resetPixelConfig()
        for x in range(256):
            for y in range(256):
                self.tpx.setPixelThreshold(x, y, 0x0F)
                self.tpx.setPixelMask(x, y, 1)
                self.tpx.setPixelTestEna(x, y, self.pixel_tp_mask[y])
                self.set_x_y(x, y, 'dac', 0x0F)
                self.set_x_y(x, y, 'tp', self.pixel_tp_mask[y])
                self.set_x_y(x, y, 'mask', 1)

        self.tpx.setPixelConfig()

    def get_pixel_tp_mask(self):
        return self.pixel_tp_mask

    def set_pixel_tp_mask(self, everyNpixels):
        self.pixel_tp_mask = list(range(256))
        tmp_count = everyNpixels
        for i in range(256):
            if tmp_count == 1:
                self.pixel_tp_mask[i] = 1
                tmp_count = everyNpixels
            else:
                self.pixel_tp_mask[i] = 0
                tmp_count -= 1
        for x in range(256):
            for y in range(256):
                self.set_x_y(x, y, 'tp', self.pixel_tp_mask[y])
        if self.debug:
            print self.pixel_tp_mask


class TPX3ConfBeforeDAQ(TPX3ConfBase):

    """Configures PLL, set DAC defaults and other operations before a DAQ"""

    def do_config(self):
        tpx = self.tpx
        dac_defaults(tpx)
        tpx.setPllConfig(
            (TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK |
                TPX3_PHASESHIFT_DIV_8 | TPX3_PHASESHIFT_NR_16 | 0x14 <<
                TPX3_PLLOUT_CONFIG_SHIFT))
        tpx.setOutputMask(0xFF)
        tpx.shutterOff()
        tpx.resetPixels()
        genConfig_register = TPX3_ACQMODE_TOA_TOT | TPX3_FASTLO_ENA | TPX3_GRAYCOUNT_ENA
        tpx.t0Sync()
        tpx.setGenConfig(genConfig_register)


class TPX3ConfTestPulses(TPX3ConfBase):

    """ Configures the chip for digital testpulse injection. """

    def do_config(self):
        genConfig_register = self.tpx.getGenConfig()
        genConfig_register |= TPX3_SELECTTP_DIGITAL
        genConfig_register |= TPX3_TESTPULSE_ENA
        self.tpx.setGenConfig(genConfig_register)
        self.tpx.setTpPeriodPhase(self.period, self.phase)
        self.tpx.setTpNumber(self.npulses)
        self.tpx.setShutterLen(self.shutter_length)
        for col in range(256):
            self.tpx.setCtprBit(col, self.mask[col])
        # self.tpx.setCtprBits(1)
        self.tpx.setCtpr()

    def set_tp_config(self, period, npulses, phase=0):
        self.period = period
        self.npulses = npulses
        self.phase = phase
        self.shutter_length = int(((2*(64*period+1)*npulses)/40) + 100)

    def set_column_mask(self, mask):
        if len(mask) != 256:
            print "ERROR. CTPR Mask of invalid length %d given. Must be 256." % (len(mask))
        else:
            self.mask = list(range(256))
            for bit in range(256):
                self.mask[bit] = mask[bit]

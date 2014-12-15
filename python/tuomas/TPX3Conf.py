
from tests.tpx3_test import *
from tests.dac_defaults import dac_defaults
import numpy

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


class TPX3ConfAndReadPower(TPX3ConfBase):

    """Class for hiding some basic configuration code"""

    def __init__(self, tpx, test, read_power=True):
        self.tpx = tpx
        self.test = test
        self.read_power = read_power  # If specified, reads power to GPIB

    def do_config(self):
        """ Configures Timepix3 for DACQ"""
        tpx = self.tpx
        test = self.test
        clk_period = test.clk_period
        phase_shift = TPX3_PHASESHIFT_DIV_2

        self.tpx.shutterOff()
        tpx.resetPixels()
        dac_defaults(tpx)
        self.tpx.setCtprBits(0)
        self.tpx.setCtpr()
        self.set_mask_all_pixels(1)

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
                self.tpx.shutterOn()
                self.do_read_power(
                    "Clk: %d" %
                    (clk_period) +
                    " Shutter OPEN, ToA counter ON",
                    "\t\t")
                self.tpx.shutterOff()


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

        self.do_read_power("After masking all pixels", "\t")
        self.tpx.datadrivenReadout()
        self.do_read_power("In data-driven readout mode", "\t")

        #self.configure_all_pixels()

        self.tpx.resetTimer()
        self.tpx.t0Sync()
        self.tpx.openShutter(sleep=False)
        self.tpx.shutterOn()
        self.do_read_power("After opening the shutter", "\t")

        # At this point, the shutter is open and the chip is ready to take data

    def set_clk_period_and_phase(self, period, phase_div, phase_num):
        self.tpx.setPllConfig(
            (TPX3_PLL_RUN | TPX3_VCNTRL_PLL | TPX3_DUALEDGE_CLK | phase_div |
             phase_num | 0x14 << TPX3_PLLOUT_CONFIG_SHIFT))

    def do_read_power(self, msg, indent=""):
        """ Reads power through GPIB if the read_power flag is True"""
        if self.read_power is True:
            print "Reading temp and current..."
            self.test.sample_cur_and_temp(msg, indent)

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

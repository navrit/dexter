#ifndef MPX3EQ_COMMON_H
#define MPX3EQ_COMMON_H

#include "QColor"

#ifndef WIN32
#include <unistd.h>
#define Sleep(ms) usleep(ms*1000)
#endif

enum class win_status { startup, connected, disconnected };

static const QColor COLOR_TABLE[] = {
        Qt::red, // 1
        Qt::black,
        Qt::darkRed,
        Qt::green,
        Qt::darkGreen,
        Qt::blue,
        Qt::darkBlue,
        Qt::cyan,
        Qt::darkCyan,
        Qt::magenta, // 10
        Qt::darkMagenta,
        Qt::yellow,
        Qt::darkYellow,
        QColor( "darkorange" ),
        QColor( "purple" ),
        QColor( "khaki" ),
        QColor( "gold" ),
        QColor( "dodgerblue" ), // 18
        QColor( "light gray" ),
        QColor( "medium gray" ),
        QColor( "red" ),
        QColor( "green" ),
        QColor( "blue" ),
        QColor( "cyan" ),
        QColor( "magenta" ),
        QColor( "yellow" ),
        QColor( "dark yellow" ) // 27
};

typedef enum {
    __VERBL_ERR = 0,
    __VERBL_INFO,
    __VERBL_DEBUG
} verblev;

const static int __matrix_size_x = 256;
const static int __matrix_size_y = 256;
const static int __matrix_size_colour_x = 128;
const static int __matrix_size_colour_y = 128;
const static int __max_colours = 8;

const static int __matrix_size = (__matrix_size_x * __matrix_size_y);
const static int __matrix_size_colour = (__matrix_size_colour_x * __matrix_size_colour_y);

const static int __max_adj_val = 0x1F; // 0x1F = 31 is the max adjustment with 5 bits

// DACs
const static double __voltage_DACS_MAX = (3.3/2.); // This is how it's setup in hardware
                                    //! This is screwed up on W521 F9 E9 H9 G9, needs to be 3.4 V for that one for some reason
                                    //! Also chips 2 and 3 (last 2) have flat DACs but they can be set
                                    //!
                                    //! This is the reference voltage - not passed to the analog circuit (it gets ~1.6V)
const static int __maxADCCounts	  = 4095; // 12 bits
const static int __max_DAC_range  = 512;
const static int __half_DAC_range = __max_DAC_range/2;

/* Maximum frame rates based on the manual v1.4, Pg. 50 */
const static int __maximumFPS_1_bit = 24414;
const static int __maximumFPS_6_bit = 4069;
const static int __maximumFPS_12_bit = 2034;
const static int __maximumFPS_24_bit = __maximumFPS_12_bit/2; /* This needs to be confirmed... */

const static int __minimumShutterDownTime_ms = 1; /* Units are milliseconds */

const static int __max_number_of_chips = 4;
const static int __max_number_of_thresholds = 8;

#endif

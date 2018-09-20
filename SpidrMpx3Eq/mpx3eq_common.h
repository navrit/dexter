#ifndef MPX3EQ_COMMON_H
#define MPX3EQ_COMMON_H

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

#define __matrix_size_x 256
#define __matrix_size_y 256
#define __matrix_size_color_x 128
#define __matrix_size_color_y 128
#define __max_colors            8

#define __array_size_x          __matrix_size_x
#define __array_size_y          __matrix_size_y
#define __matrix_size           (__array_size_x*__array_size_y)
#define __matrix_size_color 	(__matrix_size_color_x*__matrix_size_color_y)

#define __max_adj_val  	0x1F // 0x1F = 31 is the max adjustment with 5 bits

#ifndef WIN32
#include <unistd.h>
#define Sleep(ms) usleep(ms*1000)
#endif

// DACs
// #define __maxADCCounts	       65535 // 16 bits
#define __voltage_DACS_MAX 	     (3.3/2.) // This is how it's setup in hardware
                                    //! This is screwed up on W521 F9 E9 H9 G9, needs to be 3.4 V for that one for some reason
                                    //! Also chips 2 and 3 (last 2) have flat DACs but they can be set
                                    //!
                                    //! This is the reference voltage - not passed to the analog circuit (it gets ~1.6V)
#define __maxADCCounts	        4095 // 12 bits
#define __max_DAC_range         512
#define __half_DAC_range        256

// Trigger
#define __min_trigger_deadtime_ms 1000

// Data
#define __nThresholdsPerSpectroscopicPixel  4

#endif

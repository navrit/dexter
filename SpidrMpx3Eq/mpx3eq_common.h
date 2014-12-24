#ifndef MPX3EQ_COMMON_H
#define MPX3EQ_COMMON_H

typedef enum {
	__VERBL_ERR = 0,
	__VERBL_INFO,
	__VERBL_DEBUG
} verblev;

#define __array_size_x 	256
#define __array_size_y 	256
#define __matrix_size 	__array_size_x*__array_size_y
#define __max_adj_val  	0x1F // 0x1F = 31 is the max adjustment with 5 bits

#ifndef WIN32
#include <unistd.h>
#define Sleep(ms) usleep(ms*1000)
#endif

// DACs
#define __voltage_DACS_MAX 	(3.3/2.) // This is how it's setup in hardware
#define __maxADCCounts	       65535 // 16 bits
#define __max_DAC_range			 512

#endif

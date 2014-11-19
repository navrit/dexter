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


#include <unistd.h>
#define Sleep(ms) usleep(ms*1000)

#endif

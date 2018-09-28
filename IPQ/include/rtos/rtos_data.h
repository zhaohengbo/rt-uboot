#ifndef	__RTOS_RTOS_DATA_H
#define __RTOS_RTOS_DATA_H

typedef	struct	rtos_data {
	unsigned long	ram_size;
	unsigned long   ram_buttom;
	unsigned long	rtos_mem_start;
	unsigned long	rtos_mem_end;
} rd_t;

extern rd_t rd;

#endif
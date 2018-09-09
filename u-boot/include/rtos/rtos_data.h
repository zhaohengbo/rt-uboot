#ifndef	__RTOS_RTOS_DATA_H
#define __RTOS_RTOS_DATA_H

typedef	struct	rtos_data {
	unsigned long	vector_addr;
	unsigned long	rtos_mem_start;
	unsigned long	rtos_mem_end;
	unsigned long	cpu_hz;
} rd_t;


#endif
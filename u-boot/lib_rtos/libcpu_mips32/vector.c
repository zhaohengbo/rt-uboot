#include <rtthread.h>
#include <rthw.h>
#include <rtos/rtos_data.h>

#include "mipsregs.h"

extern void mips_irq_handle(void);
extern void  mips_ecpt_handle(void);
extern void  mips_tlb_handle(void);
extern void  mips_cache_handle(void);

register rt_uint32_t $GP __asm__ ("$28");

const rt_uint8_t jump_program[]=
{
	0x3C, //li k0 0x00000000
	0x1A,
	0x00, //offset_2  gp
	0x00, //offset_3  gp
	0x37,
	0x5A,
	0x00, //offset_6  gp
	0x00, //offset_7  gp
	0x3C, //li k1 0x00000000
	0x1B,
	0x00, //offset_10  entry
	0x00, //offset_11  entry
	0x37,
	0x7B,
	0x00, //offset_14  entry
	0x00, //offset_15  entry
	0x03, //jr k1
	0x60,
	0x00,
	0x08,
	0x00, //nop
	0x00,
	0x00,
	0x00
};
	

void mips_vector_fill(rt_uint32_t address,rt_uint32_t pc_pointer)
{
	rt_uint32_t gp_data;
	rt_uint32_t idx;
	rt_uint8_t *pt;
	gp_data = $GP;
	
	pt = (rt_uint8_t *)address;
	
	for(idx=0;idx<sizeof(jump_program);idx++)
	{
		pt[idx] = jump_program[idx];
	}
	
	pt[2] = (gp_data&0xFF000000) >> 24;
	pt[3] = (gp_data&0x00FF0000) >> 16;
	pt[6] = (gp_data&0x0000FF00) >> 8;
	pt[7] = (gp_data&0x000000FF);	
	
	pt[10] = (pc_pointer&0xFF000000) >> 24;
	pt[11] = (pc_pointer&0x00FF0000) >> 16;
	pt[14] = (pc_pointer&0x0000FF00) >> 8;
	pt[15] = (pc_pointer&0x000000FF);
	
}

void rt_hw_vector_init(rd_t *rd)
{
	rt_uint32_t uncached_address;
	rt_uint32_t idx;
	rt_uint32_t *pt;
	
	write_c0_ebase(rd->vector_addr);
	
	uncached_address = rd->vector_addr | 0x20000000;
	
	pt = (rt_uint32_t *)uncached_address;
	
	for(idx=0;idx<0x400;idx++)
	{
		pt[idx] = 0;
	}
	
	mips_vector_fill(uncached_address,(rt_uint32_t)&mips_tlb_handle);
	mips_vector_fill(uncached_address + 0x100,((rt_uint32_t)&mips_cache_handle | 0x20000000));
	mips_vector_fill(uncached_address + 0x180,(rt_uint32_t)&mips_ecpt_handle);
	mips_vector_fill(uncached_address + 0x200,(rt_uint32_t)&mips_irq_handle);
	
}
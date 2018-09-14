#include <rtthread.h>
#include <rthw.h>
#include <rtos/rtos_data.h>

#include "mipsregs.h"

extern void mips_irq_handle(void);
extern void  mips_ecpt_handle(void);
extern void  mips_tlb_handle(void);
extern void  mips_cache_handle(void);

register rt_uint32_t $GP __asm__ ("$28");

const rt_uint32_t jump_program[]=
{
	0x3C1A0000,//li k0 0x00000000
	0x375A0000,
	0x3C1B0000,//li k1 0x00000000
	0x377B0000,
	
	0x03600008,//jr k1
	0x00000000 //nop
};
	

void mips_vector_fill(rt_uint32_t address,rt_uint32_t pc_pointer)
{
	rt_uint32_t gp_data;
	rt_uint32_t idx;
	rt_uint32_t *pt;
	gp_data = $GP;
	
	pt = (rt_uint32_t *)address;
	
	for(idx=0;idx<(sizeof(jump_program) / sizeof(rt_uint32_t));idx++)
	{
		pt[idx] = jump_program[idx];
	}
	
	pt[0] |= (gp_data&0xFFFF0000) >> 16;
	
	pt[1] |= (gp_data&0x0000FFFF);	
	
	pt[2] |= (pc_pointer&0xFFFF0000) >> 16;
	
	pt[3] |= (pc_pointer&0x0000FFFF);
	
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
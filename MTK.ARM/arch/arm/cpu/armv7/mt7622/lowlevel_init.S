/*
 * (C) Copyright 2012 Stephen Warren
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

.globl lowlevel_init
lowlevel_init:
    /*Enable SMP bit*/
	mrc	p15, 0, r0, c1, c0, 1
	orr	r0, r0, #0x40
	mcr	p15, 0, r0, c1, c0, 1
	mov	pc, lr

.global jumparch64_smc
jumparch64_smc:
	mov r3, r2
	mov r2, r1
	mov r1, r0
	ldr r0, =0x82000200  /*please check MTK_SIP_KERNEL_TMP_AARCH32 in  vendor/arm/atf/include/bl31/services/sip_svc.h*/ 
#	smc #0
	.word   0xe1600070  @ SMC #0 to enter monitor - hand assembled	
	b   .
	

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

/*pre-loader to uboot argument Location*/
.global BOOT_ARGUMENT_LOCATION
BOOT_ARGUMENT_LOCATION:
        .word 0x00000000

.globl save_boot_params
save_boot_params:
	ldr     r6, =BOOT_ARGUMENT_LOCATION 
	str     r4, [r6]
	mov	pc, lr

.globl lowlevel_init
lowlevel_init:
    /*Enable SMP bit*/
	mrc	p15, 0, r0, c1, c0, 1
	orr	r0, r0, #0x40
	mcr	p15, 0, r0, c1, c0, 1
	mov	pc, lr

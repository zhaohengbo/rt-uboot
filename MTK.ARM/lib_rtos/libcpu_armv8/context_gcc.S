/*
 * File      : context.S
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2013, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2013-07-05     Bernard      the first version
 */
 
.text

/*
 * void rt_hw_context_switch_to(rt_uint32 to);
 * r0 --> to
 */
.globl rt_hw_context_switch_to
rt_hw_context_switch_to:
    RESTORE_CONTEXT

.text
/*
 * void rt_hw_context_switch(rt_uint32 from, rt_uint32 to);
 * r0 --> from
 * r1 --> to
 */
.globl rt_hw_context_switch
rt_hw_context_switch:

	MOV		X2,X0
	MOV		X3,X1

	SAVE_CONTEXT
	
    STR X0, [X2]            @ store sp in preempted tasks TCB
    LDR x0, [X3]            @ get new task stack pointer
	
	RESTORE_CONTEXT

/*
 * void rt_hw_context_switch_interrupt(rt_uint32 from, rt_uint32 to);
 */
.globl _rt_thread_switch_interrupt_flag
.globl _rt_interrupt_from_thread
.globl _rt_interrupt_to_thread
.globl rt_hw_context_switch_interrupt
rt_hw_context_switch_interrupt:
    LDR 	X2, _rt_thread_switch_interrupt_flag
    LDR 	X3, [X2]
    CMP 	X3, #1
    B.EQ 	_reswitch
    LDR 	X4, _rt_interrupt_from_thread   @ set rt_interrupt_from_thread
    MOV 	X3, #1              @ set rt_thread_switch_interrupt_flag to 1
    STR 	X0, [X4]
    STR 	X3, [X2]
_reswitch:
    LDR 	X2, _rt_interrupt_to_thread     @ set rt_interrupt_to_thread
    STR 	X1, [X2]
    RET

.macro SAVE_CONTEXT

	/* Switch to use the EL0 stack pointer. */
	MSR 	SPSEL, #0

	/* Save the entire context. */
	STP 	X0, X1, [SP, #-0x10]!
	STP 	X2, X3, [SP, #-0x10]!
	STP 	X4, X5, [SP, #-0x10]!
	STP 	X6, X7, [SP, #-0x10]!
	STP 	X8, X9, [SP, #-0x10]!
	STP 	X10, X11, [SP, #-0x10]!
	STP 	X12, X13, [SP, #-0x10]!
	STP 	X14, X15, [SP, #-0x10]!
	STP 	X16, X17, [SP, #-0x10]!
	STP 	X18, X19, [SP, #-0x10]!
	STP 	X20, X21, [SP, #-0x10]!
	STP 	X22, X23, [SP, #-0x10]!
	STP 	X24, X25, [SP, #-0x10]!
	STP 	X26, X27, [SP, #-0x10]!
	STP 	X28, X29, [SP, #-0x10]!
	STP 	X30, XZR, [SP, #-0x10]!

	/* Save the SPSR. */
#if defined( GUEST )
	MRS		X3, SPSR_EL1
	MRS		X2, ELR_EL1
#else
	MRS		X3, SPSR_EL3
	/* Save the ELR. */
	MRS		X2, ELR_EL3
#endif

	STP 	X2, X3, [SP, #-0x10]!

	MOV 	X0, SP   /* Move SP into X0 for saving. */

	/* Switch to use the ELx stack pointer. */
	MSR 	SPSEL, #1

	.endm

; /**********************************************************************/

.macro RESTORE_CONTEXT

	/* Switch to use the EL0 stack pointer. */
	MSR 	SPSEL, #0

	/* Set the SP to point to the stack of the task being restored. */
	MOV		SP, X0

	LDP 	X2, X3, [SP], #0x10  /* SPSR and ELR. */
	
	/* Restore the SPSR. */
	MSR		SPSR_EL3, X3 /*_RB_ Assumes started in EL3. */
	/* Restore the ELR. */
	MSR		ELR_EL3, X2

	LDP 	X30, XZR, [SP], #0x10
	LDP 	X28, X29, [SP], #0x10
	LDP 	X26, X27, [SP], #0x10
	LDP 	X24, X25, [SP], #0x10
	LDP 	X22, X23, [SP], #0x10
	LDP 	X20, X21, [SP], #0x10
	LDP 	X18, X19, [SP], #0x10
	LDP 	X16, X17, [SP], #0x10
	LDP 	X14, X15, [SP], #0x10
	LDP 	X12, X13, [SP], #0x10
	LDP 	X10, X11, [SP], #0x10
	LDP 	X8, X9, [SP], #0x10
	LDP 	X6, X7, [SP], #0x10
	LDP 	X4, X5, [SP], #0x10
	LDP 	X2, X3, [SP], #0x10
	LDP 	X0, X1, [SP], #0x10

	/* Switch to use the ELx stack pointer.  _RB_ Might not be required. */
	MSR 	SPSEL, #1

	ERET

	.endm


.text
    .align  8
.globl vector_fiq
vector_fiq:
    SAVE_CONTEXT
	STTR	X0 [SP], #0x10
    BL      rt_hw_trap_fiq
	LPTR	X0 [SP], #0x10
    RESTORE_CONTEXT

.globl      rt_interrupt_enter
.globl      rt_interrupt_leave
.globl      rt_thread_switch_interrupt_flag
.globl      rt_interrupt_from_thread
.globl      rt_interrupt_to_thread

.globl      rt_current_thread

    .align  8
.globl vector_irq
vector_irq:
    SAVE_CONTEXT
	STTR	X0 [SP], #0x10

    BL      rt_interrupt_enter
    BL      rt_hw_trap_irq
    BL      rt_interrupt_leave
	
	LPTR	X0 [SP], #0x10

    @ if rt_thread_switch_interrupt_flag set, jump to
    @ rt_hw_context_switch_interrupt_do and don't return
    LDR 	X1, _rt_thread_switch_interrupt_flag
    LDR     X2, [X1]
    CMP     X2, #1
    B.NEQ     vector_irq_exit

    MOV     X2,  #0         @ clear flag
    STR     X2,  [X1]

    LDR     X3,  _rt_interrupt_from_thread
    LDR     X4,  [X3]
	STR     x0,  [X4]       @ store sp in preempted tasks's TCB

    LDR     x3,  _rt_interrupt_to_thread
    LDR     X4,  [X3]
    LDR     x0,  [X4]       @ get new task's stack pointer
	
vector_irq_exit:	
	RESTORE_CONTEXT

    .align  8
    .globl  vector_swi
vector_swi:
    SAVE_CONTEXT
    BL      rt_hw_trap_swi
    B       .

    .align  8
    .globl  vector_undef
vector_undef:
    SAVE_CONTEXT
    BL      rt_hw_trap_undef
    B       .

    .align  8
    .globl  vector_pabt
vector_pabt:
    SAVE_CONTEXT
    BL      rt_hw_trap_pabt
    B       .

    .align  8
    .globl  vector_dabt
vector_dabt:
    SAVE_CONTEXT
    BL      rt_hw_trap_dabt
    B       .

    .align  8
    .globl  vector_resv
vector_resv:
    SAVE_CONTEXT
    BL      rt_hw_trap_resv
    B       .

.globl rt_thread_switch_interrupt_flag
.globl rt_interrupt_from_thread
.globl rt_interrupt_to_thread

_rt_thread_switch_interrupt_flag:
	.word rt_thread_switch_interrupt_flag
_rt_interrupt_from_thread:
	.word rt_interrupt_from_thread
_rt_interrupt_to_thread:
	.word rt_interrupt_to_thread
	
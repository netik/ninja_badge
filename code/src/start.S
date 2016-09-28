/*
 *  armboot - Startup Code for ARM720 CPU-core
 *
 *  Copyright (c) 2001	Marius Gröger <mag@sysgo.de>
 *  Copyright (c) 2002	Alex Züpke <azu@sysgo.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include "taskdefs.h"


/* Standard definitions of Mode bits and Interrupt (I & F) flags in PSRs */
	
	.equ    IRQ_DISABLE,          0x80      /* when I bit is set, IRQ is disabled */
	.equ    FIQ_DISABLE,          0x40      /* when F bit is set, FIQ is disabled */
	
	.equ    USR_MODE,       0x10
	.equ    FIQ_MODE,       0x11
	.equ    IRQ_MODE,       0x12
	.equ    SVC_MODE,       0x13
	.equ    ABT_MODE,       0x17
	.equ    UND_MODE,       0x1B
	.equ    SYS_MODE,       0x1F

#	.equ    usr_stack_size, 1024
#	.equ    irq_stack_size,  512
#	.equ    fiq_stack_size,   16
#	.equ    und_stack_size,  256
#	.equ    abt_stack_size,   16
#	.equ    sup_stack_size,   16

	
/*
 *************************************************************************
 *
 * Jump vector table as in table 3.1 in [1]
 *
 *************************************************************************
 */


.set base, .
.set _rom_data_init, 0x108d0
	

	.globl _start
_start:	b       _begin
	ldr	pc, _undefined_instruction
	ldr	pc, _swi_handler
	ldr	pc, _prefetch_abort
	ldr	pc, _data_abort
	ldr	pc, _not_used
	ldr	pc, _irq_handler
	ldr	pc, _fiq

#ifdef USE_ROM_VARS	
/* these vectors are used for rom patching */	
.org 0x20
.code 16
_RPTV_0_START:
	bx lr /* do nothing */

.org 0x60
_RPTV_1_START:
	bx lr /* do nothing */

.org 0xa0
_RPTV_2_START:
	bx lr /* do nothing */

.org 0xe0
_RPTV_3_START:
	bx lr /* do nothing */

.org 0x120
ROM_var_start: .word 0
.org 0x7ff
ROM_var_end:   .word 0
#endif /*USE_ROM_VARS*/

.code 32
.align
_begin:
	/* FIQ mode stack */
	msr	CPSR_c, #(FIQ_MODE | IRQ_DISABLE | FIQ_DISABLE)
	ldr	sp, =__fiq_stack_top__	/* set the FIQ stack pointer */

	/* IRQ mode stack */
	msr	CPSR_c, #(IRQ_MODE | IRQ_DISABLE | FIQ_DISABLE)
	ldr	sp, =__irq_stack_top__	/* set the IRQ stack pointer */

	/* Supervisor mode stack */
	msr	CPSR_c, #(SVC_MODE | IRQ_DISABLE | FIQ_DISABLE)
	ldr	sp, =__svc_stack_top__	/* set the SVC stack pointer */

	/* Undefined mode stack */
	msr	CPSR_c, #(UND_MODE | IRQ_DISABLE | FIQ_DISABLE)
	ldr	sp, =__und_stack_top__	/* set the UND stack pointer */

	/* Abort mode stack */
	msr	CPSR_c, #(ABT_MODE | IRQ_DISABLE | FIQ_DISABLE)
	ldr	sp, =__abt_stack_top__	/* set the ABT stack pointer */

	/* System mode stack */
	msr	CPSR_c, #(SYS_MODE | IRQ_DISABLE | FIQ_DISABLE)
	ldr	sp, =__sys_stack_top__	/* set the SYS stack pointer */

#ifdef USE_ROM_VARS	
	bl _rom_data_init+.-base
#endif	
	msr	CPSR_c, #(USR_MODE)

	b main

//	ldr     r1,=_system_stack
// 	msr     cpsr_c,#(SVC_MODE | I_BIT | F_BIT)
//	add     r1,r1,#sup_stack_size
// 	mov     sp,r1

// 	msr     cpsr_c,#(IRQ_MODE | I_BIT | F_BIT)
// 	add     r1,r1,#irq_stack_size
// 	mov     sp,r1

// 	msr     cpsr_c,#(FIQ_MODE | I_BIT | F_BIT)
// 	add     r1,r1,#fiq_stack_size
// 	mov     sp,r1
	
// 	msr     cpsr_c,#(ABT_MODE | I_BIT | F_BIT)
// 	add     r1,r1,#abt_stack_size
// 	mov     sp,r1

// 	msr     cpsr_c,#(UND_MODE | I_BIT | F_BIT)
// 	add     r1,r1,#und_stack_size
// 	mov     sp,r1

// 	msr     cpsr_c,#(USR_MODE | I_BIT | F_BIT)

//	bl _rom_data_init+.-base
//	msr     cpsr_c,#(SVC_MODE) // turn on interrupts --- for debug only
//	msr     cpsr_c,#(USR_MODE) // turn on interrupts --- for debug only		
// 	add     r1,r1,#usr_stack_size
// 	mov     sp,r1
	
_undefined_instruction:	.word undefined_instruction
_swi_handler:	.word swi_handler
_irq_handler:	.word irq_handler
_prefetch_abort:	.word prefetch_abort
_data_abort:		.word data_abort
_not_used:		.word not_used
_irq:			.word irq
_task_will_wakeup:  .word task_will_wakeup
_fiq:			.word fiq
	.balignl 16,0xdeadbeef


/*
 *************************************************************************
 *
 * Startup Code (reset vector)
 *
 * do important init only if we don't start from memory!
 * relocate armboot to ram
 * setup stack
 * jump to second stage
 *
 *************************************************************************
 */

_TEXT_BASE:
	.word	TEXT_BASE

.globl _armboot_start
_armboot_start:
	.word _start

/*
 * These are defined in the board-specific linker script.
 */
.globl _bss_start
_bss_start:
	.word __bss_start

.globl _bss_end
_bss_end:
	.word _end

_start_armboot: .word main

#_system_stack:
#	. = . + usr_stack_size + irq_stack_size + fiq_stack_size + und_stack_size + abt_stack_size + sup_stack_size

	
/*
 *************************************************************************
 *
 * CPU_init_critical registers
 *
 *************************************************************************
 */

cpu_init_crit:
	# actually do nothing for now!
    mov	pc, lr

.align 3
store_task:
    /* Save the current task state in the current task, if curtask is valid. */
    /* inputs:  r10 (spsr), r5 (value of current task) */
    ldr     r4, _tasks
    cmp     r5, #NTASKS
    bhi     exit_store

    mov     r6, #TASK_SIZE
    mla     r0, r6, r5, r4  

    mov     r4, lr
    mov     r3, r0
    mov     r1, sp
    mov     r2, #52
    bl      memcpy      /* copy r0-r12 from the stack to curtask */
    mov     lr, r4
    ldr     r2, [sp, #52]
    str     r2, [r3, #60]  /* copy r14 from the stack to curtask pc */
    str     r10, [r3, #64] /* copy SPSR to curtask */

    mrs     r2, CPSR
	msr	    cpsr_c, #(SYS_MODE | IRQ_DISABLE)
    str     r13, [r3, #52]  /* copy usermode r13 to curtask */
    str     r14, [r3, #56]  /* copy usermode r13 to curtask */
	msr	    cpsr, r2
exit_store:
    bx      lr

.align 3
restore_task:
    ldr     r4, _tasks
    ldr     r5, _curtask
    ldr     r5, [r5]
    mov     r6, #TASK_SIZE
    mov     r0, sp
    mla     r1, r6, r5, r4  
    mov     r2, #52
    mov     r4, lr
    bl      memcpy      /* copy r0-r12 */
    mov     lr, r4
    ldr     r2, [r1, #60]
    str     r2, [sp, #52]
    ldr     r2, [r1, #64]   // for spsr
 
    mrs     r3, CPSR
	msr	    cpsr_c, #(SYS_MODE | IRQ_DISABLE)
    msr     spsr, r2
    ldr     r13, [r1, #52]  /* copy r13 */
    ldr     r14, [r1, #56]
	msr	    cpsr, r3
    bx      lr

.align 3
msg2:
    .word msg2addr
msg2addr:
    .asciz "xxx\r\n\0";

.align 3
msg3:
    .word msg3addr
msg3addr:
    .asciz "zoot\r\n\0";

/*
 * exception handlers
 */
	.align  5
undefined_instruction:

	.align	5
swi_handler:
    stmfd   sp!, {r0-r12, lr}
	msr	    cpsr_c, #(SVC_MODE | IRQ_DISABLE)

    mrs     r10, SPSR
    tst     r10, #32 
    ldrneh  r11, [lr, #-2]
    bicne   r11, r11, #0xff00 
    ldreq   r11, [lr, #-4]
    biceq   r11, r11, #0xff000000 
    mov     r0, #0
    str     r0, rescheduled

    /* copy task: writes r0, r1, r2, r3, r4, r5, r6, reads r10 */

    ldr     r5, _curtask
    ldr     r5, [r5]
    bl      store_task 

    /* r3 is our value, r0-r2 are our arguments.  return value goes in r7. */
    cmp     r11, #6
    bhi     1f
    adr     r4, switable
    ldr     r0, [sp, #0]
    ldr     r1, [sp, #4]
    ldr     r2, [sp, #8]
    ldr     r3, [r4, r11, lsl#2]
    mov     lr, pc
    bx      r3

1:
    ldr     r8, rescheduled
    cmp     r8, #0
    streq   r0, [sp]
    beq     swi_return

    /* We've rescheduled.  Copy curtask regs to sp. */
    ldr     r3, _task_will_wakeup
    mov     lr, pc
    bx      r3
    bl      restore_task

swi_return:
    ldm     sp!, {r0-r12, pc}^


.align 3
switable:
    .word swi_sleep    
    .word swi_starttask
    .word swi_rfreceive
    .word swi_txpacket
    .word swi_chill
    .word swi_chill_packet_notify
    .word swi_sendpacket

.align 3
_tasks: .word tasks
_curtask: .word curtask 
.globl rescheduled
rescheduled: 
.word 0x0

// XXX remove
.align 3
msg:
    .word msgaddr
msgaddr:
    .asciz "swi\r\n\0";
.align 3
_put_hex32:
    .word put_hex32
_putstr:
    .word putstr

.align 3
.globl memcpy
memcpy:
    push    {r1-r3} 
2:
    cmp     r2, #0
    beq     1f
    ldrb    r3, [r1], #1
    strb    r3, [r0], #1
    sub     r2, #1
    b       2b
1:
    pop     {r1-r3}
    bx lr



	.align	5
irq_handler:
    sub     lr, lr, #4      // why is LR offset for IRQ/FIQ?  DDI01001 doesn't say.
    stmfd   sp!, {r0-r12, lr}

    mov     r0, lr
    cmp     r0, #0xf00000
    bls     XXXzoo
    ldr     r3, _put_hex32
    mov     lr, pc
    bx      r3
XXXzoo:

    mov     r0, #0
    str     r0, rescheduled
    ldr     r5, _curtask
    ldr     r5, [r5]

    ldr	    r3, _irq
    mov     lr, pc
    bx      r3

    ldr     r8, rescheduled
    cmp     r8, #0
    beq     irq_return 

    mrs     r10, SPSR
    bl      store_task 
    ldr     r3, _task_will_wakeup
    mov     lr, pc
    bx      r3

    bl      restore_task

irq_return:
    ldmfd     sp!, {r0-r12, pc}^
    //ldmfd   sp!, {r0-r12, lr}
    //add     lr, lr, #4
    //subs    pc, lr, #4      


	.align	5
prefetch_abort:
    ldr     r0, msg3
    ldr     r3, _putstr
    mov     lr, pc
    bx      r3
	nop
	.align	5
data_abort:
    ldr     r0, msg3
    ldr     r3, _putstr
    mov     lr, pc
    bx      r3

	.align	5
not_used:
    ldr     r0, msg3
    ldr     r3, _putstr
    mov     lr, pc
    bx      r3


	.align	5
fiq:
    ldr     r0, msg3
    ldr     r3, _putstr
    mov     lr, pc
    bx      r3

	.align	5

.globl reset_cpu
reset_cpu:
	mov     pc, r0

.globl lock_nowait
.type lock_nowait, function
lock_nowait:
    push    {r1-r2}
    mov     r2, #1
    swp     r1, r2, [r0]
    cmp     r1, r2
    movne   r0, #1
    moveq   r0, #0
    pop     {r1-r2}
    bx      lr

.globl lock_wait
.type lock_wait, function
lock_wait:
    push    {r1-r2}
lock_loop:
    mov     r2, #1
    swp     r1, r2, [r0]
    cmp     r1, r2
    beq     lock_loop  

    pop     {r1-r2}
    bx      lr

.globl lock_unlock
.type lock_unlock, function
lock_unlock:
    push    {r1-r2}
    mov     r2, #0
    swp     r1, r2, [r0]
    pop     {r1-r2}
    bx lr

.globl get_cpsr
.type get_cpsr, function
get_cpsr:
    mrs     r0, CPSR
    bx lr


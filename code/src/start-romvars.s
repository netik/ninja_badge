# 1 "src/start.S"
# 1 "/home/bc/Desktop/badge/badge2010/code//"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "src/start.S"
# 25 "src/start.S"
# 1 "src/taskdefs.h" 1
# 26 "src/start.S" 2




 .equ IRQ_DISABLE, 0x80
 .equ FIQ_DISABLE, 0x40

 .equ USR_MODE, 0x10
 .equ FIQ_MODE, 0x11
 .equ IRQ_MODE, 0x12
 .equ SVC_MODE, 0x13
 .equ ABT_MODE, 0x17
 .equ UND_MODE, 0x1B
 .equ SYS_MODE, 0x1F

# .equ usr_stack_size, 1024
# .equ irq_stack_size, 512
# .equ fiq_stack_size, 16
# .equ und_stack_size, 256
# .equ abt_stack_size, 16
# .equ sup_stack_size, 16
# 58 "src/start.S"
.set base, .
.set _rom_data_init, 0x108d0


 .globl _start
_start: b _begin
 ldr pc, _undefined_instruction
 ldr pc, _swi_handler
 ldr pc, _prefetch_abort
 ldr pc, _data_abort
 ldr pc, _not_used
 ldr pc, _irq_handler
 ldr pc, _fiq



.org 0x20
.code 16
_RPTV_0_START:
 bx lr

.org 0x60
_RPTV_1_START:
 bx lr

.org 0xa0
_RPTV_2_START:
 bx lr

.org 0xe0
_RPTV_3_START:
 bx lr

.org 0x120
ROM_var_start: .word 0
.org 0x7ff
ROM_var_end: .word 0


.code 32
.align
_begin:

 msr CPSR_c, #(FIQ_MODE | IRQ_DISABLE | FIQ_DISABLE)
 ldr sp, =__fiq_stack_top__


 msr CPSR_c, #(IRQ_MODE | IRQ_DISABLE | FIQ_DISABLE)
 ldr sp, =__irq_stack_top__


 msr CPSR_c, #(SVC_MODE | IRQ_DISABLE | FIQ_DISABLE)
 ldr sp, =__svc_stack_top__


 msr CPSR_c, #(UND_MODE | IRQ_DISABLE | FIQ_DISABLE)
 ldr sp, =__und_stack_top__


 msr CPSR_c, #(ABT_MODE | IRQ_DISABLE | FIQ_DISABLE)
 ldr sp, =__abt_stack_top__


 msr CPSR_c, #(SYS_MODE | IRQ_DISABLE | FIQ_DISABLE)
 ldr sp, =__sys_stack_top__


 bl _rom_data_init+.-base

 msr CPSR_c, #(USR_MODE)

 b main
# 160 "src/start.S"
_undefined_instruction: .word undefined_instruction
_swi_handler: .word swi_handler
_irq_handler: .word irq_handler
_prefetch_abort: .word prefetch_abort
_data_abort: .word data_abort
_not_used: .word not_used
_irq: .word irq
_task_will_wakeup: .word task_will_wakeup
_fiq: .word fiq
 .balignl 16,0xdeadbeef
# 185 "src/start.S"
_TEXT_BASE:
 .word

.globl _armboot_start
_armboot_start:
 .word _start




.globl _bss_start
_bss_start:
 .word __bss_start

.globl _bss_end
_bss_end:
 .word _end

_start_armboot: .word main

#_system_stack:
# . = . + usr_stack_size + irq_stack_size + fiq_stack_size + und_stack_size + abt_stack_size + sup_stack_size
# 217 "src/start.S"
cpu_init_crit:
 # actually do nothing for now!
    mov pc, lr

.align 3
store_task:


    ldr r4, _tasks
    cmp r5, #6
    bhi exit_store

    mov r6, #84
    mla r0, r6, r5, r4

    mov r4, lr
    mov r3, r0
    mov r1, sp
    mov r2, #52
    bl memcpy
    mov lr, r4
    ldr r2, [sp, #52]
    str r2, [r3, #60]
    str r10, [r3, #64]

    mrs r2, CPSR
 msr cpsr_c, #(SYS_MODE | IRQ_DISABLE)
    str r13, [r3, #52]
    str r14, [r3, #56]
 msr cpsr, r2
exit_store:
    bx lr

.align 3
restore_task:
    ldr r4, _tasks
    ldr r5, _curtask
    ldr r5, [r5]
    mov r6, #84
    mov r0, sp
    mla r1, r6, r5, r4
    mov r2, #52
    mov r4, lr
    bl memcpy
    mov lr, r4
    ldr r2, [r1, #60]
    str r2, [sp, #52]
    ldr r2, [r1, #64]

    mrs r3, CPSR
 msr cpsr_c, #(SYS_MODE | IRQ_DISABLE)
    msr spsr, r2
    ldr r13, [r1, #52]
    ldr r14, [r1, #56]
 msr cpsr, r3
    bx lr

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




 .align 5
undefined_instruction:

 .align 5
swi_handler:
    stmfd sp!, {r0-r12, lr}
 msr cpsr_c, #(SVC_MODE | IRQ_DISABLE)

    mrs r10, SPSR
    tst r10, #32
    ldrneh r11, [lr, #-2]
    bicne r11, r11, #0xff00
    ldreq r11, [lr, #-4]
    biceq r11, r11, #0xff000000
    mov r0, #0
    str r0, rescheduled



    ldr r5, _curtask
    ldr r5, [r5]
    bl store_task


    cmp r11, #6
    bhi 1f
    adr r4, switable
    ldr r0, [sp, #0]
    ldr r1, [sp, #4]
    ldr r2, [sp, #8]
    ldr r3, [r4, r11, lsl#2]
    mov lr, pc
    bx r3

1:
    ldr r8, rescheduled
    cmp r8, #0
    streq r0, [sp]
    beq swi_return


    ldr r3, _task_will_wakeup
    mov lr, pc
    bx r3
    bl restore_task

swi_return:
    ldm sp!, {r0-r12, pc}^


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
    push {r1-r3}
2:
    cmp r2, #0
    beq 1f
    ldrb r3, [r1], #1
    strb r3, [r0], #1
    sub r2, #1
    b 2b
1:
    pop {r1-r3}
    bx lr



 .align 5
irq_handler:
    sub lr, lr, #4
    stmfd sp!, {r0-r12, lr}

    mov r0, lr
    cmp r0, #0xf00000
    bls XXXzoo
    ldr r3, _put_hex32
    mov lr, pc
    bx r3
XXXzoo:

    mov r0, #0
    str r0, rescheduled
    ldr r5, _curtask
    ldr r5, [r5]

    ldr r3, _irq
    mov lr, pc
    bx r3

    ldr r8, rescheduled
    cmp r8, #0
    beq irq_return

    mrs r10, SPSR
    bl store_task
    ldr r3, _task_will_wakeup
    mov lr, pc
    bx r3

    bl restore_task

irq_return:
    ldmfd sp!, {r0-r12, pc}^





 .align 5
prefetch_abort:
    ldr r0, msg3
    ldr r3, _putstr
    mov lr, pc
    bx r3
 nop
 .align 5
data_abort:
    ldr r0, msg3
    ldr r3, _putstr
    mov lr, pc
    bx r3

 .align 5
not_used:
    ldr r0, msg3
    ldr r3, _putstr
    mov lr, pc
    bx r3


 .align 5
fiq:
    ldr r0, msg3
    ldr r3, _putstr
    mov lr, pc
    bx r3

 .align 5

.globl reset_cpu
reset_cpu:
 mov pc, r0

.globl lock_nowait
.type lock_nowait, function
lock_nowait:
    push {r1-r2}
    mov r2, #1
    swp r1, r2, [r0]
    cmp r1, r2
    movne r0, #1
    moveq r0, #0
    pop {r1-r2}
    bx lr

.globl lock_wait
.type lock_wait, function
lock_wait:
    push {r1-r2}
lock_loop:
    mov r2, #1
    swp r1, r2, [r0]
    cmp r1, r2
    beq lock_loop

    pop {r1-r2}
    bx lr

.globl lock_unlock
.type lock_unlock, function
lock_unlock:
    push {r1-r2}
    mov r2, #0
    swp r1, r2, [r0]
    pop {r1-r2}
    bx lr

.globl get_cpsr
.type get_cpsr, function
get_cpsr:
    mrs r0, CPSR
    bx lr

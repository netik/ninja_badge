/*
 * Copyright (c) 2010, Mariano Alvira <mar@devl.org> and other contributors
 * to the MC1322x project (http://mc1322x.devl.org)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of libmc1322x: see http://mc1322x.devl.org
 * for details. 
 *
 * $Id$
 */

#ifndef ISR_H
#define ISR_H

#define INTBASE        (0x80020000)
#define INTCNTL_OFF           (0x0)
#define INTENNUM_OFF          (0x8)
#define INTDISNUM_OFF         (0xC)
#define INTENABLE_OFF        (0x10)
#define INTSRC_OFF           (0x30)
#define INTFRC_OFF           (0x34)
#define NIPEND_OFF           (0x38)

#define INTCNTL      ((volatile uint32_t *) (INTBASE + INTCNTL_OFF))
#define INTENNUM     ((volatile uint32_t *) (INTBASE + INTENNUM_OFF))
#define INTDISNUM    ((volatile uint32_t *) (INTBASE + INTDISNUM_OFF))
#define INTENABLE    ((volatile uint32_t *) (INTBASE + INTENABLE_OFF))
#define INTSRC       ((volatile uint32_t *) (INTBASE + INTSRC_OFF))
#define INTFRC       ((volatile uint32_t *) (INTBASE + INTFRC_OFF))
#define NIPEND       ((volatile uint32_t *) (INTBASE + NIPEND_OFF))

enum interrupt_nums {
	INT_NUM_ASM = 0,
	INT_NUM_UART1,
	INT_NUM_UART2,
	INT_NUM_CRM,
	INT_NUM_I2C,
	INT_NUM_TMR,
	INT_NUM_SPIF,
	INT_NUM_MACA,
	INT_NUM_SSI,
	INT_NUM_ADC,
	INT_NUM_SPI,
};

#define global_irq_disable() (set_bit(*INTCNTL,20))
#define global_irq_enable()  (clear_bit(*INTCNTL,20))

#define enable_irq(irq)  (*INTENNUM  = INT_NUM_##irq)
#define disable_irq(irq) (*INTDISNUM = INT_NUM_##irq)

#define safe_irq_disable(x)  volatile uint32_t saved_irq; saved_irq = *INTENABLE; disable_irq(x)
#define irq_restore() *INTENABLE = saved_irq

extern void tmr2_isr(void) __attribute__((weak));

extern void rtc_isr(void) __attribute__((weak));
extern void button_isr(void) __attribute__((weak));

extern void uart1_isr(void) __attribute__((weak));

extern void maca_isr(void) __attribute__((weak));


#endif


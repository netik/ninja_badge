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
#define PAYLOAD_LEN 16

#include <mc1322x.h>
#include <board.h>
#include <stdio.h>
#include <pinio.h>

#include "tests.h"
#include "config.h"

#define LED LED_GREEN

void fill_packet(volatile packet_t *p) {
	static volatile uint8_t count=0;
	volatile uint8_t i;
	p->length = PAYLOAD_LEN;
	p->offset = 0;
    p->data[0] = 0x0;
    p->data[1] = 0x7a;
    p->data[2] = 0x69;
	for(i=3; i<PAYLOAD_LEN; i++) {
		p->data[i] = count++;
	}

	/* acks get treated differently, even in promiscuous mode */
	/* setting the third bit makes sure that we never send an ack */
        /* or any valid 802.15.4-2006 packet */
	p->data[0] |= (1 << 3); 
}

void maca_rx_callback(volatile packet_t *p) {
	(void)p;
	//gpio_data_set(1ULL<< LED);
	//gpio_data_reset(1ULL<< LED);
}

void main(void) {
	volatile packet_t *p;

	gpio_data(0);
	
	gpio_pad_dir_set( 1ULL << LED );
        /* read from the data register instead of the pad */
	/* this is needed because the led clamps the voltage low */
	gpio_data_sel( 1ULL << LED);

	/* trim the reference osc. to 24MHz */
	trim_xtal();

	uart_init(INC, MOD, SAMP);

	vreg_init();

	maca_init();

        /* sets up tx_on, should be a board specific item */
	//       *GPIO_FUNC_SEL2 = (0x01 << ((44-16*2)*2));
	gpio_pad_dir_set( 1ULL << 44 );

	set_power(0x0f); /* 0dbm */
	set_channel(0); /* channel 11 */

	print_welcome("econrf");
    pinFunc(LED_RED, 3);
    pinDirection(LED_RED, PIN_OUTPUT);
    pinFunc(LED_GREEN, 3);
    pinDirection(LED_GREEN, PIN_OUTPUT);
    setPin(LED_RED, 0);
    setPin(LED_GREEN, 0);


	while(1) {		

		/* call check_maca() periodically --- this works around */
		/* a few lockup conditions */
        for(int z = 0; z < 0x20000; z++) {
            check_maca();
            if((p = rx_packet())) {
                int isvalid = 0;
                print_packet(p);
                if(p->data[p->offset + 1] == 0x6 && p->data[p->offset + 2] == 0x17) {
                    isvalid = 1;
                }
                free_packet(p);
                if(isvalid) {
                    setPin(LED_RED, 1);
                    for(volatile int i = 0; i < 10000; i++) { continue; }
                    setPin(LED_RED, 0);
                }
            }
        }
        p = get_free_packet();
        if(p) {
            check_maca();
            fill_packet(p);
            tx_packet(p);
            setPin(LED_GREEN, 1);
            for(volatile int i = 0; i < 10000; i++) { continue; }
            setPin(LED_GREEN, 0);
        }
	}
}

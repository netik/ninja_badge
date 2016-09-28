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

#include <mc1322x.h>
#include <board.h>

const uint8_t hex[16]={'0','1','2','3','4','5','6','7',
		 '8','9','A','B','C','D','E','F'};

void putchr(char c) {
	while(*UART1_UTXCON == 31); 
        /* wait for there to be room in the buffer */
// 	while( *UART1_UTXCON == 0 ) { continue; }
	*UART1_UDATA = c;
}
	
void putstr(char *s) {
    volatile char *origs = s;
	while(origs && *origs!=0) {
		putchr(*origs++);
	}
}

void put_hex(uint8_t x)
{
        putchr(hex[x >> 4]);
        putchr(hex[x & 15]);
}

void put_hex16(uint16_t x)
{
        put_hex((x >> 8) & 0xFF);
        put_hex((x) & 0xFF);
}

void put_hex32(uint32_t x)
{
        put_hex((x >> 24) & 0xFF);
        put_hex((x >> 16) & 0xFF);
        put_hex((x >> 8) & 0xFF);
        put_hex((x) & 0xFF);
}
void format_hex32(uint32_t x, char *buf) {
    buf[0] = hex[(x >> 28) & 0xf];
    buf[1] = hex[(x >> 24) & 0xf];
    buf[2] = hex[(x >> 20) & 0xf];
    buf[3] = hex[(x >> 16) & 0xf];
    buf[4] = hex[(x >> 12) & 0xf];
    buf[5] = hex[(x >> 8) & 0xf];
    buf[6] = hex[(x >> 4) & 0xf];
    buf[7] = hex[x & 0xf];
}

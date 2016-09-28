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
#include <pinio.h>
#include "tests.h"
#include "config.h"

void flushrx(void);
void supercheck(void);
void realswi(void);
uint32_t to_u32(volatile uint32_t *c);
uint8_t from_hex(uint8_t ch);

#define dbg_putchr(...) putchr(__VA_ARGS__)
#define dbg_putstr(...) putstr(__VA_ARGS__)
#define dbg_put_hex(...) put_hex(__VA_ARGS__)
#define dbg_put_hex16(...) put_hex16(__VA_ARGS__)
#define dbg_put_hex32(...) put_hex32(__VA_ARGS__)

#define DELAY 800000

#ifndef MIN
#define MIN(x, y) (((x) > (y)) ? (y) : (x))
#endif

#define SRC_START_ADDR ((volatile uint32_t *) ( 0x408000 ))
#define DEST_START_ADDR ((volatile uint32_t *) ( 0x408004 ))
#define FLASHLEN_ADDR ((volatile uint32_t *) ( 0x408008 ))
#define CRC_ADDR ((volatile uint32_t *) ( 0x40800c ))
#define OUT_CRC_ADDR ((volatile uint32_t *) ( 0x408010 ))
#define OUT_STATUS_ADDR ((volatile uint32_t *) ( 0x408014 ))

uint8_t getc(void) {
	volatile uint8_t c;
	while(*UART1_URXCON == 0);
	
	c = *UART1_UDATA;
	return c;
}
void callswi(void) {
    putstr("calling swi\n");
    __asm__("swi #0x42");
    putstr("done calling swi\r\n");
    supercheck();
}
enum parse_states {
	SCAN_X,
	READ_CHARS,
	PROCESS,
	MAX_STATE,
};

void main(void) {	
	nvmErr_t err;
	nvmType_t type=0;
	volatile uint8_t c;
	volatile uint32_t buf[4], i=0;
	volatile uint32_t state = SCAN_X;
	volatile uint32_t len=0;
    volatile uint32_t src_addr, dest_addr;
    volatile uint32_t crc;
	volatile uint32_t addr,data;

    uart1_init(INC,MOD,SAMP);
	disable_irq(UART1);
	vreg_init();

	dbg_putstr("Detecting internal nvm\r\n");
	err = nvm_detect(gNvmInternalInterface_c, &type);
	dbg_putstr("nvm_detect returned: 0x");
	dbg_put_hex(err);
	dbg_putstr(" type is: 0x");
	dbg_put_hex32(type);
	dbg_putstr("\r\n");

    //supercheck();
	
    pinFunc(LED_RED, 3);
    pinDirection(LED_RED, PIN_OUTPUT);
    pinFunc(LED_GREEN, 3);
    pinDirection(LED_GREEN, PIN_OUTPUT);
    setPin(LED_RED, 1);
    setPin(LED_GREEN, 0);
	//flushrx();

    len = *(FLASHLEN_ADDR);
    src_addr = *(SRC_START_ADDR);
    dest_addr = *(DEST_START_ADDR);
    crc = *(CRC_ADDR);
	dbg_putstr("len  ");
	dbg_put_hex32(len);
    dbg_putstr("  src ");
    dbg_put_hex32(src_addr);
    dbg_putstr("  dest ");
    dbg_put_hex32(dest_addr);
    dbg_putstr("  crc ");
    dbg_put_hex32(crc);
	dbg_putstr("\r\n");

    uint32_t erasemask = 0, xpos = 0;
    for(xpos = dest_addr; xpos < (dest_addr + len); xpos += (0x1000 - (xpos & 0xfff))) {
        erasemask |= (1 << (xpos/0x1000));
    }

    dbg_putstr("sector erase mask: ");
    dbg_put_hex32(erasemask);
    dbg_putstr("\r\n");

	err = nvm_erase(gNvmInternalInterface_c, type, erasemask); 
	dbg_putstr("nvm_erase returned: 0x");
	dbg_put_hex(err);
    dbg_putstr("\r\n");

    dbg_putstr("writing\r\n");
    uint8_t *ptr = (uint8_t *)src_addr;
	for(i=0; i<len; i+=8) {
		err = nvm_write(gNvmInternalInterface_c, type, &ptr[i], dest_addr+i, MIN(8, (len-i))); 
        if(err != gNvmErrNoError_c) {
            dbg_putstr("write err at ");
            dbg_put_hex32(i);
            dbg_putstr("  err: ");
            dbg_put_hex(err);
            dbg_putstr("\r\n");
        }
        if((i % 256) == 0) {
            setPin(LED_RED, ((i % 512) == 0) ? 1 : 0);
        }
	}
    dbg_putstr("writing done\r\n");
    err = nvm_verify(gNvmInternalInterface_c, type, ptr, dest_addr, len);
    if(err == gNvmErrNoError_c) {
        dbg_putstr("verified\r\n");
        setPin(LED_RED, 0);
        setPin(LED_GREEN, 1);
    } else {
        dbg_putstr("nvm_verify returned: 0x");
        dbg_put_hex(err);
        dbg_putstr("\r\n");
        while(1) {
            setPin(LED_RED, 0);
            for(i=0; i<DELAY; i++) { continue; }
            setPin(LED_RED, 1);
            for(i=0; i<DELAY; i++) { continue; }
        }
    }


    while(1) { continue; }
	int j = 0;

	while(1) {
        setPin(LED_RED, 1);
        putstr("blinkz\r\n");
		
		for(i=0; i<DELAY; i++) { continue; }

        setPin(LED_RED, 0);
		
		for(i=0; i<DELAY; i++) { continue; }
        char *x = (char *)0x0;

        *x = 'f';
        putstr(x);
        j++;
        if(j == 5) {
            callswi();
            realswi();
            j = 0;
        }
	}

	/* write the OKOK magic */

#if BOOT_OK
	((uint8_t *)buf)[0] = 'O'; ((uint8_t *)buf)[1] = 'K'; ((uint8_t *)buf)[2] = 'O'; ((uint8_t *)buf)[3] = 'K';	
#elif BOOT_SECURE
	((uint8_t *)buf)[0] = 'S'; ((uint8_t *)buf)[1] = 'E'; ((uint8_t *)buf)[2] = 'C'; ((uint8_t *)buf)[3] = 'U';	
#else
	((uint8_t *)buf)[0] = 'N'; ((uint8_t *)buf)[1] = 'O'; ((uint8_t *)buf)[2] = 'N'; ((uint8_t *)buf)[3] = 'O';
#endif

	dbg_putstr(" type is: 0x");
	dbg_put_hex32(type);
	dbg_putstr("\r\n");

	err = nvm_write(gNvmInternalInterface_c, type, (uint8_t *)buf, 0, 4);

	dbg_putstr("nvm_write returned: 0x");
	dbg_put_hex(err);
	dbg_putstr("\n");

	/* write the length */
	err = nvm_write(gNvmInternalInterface_c, type, (uint8_t *)&len, 4, 4);

	/* read a byte, write a byte */
	/* byte at a time will make this work as a contiki process better */
	/* for OTAP */
	for(i=0; i<len; i++) {
		c = getc();	       
		err = nvm_write(gNvmInternalInterface_c, type, (uint8_t *)&c, 8+i, 1); 
	}

	putstr("flasher done\n");

	state = SCAN_X; addr=0;
	while((c=getc())) {
		if(state == SCAN_X) {
			/* read until we see an 'x' */
			if(c==0) { break; }
			if(c!='x'){ continue; } 	
			/* go to read_chars once we have an 'x' */
			state = READ_CHARS;
			i = 0; 
		}
		if(state == READ_CHARS) {
			/* read all the chars up to a ',' */
			((uint8_t *)buf)[i++] = c;
			/* after reading a ',' */
			/* goto PROCESS state */
			if((c == ',') || (c == 0)) { state = PROCESS; }				
		}
		if(state == PROCESS) {
			if(addr==0) {
				/*interpret the string as the starting address */
				addr = to_u32(buf);				
			} else {
				/* string is data to write */
				data = to_u32(buf);
				putstr("writing addr ");
				put_hex32(addr);
				putstr(" data ");
				put_hex32(data);
				putstr("\n");
				err = nvm_write(gNvmInternalInterface_c, 1, (uint8_t *)&data, addr, 4);
				addr += 4;
			}
			/* look for the next 'x' */
			state=SCAN_X;
		}
	}

	while(1) {continue;};
}
void flushrx(void)
{
	volatile uint8_t c;
	while(*UART1_URXCON !=0) {
		c = *UART1_UDATA;
	}
}
uint32_t to_u32(volatile uint32_t *c) 
{
	volatile uint32_t ret=0;
	volatile uint32_t i,val;
	
	/* c should be /x\d+,/ */
	i=1; /* skip x */
	while(((uint8_t *)c)[i] != ',') {
		ret = ret<<4;
		val = from_hex(((uint8_t *)c)[i++]);
		ret += val;
	}
	return ret;
}
uint8_t from_hex(uint8_t ch)
{
        if(ch==' ' || ch=='\r' || ch=='\n')
                return 16;

        if(ch < '0')
                goto bad;
        if(ch <= '9')
                return ch - '0';
        ch |= 0x20;
        if(ch < 'a')
                goto bad;
        if(ch <= 'f')
                return ch - 'a' + 10;
bad:
        return 32;
}



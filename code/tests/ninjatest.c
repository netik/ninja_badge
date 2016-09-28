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
#include <stdio.h>
#include <pinio.h>

#include "smallfont.h"
#include "tests.h"
#include "config.h"
#include "shuriken.h" 

#ifdef BROKEN_BOARD
#define SER1P 21        // XXX MODIFIED; normally 1
#define SHIFT_OUT 20    // XXX MODIFIED; normally 0
#define LED_PWM 9       // XXX differs
#else
#define SER1P 1        // XXX MODIFIED; normally 1
#define SHIFT_OUT 0    // XXX MODIFIED; normally 0
#define LED_PWM 9       // XXX differs
#endif /* BROKEN_BOARD */
#define SCK1P 3
#define RCK1P 2

#define BUZZER_PIN 11   

#define SPIFL_CMD_READID 0x90   // read-id
#define SPI_SCK 7      // SPI serial clock
#define SPI_MOSI 6     // SPI master out, slave in
#define SPI_MISO 5     // SPI master in, slave out
#define FLASH_E 4      // flash enable
#define FLASH_HOLD 6   // flash hold mode (NOT USED; tied high)

#define JTAG_RTCK 37
#define JTAG_TDO 49
#define JTAG_TDI 48
#define JTAG_TCK 47
#define JTAG_TMS 46

#define BUTTON_UP 22
#define BUTTON_DOWN 23
#define BUTTON_RIGHT 24
#define BUTTON_LEFT 25
#define BUTTON_A 26
#define BUTTON_B 27

#define BLA 8     // backlight anode, for PWMing     (XXX: differs)
#define SID 36     // aka DB7
#define SCLK 35      // serial clock; aka DB6
#define A0 34      // aka RS in some datasheets; H for display, L for control
#define RST 33     // aka RESETB
#define CS 32 

#define LOW 0
#define HIGH 1

#define CMD_DISPLAY_OFF 0xAE
#define CMD_DISPLAY_ON 0xAF
#define CMD_SET_DISP_START_LINE 0x40
#define CMD_SET_PAGE 0xB0
#define CMD_SET_COLUMN_UPPER 0x10
#define CMD_SET_COLUMN_LOWER 0x00
#define CMD_SET_ADC_NORMAL 0xA0
#define CMD_SET_ADC_REVERSE 0xA1
#define CMD_SET_DISP_NORMAL 0xA6
#define CMD_SET_DISP_REVERSE 0xA7
#define CMD_SET_ALLPTS_NORMAL 0xA4
#define CMD_SET_ALLPTS_ON 0xA5
#define CMD_SET_BIAS_9 0xA2
#define CMD_SET_BIAS_7 0xA3
#define CMD_RMW 0xE0
#define CMD_RMW_CLEAR 0xEE
#define CMD_INTERNAL_RESET 0xE2
#define CMD_SET_COM_NORMAL 0xC0
#define CMD_SET_COM_REVERSE 0xC8
#define CMD_SET_POWER_CONTROL 0x28
#define CMD_SET_RESISTOR_RATIO 0x20
#define CMD_SET_VOLUME_FIRST 0x81
#define CMD_SET_VOLUME_SECOND 0
#define CMD_SET_STATIC_OFF 0xAC
#define CMD_SET_STATIC_ON 0xAD
#define CMD_SET_STATIC_REG 0x0
#define CMD_SET_BOOSTER_FIRST 0xF8
#define CMD_SET_BOOSTER_234 0
#define CMD_SET_BOOSTER_5 1
#define CMD_SET_BOOSTER_6 3
#define CMD_NOP 0xE3
#define CMD_TEST 0xF0

uint8_t spi_op(uint8_t outb);
void st7565_init(void);
void st7565_command(uint8_t c);
void st7565_data(uint8_t c);
void st7565_set_brightness(uint8_t val);
void write_bitmap(unsigned char *bitmap);
void clear_bitmap(void);
void text_write_small(unsigned int x, unsigned int y, char *str);
void flash_init(void);
void flash_secterase(int addr);
void flash_read(int addr, int len, uint8_t *buf);
void flash_bytewrite(int addr, uint8_t val);
void flash_wren(void);
void flash_wrsr(uint8_t sr);
uint8_t flash_rdsr(void);
int flash_checkid(void);
int flash_checkwrite(void);
void hexstr(char *outbuf, unsigned char *src, unsigned int srclen);

void flash_init(void) {
    pinFunc(SPI_SCK, 3);
    pinFunc(SPI_MOSI, 3);
    pinFunc(SPI_MISO, 3);
    pinFunc(FLASH_E, 3);

    pinDirection(SPI_SCK, PIN_OUTPUT);
    pinDirection(SPI_MOSI, PIN_OUTPUT);
    pinDirection(SPI_MISO, PIN_INPUT);
    pinDirection(FLASH_E, PIN_OUTPUT);
}
#define CLICKS 600    // counter val at 24MHz/8
volatile unsigned int clockus;
void tmr2_isr(void) {
    clockus += (1000000/(24000000/(8*CLICKS)));
    *TMR2_SCTRL = 0;
    *TMR2_CSCTRL = 0x0040; /* clear compare flag */

}
void timer_init(void) {
    clockus = 0;
    *TMR_ENBL &= ~0x0;
    *TMR2_SCTRL = 0;
    *TMR2_CSCTRL = 0x40;
    *TMR2_LOAD = 0;
    *TMR2_COMP1 = CLICKS;
    *TMR2_CMPLD1 = CLICKS;
    *TMR2_CNTR = 0;
    // set primary count source to peri_clk/8 in TMR2_CTRL
    *TMR2_CTRL = 0x20 | (0xb << 9) | (1 << 13);
    *TMR_ENBL = 0xf;
    enable_irq(TMR);
}

void delaycs(uint32_t centis) {
    unsigned int j = clockus + (centis * 10000);
    if(j < clockus) {
        while(j < clockus) {
            continue;
        }
    }
    while(clockus < j) {
        continue;
    }
}

void shift1(uint8_t c) {
    int i;
    setPin(SER1P, 0);
    setPin(SCK1P, 0);
    setPin(RCK1P, 0);
    for(i = 7; i >= 0; i--) {
        setPin(SCK1P, 0);
        setPin(RCK1P, 0);
        if(((c >> i) & 1) == 1){
            setPin(SER1P, 1);
        } else {
            setPin(SER1P, 0);
        }
        setPin(SCK1P, 1);
        setPin(RCK1P, 1);
        setPin(SER1P, 0);
    }
}



void maca_rx_callback(volatile packet_t *p) {
	(void)p;
	//gpio_data_set(1ULL<< LED);
	//gpio_data_reset(1ULL<< LED);
}


#define LED LED_RED

/* 802.15.4 PSDU is 127 MAX */
/* 2 bytes are the FCS */
/* therefore 125 is the max payload length */
#define PAYLOAD_LEN 16
#define DELAY 100000

void fill_packet(volatile packet_t *p) {
	static volatile uint8_t count=0;
	volatile uint8_t i;
	p->length = PAYLOAD_LEN;
	p->offset = 0;
    p->data[0] = 0x0;
    p->data[1] = 0x6;
    p->data[2] = 0x17;
	for(i=3; i<PAYLOAD_LEN; i++) {
		p->data[i] = count++;
	}

	/* acks get treated differently, even in promiscuous mode */
	/* setting the third bit makes sure that we never send an ack */
        /* or any valid 802.15.4-2006 packet */
	p->data[0] |= (1 << 3); 
}
void inputs(void) {
    pinDirection(SHIFT_OUT, PIN_INPUT);
    pinDirection(BUTTON_DOWN, PIN_INPUT);
    pinDirection(BUTTON_LEFT, PIN_INPUT);
    pinDirection(BUTTON_RIGHT, PIN_INPUT);
    pinDirection(BUTTON_B, PIN_INPUT);
    pinDirection(BUTTON_A, PIN_INPUT);
    pinDirection(BUTTON_UP, PIN_INPUT);
}


void mode0(void);
void mode1(void);
void mode2(void);
void mode3(void);

void mode0(void) {
    while(1) {
        clear_bitmap();
        inputs();
        text_write_small(36, 0, "TEST MODE 0");
        text_write_small(26, 8, "RIGHT FOR MODE1");
        if(flash_checkid()) {
            text_write_small(33, 16, "FLASH ID: OK");
        } else {
            text_write_small(33, 16, "FLASH ID: BAD <---");
        }
        if(flash_checkwrite()) {
            text_write_small(33, 24, "FLASH WR: OK");
        } else {
            text_write_small(33, 24, "FLASH WR: BAD <---");
        }
        shift1(0xff);
        shift1(0xff);
        pinFunc(BUZZER_PIN, 1);
        *TMR_ENBL |= (1 << 3);
        *TMR3_LOAD = 0;
        *TMR3_CNTR = 0;
        *TMR3_SCTRL = ((1 << 2) | 1);
        *TMR3_CSCTRL = 0x6;     // XXX: ??
        *TMR3_COMP1 = 8571;
        *TMR3_CMPLD1 = 8571;
        *TMR3_COMP2 = 15389;
        *TMR3_CMPLD2 = 15389;
        *TMR3_CTRL = (0x2024 | (0xb << 9));

        delaycs(20);
        *TMR3_CMPLD1 = 875;
        *TMR3_CMPLD2 = 1125;
        delaycs(30);
        *TMR_ENBL &= ~(1 << 3);

        while(1) {
            int i;
            for(i = 0; i < 5; i++) {
                if(getPin(BUTTON_RIGHT) == 0) {
                    mode1();
                    i = 9;
                    break;
                }
                delaycs(10);
            }
            if(i == 9) {
                break;
            }
            shift1(0x55);
            shift1(0x55);
            setPin(LED_RED, 0);
            setPin(LED_GREEN, 1);
            delaycs(5);
            for(i = 0; i < 5; i++) {
                if(getPin(BUTTON_RIGHT) == 0) {
                    mode1();
                    i = 9;
                    break;
                }
                delaycs(10);
            }
            if(i == 9) {
                break;
            }
            shift1(0xaa);
            shift1(0xaa);
            setPin(LED_RED, 1);
            setPin(LED_GREEN, 0);
        }
    }
}
#define LED_00 (0x8000)
#define LED_01 (0x4000)
#define LED_02 (0x80)
#define LED_03 (0x40)
#define LED_04 (0x20)
#define LED_05 (0x10)


void mode1(void) {
    while(1) {
        inputs();
        shift1(0x00);
        shift1(0x00);
        setPin(LED_RED, 0);
        setPin(LED_GREEN, 0);
        clear_bitmap();
        text_write_small(14, 0, "TEST MODE 1: BUTTONS");
        text_write_small(31, 8, "A+B FOR MODE2");
        text_write_small(31, 16, "U+D FOR MODE1");
        delaycs(5);
        while(1) {
            int curval = 0;
            if(getPin(BUTTON_UP) == 0) {
                curval |= LED_00;
            }
            if(getPin(BUTTON_DOWN) == 0) {
                curval |= LED_01;
            }
            if(getPin(BUTTON_LEFT) == 0) {
                curval |= LED_02;
            }
            if(getPin(BUTTON_RIGHT) == 0) {
                curval |= LED_03;
            }
            if(getPin(BUTTON_B) == 0) {
                curval |= LED_04;
            }
            if(getPin(BUTTON_A) == 0) {
                curval |= LED_05;
            }
            shift1(curval >> 8);
            shift1(curval & 0xff);
            delaycs(3);
            if(getPin(BUTTON_A) == 0 && getPin(BUTTON_B) == 0) {
                mode2();
                break;
            }
            if(getPin(BUTTON_UP) == 0 && getPin(BUTTON_DOWN) == 0) {
                return;
            }
        }
    }
}

void mode2(void) {
    while(1) {
        inputs();
        shift1(0x00);
        shift1(0x00);
        setPin(LED_RED, 0);
        setPin(LED_GREEN, 0);
        clear_bitmap();
        text_write_small(14, 0, "TEST MODE 2: STROBE");
        text_write_small(26, 8, "LEFT FOR MODE 1");
        text_write_small(23, 16, "RIGHT FOR MODE 1");
        delaycs(15);

        unsigned long val0 = 0, val1 = 0;
        for(int i = 0; i < 64; i++) {
            if(i != JTAG_TMS
                    && i != JTAG_TCK
                    && i != JTAG_TDI
                    && i != JTAG_TDO
                    && i != JTAG_RTCK
                    && i != SER1P
                    && i != SCK1P
                    && i != RCK1P
                    && i != SID
                    && i != SCLK
                    && i != SPI_SCK
                    && i != SPI_MOSI
                    && i != SPI_MISO
                    && i != FLASH_E
                    && i != A0  
                    && i != RST 
                    && i != CS  
                    && i != BUTTON_LEFT
                    && i != BUTTON_RIGHT) {
                pinFunc(i, 3);
                pinDirection(i, PIN_OUTPUT);
                if(i > 31) {
                    val1 |= (1 << (i-32));
                } else {
                    val0 |= (1 << i);
                }
            }
        }

        while(1) {
            int i;
            for(i = 0; i < 10; i++) {
                if(getPin(BUTTON_RIGHT) == 0) {
                    inputs();
                    setPin(BLA, 1);
                    mode3();
                    i = 9;
                    break;
                }
                if(getPin(BUTTON_LEFT) == 0) {
                    inputs();
                    setPin(BLA, 1);
                    return;
                }
                delaycs(10);
            }
            if(i == 9) {
                break;
            }

            *GPIO_DATA_SET0 = val0;
            *GPIO_DATA_SET1 = val1;
            if(getPin(BUTTON_LEFT) == 0) {
                return;
            }
            for(i = 0; i < 10; i++) {
                if(getPin(BUTTON_RIGHT) == 0) {
                    inputs();
                    setPin(BLA, 1);
                    mode3();
                    i = 9;
                    break;
                }
                if(getPin(BUTTON_LEFT) == 0) {
                    inputs();
                    setPin(BLA, 1);
                    return;
                }
                delaycs(10);
            }
            if(i == 9) {
                break;
            }
            *GPIO_DATA_RESET0 = val0;
            *GPIO_DATA_RESET1 = val1;
        }
    }
}

void mode3(void) {
	while(1) {		
        maca_init();
        set_channel(0); /* channel 11 */
    //	set_power(0x0f); /* 0xf = -1dbm, see 3-22 */
        set_power(0x11); /* 0x11 = 3dbm, see 3-22 */
    //	set_power(0x12); /* 0x12 is the highest, not documented */

        // XXX XXX
        /* sets up tx_on, should be a board specific item */
    //    *GPIO_FUNC_SEL2 = (0x01 << ((44-16*2)*2));
    //	gpio_pad_dir_set( 1ULL << 44 );

        inputs();
        int messagechanged = 0;
        unsigned int rxpackets = 0;
        shift1(0x00);
        shift1(0x00);
        setPin(LED_RED, 0);
        setPin(LED_GREEN, 0);
        clear_bitmap();
        text_write_small(16, 0, "TEST MODE 3: RX-TX");
        text_write_small(26, 8, "LEFT FOR MODE 2");
        text_write_small(28, 24, "WAITING FOR RX");
        delaycs(50);

        while(1) {
            volatile packet_t *p;

            check_maca();
            p = get_free_packet();
            if(p) {
                fill_packet(p);

                tx_packet(p);
                
                setPin(LED_RED, 1);
                for(volatile int i = 0; i < 10000; i++) { continue; }
                setPin(LED_RED, 0);
            }
            for(int i = 0; i < 50; i++) {
//                if(getPin(BUTTON_RIGHT) == 0) {
//                    inputs();
//                    setPin(BLA, 1);
//                    mode3();
//                    i = 9;
//                    break;
//                }
                check_maca();
                while((p = rx_packet())) {
                    int isvalid = 0;
                    check_maca();
                    if(p->data[p->offset + 1] == 0x7a && p->data[p->offset + 2] == 0x69) {
                        rxpackets++;
                        isvalid = 1;
                    }
                    free_packet(p);
                    if(isvalid) {
                        setPin(LED_GREEN, 1);
                        for(volatile int i = 0; i < 10000; i++) { check_maca(); }
                        setPin(LED_GREEN, 0);
                    }
                }
                if(getPin(BUTTON_LEFT) == 0) {
                    maca_off();
                    inputs();
                    return;
                }
                delaycs(3);
            }
            if(messagechanged == 0 && rxpackets >= 3) {
                text_write_small(28, 32, "    RX OK     ");
                messagechanged = 1;
            }
        }
	}

}



int mode;
void main(void) {
    pinFunc(LED_RED, 3);
    pinFunc(LED_GREEN, 3);
    pinDirection(LED_RED, PIN_OUTPUT);
    pinDirection(LED_GREEN, PIN_OUTPUT);
    setPin(LED_RED, 1);

	/* trim the reference osc. to 24MHz */
	trim_xtal();
	uart_init(INC, MOD, SAMP);
	vreg_init();
    timer_init();
    setPin(LED_GREEN, 1);

    flash_init();

    inputs();
    pinFunc(SHIFT_OUT, 3);
    pinFunc(SER1P, 3);
    pinFunc(SCK1P, 3);
    pinFunc(RCK1P, 3);
    pinFunc(LED_PWM, 3);
    pinFunc(BLA, 3);
    pinDirection(BUZZER_PIN, PIN_OUTPUT);
    pinDirection(SER1P, PIN_OUTPUT);
    pinDirection(SCK1P, PIN_OUTPUT);
    pinDirection(RCK1P, PIN_OUTPUT);
    pinDirection(LED_PWM, PIN_OUTPUT);
    pinDirection(BLA, PIN_OUTPUT);
    pinFunc(SID, 3);
    pinFunc(SCLK, 3);
    pinFunc(A0, 3);
    pinFunc(RST, 3);
    pinFunc(CS, 3);
    pinDirection(SID, PIN_OUTPUT);
    pinDirection(SCLK, PIN_OUTPUT);
    pinDirection(A0, PIN_OUTPUT);
    pinDirection(RST, PIN_OUTPUT);
    pinDirection(CS, PIN_OUTPUT);
    pinFunc(BUTTON_UP, 3);
    pinFunc(BUTTON_DOWN, 3);
    pinFunc(BUTTON_LEFT, 3);
    pinFunc(BUTTON_RIGHT, 3);
    pinFunc(BUTTON_B, 3);
    pinFunc(BUTTON_A, 3);
    pinPullupEnable(BUTTON_UP, 1);
    pinPullupEnable(BUTTON_DOWN, 1);
    pinPullupEnable(BUTTON_LEFT, 1);
    pinPullupEnable(BUTTON_RIGHT, 1);
    pinPullupEnable(BUTTON_B, 1);
    pinPullupEnable(BUTTON_A, 1);
    pinPullupSelect(BUTTON_UP, PIN_PULLUP);
    pinPullupSelect(BUTTON_DOWN, PIN_PULLUP);
    pinPullupSelect(BUTTON_LEFT, PIN_PULLUP);
    pinPullupSelect(BUTTON_RIGHT, PIN_PULLUP);
    pinPullupSelect(BUTTON_B, PIN_PULLUP);
    pinPullupSelect(BUTTON_A, PIN_PULLUP);

    shift1(0x00);
    shift1(0x00);
    setPin(LED_PWM, 1);
    setPin(LED_RED, 0);
    setPin(LED_GREEN, 0);

    setPin(BLA, 1);
    st7565_init();
    st7565_command(CMD_DISPLAY_ON);
    st7565_command(CMD_SET_ALLPTS_NORMAL);
    st7565_set_brightness(0x18);
    clear_bitmap();

    mode0();
}
void clear_bitmap(void) {
    uint8_t p, c;
    for(p = 0; p < 8; p++) {
        st7565_command(CMD_SET_PAGE | p);
        for(c = 0; c < 129; c++) {
            st7565_command(CMD_SET_COLUMN_LOWER | (c & 0xf));
            st7565_command(CMD_SET_COLUMN_UPPER | ((c >> 4) & 0xf));
            st7565_data(0x0);
        }

    }
}

void st7565_init(void) {
    // toggle RST low to reset; CS low so it'll listen to us
    setPin(CS, LOW);
    setPin(RST, LOW);
    delaycs(50);
    setPin(RST, HIGH);
    // ADC select
    st7565_command(CMD_SET_ADC_NORMAL);
    // SHL select
    st7565_command(CMD_SET_COM_NORMAL);
    // LCD bias select
    st7565_command(CMD_SET_BIAS_7);

    delaycs(2);
    setPin(RST, HIGH);
    // turn on voltage converter (VC=1, VR=0, VF=0)
    st7565_command(CMD_SET_POWER_CONTROL | 0x4);
    // wait for 50% rising
    delaycs(2);
    // turn on voltage regulator (VC=1, VR=1, VF=0)
    st7565_command(CMD_SET_POWER_CONTROL | 0x6);
    // wait >=1ms
    delaycs(2);
    // turn on voltage follower (VC=1, VR=1, VF=1)
    st7565_command(CMD_SET_POWER_CONTROL | 0x7);
    // set lcd operating voltage (regulator resistor, ref voltage resistor)
    st7565_command(CMD_SET_RESISTOR_RATIO);
    // wait (10ms)
    delaycs(2);

    // initial display line
    // set page address
    // set column address
    // write display data
}
void st7565_command(uint8_t c) {
    setPin(A0, LOW);
    shiftMsb(SID, SCLK, c);
}
void st7565_data(uint8_t c) {
    setPin(A0, HIGH);
    shiftMsb(SID, SCLK, c);
}
void st7565_set_brightness(uint8_t val) {
    st7565_command(CMD_SET_VOLUME_FIRST);
    st7565_command(CMD_SET_VOLUME_SECOND | (val & 0x3f));
}

int pagemap[] = { 3, 2, 1, 0, 7, 6, 5, 4 };

#define SMALLFONT_WIDTH 5
void text_write_small(unsigned int x, unsigned int y, char *str) {
    const int page = y/8, shift = y%8;
    x++;
    st7565_command(CMD_SET_PAGE | pagemap[page]);
    st7565_command(CMD_SET_COLUMN_LOWER | (x & 0xf));
    st7565_command(CMD_SET_COLUMN_UPPER | ((x >> 4) & 0xf));
    //st7565_command(CMD_RMW);
    //st7565_command(CMD_RMW);
    //st7565_data(0xff);
    for(int i = 0; str[i] != 0; i++) {
        unsigned char c = str[i];
        if(c > 0x20 && c < 0x5b) {
            for(int j = 0; j < SMALLFONT_WIDTH; j++) {
                st7565_data(smallfont_bmp[(c-0x21)*SMALLFONT_WIDTH+j] >> shift);
            }
        } else {
            for(int j = 0; j < SMALLFONT_WIDTH; j++) {
                st7565_data(0);
            }
        }
    }
    /* If we've bled over to another page, then write whatever's left. */
    const int endpage = (y+SMALLFONT_WIDTH-1) / 8;
    if(endpage != page) {
        st7565_command(CMD_SET_PAGE | pagemap[endpage]);
        st7565_command(CMD_SET_COLUMN_LOWER | (x & 0xf));
        st7565_command(CMD_SET_COLUMN_UPPER | ((x >> 4) & 0xf));
        for(int i = 0; str[i] != 0; i++) {
            unsigned char c = str[i];
            if(c > 0x20 && c < 0x5b) {
                for(int j = 0; j < SMALLFONT_WIDTH; j++) {
                    st7565_data(smallfont_bmp[(c-0x21)*SMALLFONT_WIDTH+j] << (8-shift));
                }
            } else {
                for(int j = 0; j < SMALLFONT_WIDTH; j++) {
                    st7565_data(0);
                }
            }
        }
    }
}

void write_bitmap(unsigned char *bitmap) {
    uint8_t c, p;
    for(p = 0; p < 8; p++) {
        st7565_command(CMD_SET_PAGE | pagemap[p]);
        st7565_command(CMD_SET_COLUMN_LOWER | (0x0 & 0xf));
        st7565_command(CMD_SET_COLUMN_UPPER | ((0x0 >> 4) & 0xf));
        //st7565_command(CMD_RMW);
        //st7565_data(0xff);

        //st7565_data(0x80);
        //continue;

        for(c = 0; c < 128; c++) {
            st7565_data(bitmap[2+(128*p)+c]);
        }
    }
}

const unsigned char hexchars[16]={'0','1','2','3','4','5','6','7', '8','9',
    'A','B','C','D','E','F'};
void hexstr(char *outbuf, unsigned char *src, unsigned int srclen) {
    unsigned int outidx = 0;
    for(unsigned int i = 0; i < srclen; i++) {
        const unsigned char c = src[i];
        outbuf[outidx++] = hexchars[c >> 4];
        outbuf[outidx++] = hexchars[c & 0xf];
    }
    outbuf[outidx] = 0;
}

int flash_checkwrite(void) {
    int retval = 1;
    uint8_t testbuf[] = { 
        0x0f, 0x1e, 0x2d, 0x3c, 0x4b, 
        0x5a, 0x69, 0x78, 0x87, 0x96 
    };
    delaycs(5);
    flash_wren();
    uint8_t b = flash_rdsr();
    flash_wrsr(b & ~(0xc));

    delaycs(5);
    flash_wren();
    flash_secterase(0x0);
    delaycs(15);
    for(unsigned int i = 0; i < sizeof(testbuf); i++) {
        flash_wren();
        flash_bytewrite(i, testbuf[i]);
        // XXX: 50 us
        delaycs(5);
    }
    delaycs(25);
    uint8_t inbuf[10];
    for(unsigned int i = 0; i < sizeof(inbuf); i++) {
        inbuf[i] = 0;
    }
    flash_read(0x0, sizeof(inbuf), inbuf);
    for(unsigned int i = 0; i < sizeof(inbuf); i++) {
        if(inbuf[i] != testbuf[i]) {
            char strbuf[sizeof(inbuf)*2+1];
            hexstr(strbuf, inbuf, sizeof(inbuf));
            text_write_small(13, 48, strbuf);
            retval = 0;
        }
    }

    return retval;
}

int flash_checkid(void) {
    int retval = 1;
    setPin(FLASH_E, HIGH);
    setPin(SPI_SCK, LOW);
    setPin(FLASH_E, LOW);
    uint8_t b = spi_op(SPIFL_CMD_READID);
    b = spi_op(0x0);
    b = spi_op(0x0);
    b = spi_op(0x0);
    unsigned char databuf[4];
    databuf[0] = spi_op(0x0);
    databuf[1] = spi_op(0x0);
    databuf[2] = spi_op(0x0);
    databuf[3] = spi_op(0x0);
    if(databuf[0] != 0xbf || databuf[1] != 0x43 || databuf[2] != 0xbf || databuf[3] != 0x43) {
        char strbuf[9];
        hexstr(strbuf, databuf, 4);
        text_write_small(40, 40, strbuf);
        retval = 0;
    }
    setPin(FLASH_E, HIGH);
    return retval;
}

uint8_t spi_op(uint8_t outb) {
    uint8_t inb = 0;
    uint8_t i;
    for(i = 0; i < 8; i++) {
        setPin(SPI_SCK, LOW);
        if(getPin(SPI_MISO) == HIGH) {
            inb |= (1 << (7-i));
        }
        setPin(SPI_MOSI, ((((outb >> (7-i)) & 0x1) == 1) ? HIGH : LOW));
        setPin(SPI_SCK, HIGH);
    }
    return inb;
}

uint8_t flash_rdsr(void) {
    setPin(FLASH_E, HIGH);
    setPin(SPI_SCK, LOW);
    setPin(FLASH_E, LOW);

    uint8_t b = spi_op(0x5);
    b = spi_op(0x0);

    setPin(FLASH_E, HIGH);
    return b;
}

void flash_wren(void) {
    setPin(FLASH_E, HIGH);
    setPin(SPI_SCK, LOW);
    setPin(FLASH_E, LOW);

    spi_op(0x6);
    setPin(FLASH_E, HIGH);
}

void flash_wrsr(uint8_t sr) {
    setPin(FLASH_E, HIGH);
    setPin(SPI_SCK, LOW);
    setPin(FLASH_E, LOW);

    spi_op(0x50);

    setPin(FLASH_E, HIGH);
    // XXX: supposed to be 10 us
    delaycs(1);

    setPin(SPI_SCK, LOW);
    setPin(FLASH_E, LOW);

    spi_op(0x1);
    spi_op(sr);
    setPin(FLASH_E, HIGH);
}

void flash_secterase(int addr) {
    setPin(FLASH_E, HIGH);
    setPin(SPI_SCK, LOW);
    setPin(FLASH_E, LOW);

    spi_op(0x20);
    // casts not really necessary
    spi_op((uint8_t)((addr >> 16) & 0xff));
    spi_op((uint8_t)((addr >> 8) & 0xff));
    spi_op((uint8_t)(addr & 0xff));

    setPin(FLASH_E, HIGH);
}



void flash_read(int addr, int len, uint8_t *buf) {
    setPin(FLASH_E, HIGH);
    setPin(SPI_SCK, LOW);
    setPin(FLASH_E, LOW);

    spi_op(0x3);
    spi_op((uint8_t)(((addr >> 16) & 0xff)));
    spi_op((uint8_t)(((addr >> 8) & 0xff)));
    spi_op((uint8_t)(addr & 0xff));
    for(int i = 0; i < len; i++) {
        buf[i] = spi_op(0x0);
    }

    setPin(FLASH_E, HIGH);
}

void flash_bytewrite(int addr, uint8_t val) {
    setPin(FLASH_E, HIGH);
    setPin(SPI_SCK, LOW);
    setPin(FLASH_E, LOW);

    spi_op(0x2);
    spi_op((uint8_t)(((addr >> 16) & 0xff)));
    spi_op((uint8_t)(((addr >> 8) & 0xff)));
    spi_op((uint8_t)(addr & 0xff));
    spi_op(val);

    setPin(FLASH_E, HIGH);
}



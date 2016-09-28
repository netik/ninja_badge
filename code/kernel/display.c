#include <mc1322x.h>
#include <msgbox.h>
#include <board.h>
#include <ext.h>
#include <pinio.h>
#include <pinout.h>
#include "config.h"
#include <timer.h>
#include <misc.h>
#include "put.h"
#include <smallfont.h>
#include <largefont.h>
#include <lock.h>
#include <debug.h>
#include <task.h>
#include "syscalls.h"
#include <rfio.h>
#include <display.h>
#include <leftarrow.h>
#include <rightarrow.h>
#include <buttona.h>
#include <buttonb.h>

u_int8_t display_fbuf[1024];      // frame buffer
u_int8_t display_brightness;
int pagemap[] = { 3, 2, 1, 0, 7, 6, 5, 4 };

void display_init(void) {
    memset(display_fbuf, 0, sizeof(display_fbuf));
    pinFunc(BLA, 3);
    pinFunc(SID, 3);
    pinFunc(SCLK, 3);
    pinFunc(A0, 3);
    pinFunc(RST, 3);
    pinFunc(CS, 3);
    pinDirection(BLA, PIN_OUTPUT);
    pinDirection(SID, PIN_OUTPUT);
    pinDirection(SCLK, PIN_OUTPUT);
    pinDirection(A0, PIN_OUTPUT);
    pinDirection(RST, PIN_OUTPUT);
    pinDirection(CS, PIN_OUTPUT);

    setPin(BLA, 1);
    // toggle RST low to reset; CS low so it'll listen to us
    setPin(CS, LOW);
    setPin(RST, LOW);
    usleep(500000);
    setPin(RST, HIGH);
    // ADC select
    st7565_command(CMD_SET_ADC_NORMAL);
    // SHL select
    st7565_command(CMD_SET_COM_NORMAL);
    // LCD bias select
    st7565_command(CMD_SET_BIAS_7);

    usleep(20000);
    setPin(RST, HIGH);
    // turn on voltage converter (VC=1, VR=0, VF=0)
    st7565_command(CMD_SET_POWER_CONTROL | 0x4);
    // wait for 50% rising
    usleep(20000);
    // turn on voltage regulator (VC=1, VR=1, VF=0)
    st7565_command(CMD_SET_POWER_CONTROL | 0x6);
    // wait >=1ms
    usleep(1000);
    // turn on voltage follower (VC=1, VR=1, VF=1)
    st7565_command(CMD_SET_POWER_CONTROL | 0x7);
    // set lcd operating voltage (regulator resistor, ref voltage resistor)
    st7565_command(CMD_SET_RESISTOR_RATIO);
    // wait (10ms)
    usleep(10000);

    // initial display line
    // set page address
    // set column address
    // write display data
    st7565_command(CMD_DISPLAY_ON);
    st7565_command(CMD_SET_ALLPTS_NORMAL);
    display_brightness = 0x18;
    display_set_brightness(display_brightness);
    display_clear();
}
void display_clear(void) {
    memset(display_fbuf, 0x0, sizeof(display_fbuf));
    display_refresh();
}

void display_set_line(unsigned char i) {
        st7565_command(CMD_SET_DISP_START_LINE | i);
}

void display_copy(char *buf, unsigned int bmpx, unsigned int bmpy, 
        unsigned int dispx, unsigned int dispy, unsigned int width, 
        unsigned int height) {
    unsigned int by = 0, dy;
    unsigned int bbperrow = (width & 0x7) ? ((width & 0xf8) + 8) : width;
    by = 0;
    dy = dispy;
    unsigned char bshift = (dy & 7);
    if(bshift) {
        unsigned char mask = 0xff >> bshift;
        for(unsigned int i = 0; i < width; i++) {
            unsigned char c = display_fbuf[(dispx+16*(dy & 0xf8)) + i];
            c &= ~(mask);
            c |= ((~(buf[(bbperrow/8*by) + i]) >> bshift) & mask);
            display_fbuf[(dispx+16*(dy & 0xf8)) + i] = c;
        }
        dy += (8-bshift);
        by += (8-bshift);
    }
    for(; by < height && (height-by >= 8); by += 8, dy += 8) {
        for(unsigned int i = 0; i < width; i++) {
            unsigned int idx = (bbperrow) * (by / 8) + i;
            unsigned char c = 
                (buf[idx] << (by % 8)) |
                ((buf[idx + bbperrow] >> (8-(by % 8))));
            display_fbuf[(dispx+16*dy) + i] = ~c;
        }
    }
    if(by < height) {
        unsigned int mask = ~(0xff >> (height-by));
        for(unsigned int i = 0; i < width; i++) {
            unsigned char c = display_fbuf[(dispx+16*dy) + i];
            unsigned int idx = (bbperrow) * (by / 8) + i;
            c &= ~(mask);
            c |= 
                ~(
                    (buf[idx] << (by % 8))
                    | (buf[idx+bbperrow] >> (8-(by % 8)))
                ) & mask;
            display_fbuf[(dispx+16*dy) + i] = c;
        }
    }
    dispx = dispy = bmpx = bmpy;     // XXX
}

void display_refresh_flipped(void) {
    uint8_t c, p;
    for(p = 0; p < 8; p++) {
        st7565_command(CMD_SET_PAGE | pagemap[7-p]);
        st7565_command(CMD_SET_COLUMN_LOWER | (0x0 & 0xf));
        st7565_command(CMD_SET_COLUMN_UPPER | ((0x0 >> 4) & 0xf));
        st7565_data(0x00);

        for(c = 127; (c & 0x80) == 0; c--) {
            unsigned char x = display_fbuf[(128*p)+c];
            x = ((x * 0x0802LU & 0x22110LU) | (x * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16; 
            
            st7565_data(x);
        }

    }
}

void display_refresh(void) {
    uint8_t c, p;
    for(p = 0; p < 8; p++) {
        st7565_command(CMD_SET_PAGE | pagemap[p]);
        st7565_command(CMD_SET_COLUMN_LOWER | (0x0 & 0xf));
        st7565_command(CMD_SET_COLUMN_UPPER | ((0x0 >> 4) & 0xf));
        //st7565_command(CMD_RMW);
        //st7565_data(0xff);

        //st7565_data(0x80);
        //continue;
        st7565_data(0x00);

        for(c = 0; c < 128; c++) {
            st7565_data(display_fbuf[(128*p)+c]);
        }
    }
}

void st7565_command(uint8_t c) {
    setPin(A0, LOW);
    shiftMsb(SID, SCLK, c);
}
void st7565_data(uint8_t c) {
    setPin(A0, HIGH);
    shiftMsb(SID, SCLK, c);
}
void display_set_brightness(uint8_t val) {
    st7565_command(CMD_SET_VOLUME_FIRST);
    st7565_command(CMD_SET_VOLUME_SECOND | (val & 0x3f));
}

void smallfont_write(char *buf, unsigned int len, unsigned int x, unsigned int y) {
    unsigned int off = (y % 8);
    unsigned int topmask = (0xf8 >> off), bottommask = (0xf8 << (8-off));
    for(unsigned int i = 0; i < len; i++) {
        unsigned char ch = buf[i] & 0x7f;
        for(int j = 0; j < SMALLFONT_WIDTH; j++) {
            unsigned char c;
            c = display_fbuf[(128*(y/8)) + x + (SMALLFONT_WIDTH * i) + j];
            c &= ~(topmask);
            if(ch > 0x20 && ch < 0x5b) {
                if(buf[i] & 0x80) {
                    c |= ((~(smallfont_bmp[(ch-0x21) * SMALLFONT_WIDTH + j]) >> off) & topmask);
                } else {
                    c |= ((smallfont_bmp[(ch-0x21) * SMALLFONT_WIDTH + j] >> off) & topmask);
                }
            } else if(buf[i] & 0x80) {
                c |= (0xff & topmask);
            }
            display_fbuf[(128*(y/8)) + x + (SMALLFONT_WIDTH * i) + j] = c;
            c = display_fbuf[(128*(y/8)) + x + (SMALLFONT_WIDTH * i) + 128+ j];
            c &= ~(bottommask);
            if(ch > 0x20 && ch < 0x5b) {
                if(buf[i] & 0x80) {
                    c |= ((~(smallfont_bmp[(ch-0x21) * SMALLFONT_WIDTH + j]) << (8-off)) & bottommask);
                } else {
                    c |= ((smallfont_bmp[(ch-0x21) * SMALLFONT_WIDTH + j] << (8-off)) & bottommask);
                }
            } else if(buf[i] & 0x80) {
                c |= (0xff & bottommask);
            }
            display_fbuf[(128*(y/8)) + x + (SMALLFONT_WIDTH * i) + 128 + j] = c;
        }
    }
}
void largefont_write(char *buf, unsigned int len, unsigned int x, unsigned int y) {
    unsigned int off = (y % 8);
    unsigned int topmask = (0xff >> off), bottommask = (0xff << (8-off));
    for(unsigned int i = 0; i < len; i++) {
        unsigned char ch = buf[i] & 0x7f;
        for(int j = 0; j < LARGEFONT_WIDTH; j++) {
            unsigned char c;
            c = display_fbuf[(128*(y/8)) + x + (LARGEFONT_WIDTH * i) + j];
            c &= ~(topmask);
            if(ch > 0x20 && ch < 0x5b) {
                if(buf[i] & 0x80) {
                    c |= ((largefont_bmp[(ch-0x21) * LARGEFONT_WIDTH + j] >> off) & topmask);
                } else {
                    c |= ((~(largefont_bmp[(ch-0x21) * LARGEFONT_WIDTH + j]) >> off) & topmask);
                }
            } else if(buf[i] & 0x80) {
                c |= (0xff & topmask);
            }
            display_fbuf[(128*(y/8)) + x + (LARGEFONT_WIDTH * i) + j] = c;
            c = display_fbuf[(128*(y/8)) + x + (LARGEFONT_WIDTH * i) + 128+ j];
            c &= ~(bottommask);
            if(ch > 0x20 && ch < 0x5b) {
                if(buf[i] & 0x80) {
                    c |= ((largefont_bmp[(ch-0x21) * LARGEFONT_WIDTH + j] << (8-off)) & bottommask);
                } else {
                    c |= ((~(largefont_bmp[(ch-0x21) * LARGEFONT_WIDTH + j]) << (8-off)) & bottommask);
                }
            } else if(buf[i] & 0x80) {
                c |= (0xff & bottommask);
            }
            display_fbuf[(128*(y/8)) + x + (LARGEFONT_WIDTH * i) + 128 + j] = c;
        }
    }
}
void number_format(unsigned int num, char *buf, int max) {
    unsigned int power = 10;
    for(int u = max-1; u >= 0; u--) {
        if((num % power) == 0 && num == 0 && (u != (max-1))) {
            buf[u] = ' ';
        } else {
            buf[u] = '0' + ((num % power) / (power/10));
        }
        num -= (num % power);
        power *= 10;
    }
}
void erase_bottom(void) {
    for(int i = 0; i < 128; i++) {
        display_fbuf[128*6+i] |= 0x07;
        display_fbuf[128*7+i] = 0xff;
    }
}
void display_levelup(void) {
    for(int i = 0; i < 80; i++) {
        display_fbuf[128+24+i] = 0;
        display_fbuf[256+24+i] = 0;
        display_fbuf[384+24+i] = 0;
        display_fbuf[512+24+i] = 0;
    }
    largefont_write("LEVEL UP!", 9, 31, 12);
    char msg[] = {
        'L', 'E', 'V', 'E', 'L', ' ', ' ', ' ' };
    number_format(playerinfo.player_level, &msg[6], 2);
    largefont_write(msg, 8, 34, 26);
}
void erase_top(void) {
    for(int i = 0; i < 128; i++) {
        display_fbuf[128+i] |= 0xe0;
        display_fbuf[i] = 0xff;
    }
}


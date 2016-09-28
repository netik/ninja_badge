#ifndef DISPLAY_H
#define DISPLAY_H

void display_init(void);
void display_clear(void);
void display_set_brightness(u_int8_t val);
void display_copy(char *buf, unsigned int bmpx, unsigned int bmpy, 
        unsigned int dispx, unsigned int dispy, unsigned int width, 
        unsigned int height);
void display_refresh(void);
void display_refresh_flipped(void);
void display_set_line(unsigned char i);
void display_load_full_bitmap(int bmpid, int x, int y);
void st7565_command(uint8_t c);
void st7565_data(uint8_t c);
extern u_int8_t display_fbuf[1024];      // frame buffer
void smallfont_write(char *buf, unsigned int len, unsigned int x, unsigned int y);
void largefont_write(char *buf, unsigned int len, unsigned int x, unsigned int y);
void number_format(unsigned int num, char *buf, int max);
void erase_bottom(void);
void erase_top(void);
void display_levelup(void);

#define SMALLFONT_WIDTH 5
#define LARGEFONT_WIDTH 7

#define INV(x) ((x) | 0x80)

#define BLA 8     // backlight anode, for PWMing     (XXX: differs)
#define SID 36     // aka DB7
#define SCLK 35      // serial clock; aka DB6
#define A0 34      // aka RS in some datasheets; H for display, L for control
#define RST 33     // aka RESETB
#define CS 32

extern unsigned char leftarrow[], rightarrow[], buttona[], buttonb[], msgbox[];

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


#endif /* DISPLAY_H */

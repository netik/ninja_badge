#include <mc1322x.h>
#include <board.h>
#include <pinio.h>
#include "shuriken.h" 

#define BLA 8     // backlight anode, for PWMing
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

void st7565_init(void);
void st7565_command(uint8_t c);
void st7565_data(uint8_t c);
void st7565_set_brightness(uint8_t val);
void write_bitmap(unsigned char *bitmap);
void clear_bitmap(void);

void delaycs(uint32_t centis) {
    volatile uint32_t count = *CRM_RTC_COUNT;
    //count += (centis * 207);
    count += (centis * 40);
    while((*CRM_RTC_COUNT) < count) {
        continue;
    }
}
#define SER1P 1
#define SCK1P 3
#define RCK1P 2
#define SHIFT_OUT 0
#define LED_PWM 9


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


void main(void) {
    vreg_init();
    pinFunc(LED_RED, 3);
    pinDirection(LED_RED, PIN_OUTPUT);
    pinFunc(LED_GREEN, 3);
    pinDirection(LED_GREEN, PIN_OUTPUT);
    setPin(LED_RED, 1);
    setPin(LED_GREEN, 1);

//    while(1) {
//    setPin(LED_RED, 0);
//    setPin(LED_GREEN, 1);
//        for(volatile int ii = 0; ii < 400000; ii++) {continue; }
//    setPin(LED_RED, 1);
//    setPin(LED_GREEN, 0);
//        for(volatile int ii = 0; ii < 400000; ii++) {continue; }
//    }
    pinFunc(SHIFT_OUT, 3);
    pinDirection(SHIFT_OUT, PIN_INPUT);
    pinFunc(SER1P, 3);
    pinDirection(SER1P, PIN_OUTPUT);
    pinFunc(SCK1P, 3);
    pinDirection(SCK1P, PIN_OUTPUT);
    setPin(SER1P, 1);
    pinFunc(RCK1P, 3);
    pinDirection(RCK1P, PIN_OUTPUT);
    pinFunc(LED_PWM, 3);
    pinDirection(LED_PWM, PIN_OUTPUT);
    setPin(LED_PWM, 1);
    pinFunc(BLA, 3);
    pinDirection(BLA, PIN_OUTPUT);
    setPin(BLA, 1);
    pinFunc(SID, 3);
    pinDirection(SID, PIN_OUTPUT);
    pinFunc(SCLK, 3);
    pinDirection(SCLK, PIN_OUTPUT);
    pinFunc(A0, 3);
    pinDirection(A0, PIN_OUTPUT);
    pinFunc(RST, 3);
    pinDirection(RST, PIN_OUTPUT);
    pinFunc(CS, 3);
    pinDirection(CS, PIN_OUTPUT);


    st7565_init();
    st7565_command(CMD_DISPLAY_ON);
    st7565_command(CMD_SET_ALLPTS_NORMAL);
    st7565_set_brightness(0x18);
    clear_bitmap();
    write_bitmap(BITMAP);
    shift1(0xff);
    shift1(0xff);
    while(1) {
        setPin(LED_PWM, 1);
        delaycs(100);
    }


    while(1) {
        delaycs(100);
    }

    //vreg_init();
    pinFunc(SHIFT_OUT, 3);
    pinDirection(SHIFT_OUT, PIN_OUTPUT);
    pinFunc(SER1P, 3);
    pinDirection(SER1P, PIN_OUTPUT);
    pinFunc(SCK1P, 3);
    pinDirection(SCK1P, PIN_OUTPUT);
    pinFunc(RCK1P, 3);
    pinDirection(RCK1P, PIN_OUTPUT);
    pinFunc(LED_PWM, 3);
    pinDirection(LED_PWM, PIN_OUTPUT);
    pinFunc(LED_RED, 3);
    pinDirection(LED_RED, PIN_OUTPUT);
    setPin(SER1P, 1);
    setPin(SHIFT_OUT, 1);

    pinFunc(BLA, 3);
    pinDirection(BLA, PIN_OUTPUT);
    pinFunc(SID, 3);
    pinDirection(SID, PIN_OUTPUT);
    pinFunc(SCLK, 3);
    pinDirection(SCLK, PIN_OUTPUT);
    pinFunc(A0, 3);
    pinDirection(A0, PIN_OUTPUT);
    pinFunc(RST, 3);
    pinDirection(RST, PIN_OUTPUT);
    pinFunc(CS, 3);
    pinDirection(CS, PIN_OUTPUT);
    
    setPin(BLA, HIGH);

    st7565_init();
    st7565_command(CMD_DISPLAY_ON);
    st7565_command(CMD_SET_ALLPTS_NORMAL);
    st7565_set_brightness(0x18);
    clear_bitmap();
    write_bitmap(BITMAP);
    setPin(LED_PWM, 0);
    setPin(BLA, 0);
    shift1(0x0);
    shift1(0x0);
    delaycs(500);


    setPin(LED_PWM, 1);
    shift1(0xff);
    shift1(0xff);
    while(1) {
        delaycs(500);
        setPin(LED_PWM, 1);
        setPin(BLA, 1);
        setPin(LED_RED, 1);
        setPin(LED_GREEN, 1);
        delaycs(500);
        setPin(LED_PWM, 0);
        setPin(BLA, 1);
        setPin(LED_RED, 0);
        setPin(LED_GREEN, 0);
    }
    while(1) {
        st7565_command(CMD_SET_DISP_REVERSE);
        setPin(BLA, 1);
        setPin(LED_RED, 1);
        setPin(LED_GREEN, 1);
        delaycs(50);
        //st7565_command(CMD_SET_DISP_NORMAL);
        setPin(BLA, 1);
        setPin(LED_RED, 1);
        delaycs(50);
    }
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

void write_bitmap(unsigned char *bitmap) {
    uint8_t c, p;
    for(p = 0; p < 8; p++) {
        st7565_command(CMD_SET_PAGE | pagemap[p]);
        st7565_command(CMD_SET_COLUMN_LOWER | (0x0 & 0xf));
        st7565_command(CMD_SET_COLUMN_UPPER | ((0x0 >> 4) & 0xf));
        st7565_command(CMD_RMW);
        st7565_data(0xff);

        //st7565_data(0x80);
        //continue;

        for(c = 0; c < 128; c++) {
            st7565_data(bitmap[2+(128*p)+c]);
        }
    }
}


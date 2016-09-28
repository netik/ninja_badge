#include <mc1322x.h>
#include <board.h>
#include <pinio.h>
#include "config.h"
#include "put.h"

void delaycs(uint32_t centis) {
    volatile uint32_t count = *CRM_RTC_COUNT;
    //count += (centis * 207);
    count += (centis * 40);
    while((*CRM_RTC_COUNT) < count) {
        continue;
    }
}

#define BUTTON_UP 28
#define BLA 8     // backlight anode, for PWMing
#define SER1P 21
#define SCK1P 3
#define RCK1P 2
#define SHIFT_OUT 20
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
    setPin(LED_RED, 1);
    setPin(LED_GREEN, 1);
	vreg_init();

#if 0
    uart1_init(INC,MOD,SAMP);
	disable_irq(UART1);
    uint8_t *xxx = (uint8_t *)(0x408000);
    for(int i = 0; i < 1024; i += 16) {
        put_hex32((int)&xxx[i]);
        putstr(": ");
        put_hex(xxx[i]);
        putstr(" ");
        put_hex(xxx[i+1]);
        putstr(" ");
        put_hex(xxx[i+2]);
        putstr(" ");
        put_hex(xxx[i+3]);
        putstr(" ");
        put_hex(xxx[i+4]);
        putstr(" ");
        put_hex(xxx[i+5]);
        putstr(" ");
        put_hex(xxx[i+6]);
        putstr(" ");
        put_hex(xxx[i+7]);
        putstr(" ");
        put_hex(xxx[i+8]);
        putstr(" ");
        put_hex(xxx[i+9]);
        putstr(" ");
        put_hex(xxx[i+10]);
        putstr(" ");
        put_hex(xxx[i+11]);
        putstr(" ");
        put_hex(xxx[i+12]);
        putstr(" ");
        put_hex(xxx[i+13]);
        putstr(" ");
        put_hex(xxx[i+14]);
        putstr(" ");
        put_hex(xxx[i+15]);
        putstr("\r\n");
    }
#endif

    pinFunc(SHIFT_OUT, 3);
    pinDirection(SHIFT_OUT, PIN_INPUT);
    pinFunc(SER1P, 3);
    pinDirection(SER1P, PIN_OUTPUT);
    pinFunc(SCK1P, 3);
    pinDirection(SCK1P, PIN_OUTPUT);
    pinFunc(RCK1P, 3);
    pinDirection(RCK1P, PIN_OUTPUT);
    pinFunc(LED_PWM, 3);
    pinDirection(LED_PWM, PIN_OUTPUT);
    setPin(LED_PWM, 1);
    pinFunc(BLA, 3);
    pinDirection(BLA, PIN_OUTPUT);
    setPin(BLA, 1);
    pinFunc(LED_GREEN, 3);
    pinDirection(LED_GREEN, PIN_OUTPUT);
    pinFunc(LED_RED, 3);
    pinDirection(LED_RED, PIN_OUTPUT);
    setPin(SER1P, 1);


    shift1(0xff);
    shift1(0xff);
    while(1) {
        setPin(LED_PWM,0);
        delaycs(10);
        setPin(LED_PWM, 1);
        delaycs(10);
    }

    pinFunc(LED_PWM, 3);
    pinDirection(LED_PWM, PIN_OUTPUT);
    setPin(LED_PWM, 0);
    pinFunc(LED_GREEN, 3);
    pinDirection(LED_GREEN, PIN_OUTPUT);
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
    pinFunc(LED_GREEN, 3);
    pinDirection(LED_GREEN, PIN_OUTPUT);
    pinFunc(LED_RED, 3);
    pinDirection(LED_RED, PIN_OUTPUT);
    setPin(SER1P, 1);
    setPin(SHIFT_OUT, 1);

    setPin(BLA, 0);
    setPin(LED_PWM, 0);
    setPin(LED_RED, 0);
    setPin(LED_GREEN, 1);
    //pinFunc(BUTTON_UP, 3);
    //pinDircetion(BUTTON_UP, PIN_INPUT);
    //pinPullupEnable(BUTTON_UP, 1);
    //pinPullupSelect(BUTTON_UP, PIN_PULLUP);
    //*CRM_WU_CNTL |= ((1 << 20) | (1 << 21) | (1 << 22) | (1 << 23))
    //*CRM_BS_CNTL |= 3;
    shift1(0x00);
    shift1(0x1f);
    while(1) {
        setPin(LED_PWM,0);
        delaycs(2);
        setPin(LED_PWM, 1);
        delaycs(2);
    }
    while(1) {
        delaycs(500);
        //setPin(LED_PWM, 1);
        setPin(LED_RED, 1);
        setPin(LED_GREEN, 1);
        delaycs(500);
        setPin(LED_PWM, 0);
        setPin(LED_RED, 0);
        setPin(LED_GREEN, 0);
    }
    while(1) {
        shift1(0x55);
        //setPin(SER1P, 1);
        setPin(LED_PWM, 1);
        setPin(LED_RED, 0);
        //for(volatile int i = 0; i < 600000; i++) { continue; }
        delaycs(100);
        shift1(0xaa);
        //setPin(SER1P, 0);
        setPin(LED_PWM, 1);
        setPin(LED_RED, 1);
        delaycs(100);
        //for(volatile int i = 0; i < 600000; i++) { continue; }
    }
}

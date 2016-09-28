#include <mc1322x.h>
#include <board.h>
#include <pinio.h>

void delaycs(uint32_t centis) {
    volatile uint32_t count = *CRM_RTC_COUNT;
    //count += (centis * 207);
    count += (centis * 40);
    while((*CRM_RTC_COUNT) < count) {
        continue;
    }
}

#define SER1P 21
#define SCK1P 3
#define RCK1P 2
#define SHIFT_OUT 20
#define LED_PWM 9

#define BUTTON_UP 28
#define BUTTON_DOWN 23
#define BUTTON_LEFT 25
#define BUTTON_RIGHT 24
#define BUTTON_B 27
#define BUTTON_A 26

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


#define LED_00 (0x8000)
#define LED_01 (0x4000)
#define LED_02 (0x80)
#define LED_03 (0x40)
#define LED_04 (0x20)
#define LED_05 (0x10)

void main(void) {
    vreg_init();
    pinFunc(SHIFT_OUT, 3);
    pinFunc(SER1P, 3);
    pinFunc(SCK1P, 3);
    pinFunc(RCK1P, 3);
    pinFunc(LED_PWM, 3);
    pinFunc(LED_RED, 3);
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
    pinDirection(SHIFT_OUT, PIN_OUTPUT);
    pinDirection(SER1P, PIN_OUTPUT);
    pinDirection(SCK1P, PIN_OUTPUT);
    pinDirection(RCK1P, PIN_OUTPUT);
    pinDirection(LED_PWM, PIN_OUTPUT);
    pinDirection(LED_RED, PIN_OUTPUT);
    pinDirection(BUTTON_DOWN, PIN_INPUT);
    pinDirection(BUTTON_LEFT, PIN_INPUT);
    pinDirection(BUTTON_RIGHT, PIN_INPUT);
    pinDirection(BUTTON_B, PIN_INPUT);
    pinDirection(BUTTON_A, PIN_INPUT);
    pinDirection(BUTTON_UP, PIN_INPUT);
    setPin(SER1P, 1);
    setPin(SHIFT_OUT, 1);
    setPin(LED_PWM, 1);
    setPin(LED_RED, 0);
    int curval;
    while(1) {
        curval = 0;
        if(getPin(BUTTON_UP)) {
            curval |= LED_00;
        } else {
            setPin(LED_RED, 1);
        }
        if(getPin(BUTTON_DOWN)) {
            curval |= LED_01;
        }
        if(getPin(BUTTON_LEFT)) {
            curval |= LED_02;
        }
        if(getPin(BUTTON_RIGHT)) {
            curval |= LED_03;
        }
        if(getPin(BUTTON_B)) {
            curval |= LED_04;
        }
        if(getPin(BUTTON_A)) {
            curval |= LED_05;
        }
        shift1(curval >> 8);
        shift1(curval & 0xff);
        delaycs(10);
    }
}


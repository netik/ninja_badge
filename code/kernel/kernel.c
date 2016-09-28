#include <mc1322x.h>
#include <board.h>
#include <ext.h>
#include <pinio.h>
#include <pinout.h>
#include "config.h"
#include <timer.h>
#include <misc.h>
#include "put.h"
#include <lock.h>
#include <debug.h>
#include <task.h>
#include "syscalls.h"
#include <rfio.h>
#include <info.h>
#include <globals.h>
#include <display.h>

volatile struct globalvars globals;

void ui_init(void) {
    pinFunc(SHIFT_OUT, 3);
    pinFunc(SER1P, 3);
    pinFunc(SCK1P, 3);
    pinFunc(RCK1P, 3);
    pinFunc(LED_PWM, 1);
    pinDirection(SHIFT_OUT, PIN_INPUT);
    pinDirection(SER1P, PIN_OUTPUT);
    pinDirection(SCK1P, PIN_OUTPUT);
    pinDirection(RCK1P, PIN_OUTPUT);
    pinDirection(LED_PWM, PIN_OUTPUT);

    pinDirection(BUZZER_PIN, PIN_OUTPUT);
    pinFunc(BUZZER_PIN, 1);
    // XXX: remove?
}
void fordelay(unsigned int val) {
    for(volatile unsigned int v = 0; v < val; v++) {
        continue;
    }
}
unsigned int escapable_clockus(unsigned int microsecs) {
    unsigned int k = clockus + microsecs;
    if(k < clockus) {
        while(k < clockus) {
            if((*GPIO_DATA0 & (1 << BUTTON_B)) == 0) {
                return 1;
            }
            continue;
        }
    }
    while(clockus < k) {
        if((*GPIO_DATA0 & (1 << BUTTON_B)) == 0) {
            return 1;
        }
        continue;
    }
    return 0;
}

void clockusdelay(unsigned int microsecs) {
    unsigned int k = clockus + microsecs;
    if(k < clockus) {
        while(k < clockus) {
            continue;
        }
    }
    while(clockus < k) {
        continue;
    }
    return;
}

void halt_failure(unsigned int red, unsigned int green) {
    putstr("XXX HALT: ");
    put_hex32(red);
    putstr(" ");
    put_hex32(green);
    putstr(NEWLINE);
    while(1) {
        fordelay(1000000);
        setPin(LED_RED, 0);
        setPin(LED_GREEN, 0);
        for(unsigned int i = 0; i < red; i++) {
            setPin(LED_RED, 1);
            fordelay(500000);
            setPin(LED_RED, 0);
            fordelay(500000);
        }
        fordelay(1000000);
        for(unsigned int i = 0; i < green; i++) {
            setPin(LED_GREEN, 1);
            fordelay(500000);
            setPin(LED_GREEN, 0);
            fordelay(500000);
        }
    }
}

void ui_main(void) {
    ui_init();
    display_init();
    globals.togrant = 0xff;
    unsigned int screen = SCREEN_SPLASH;
    while(1) {
        unsigned int (*ptr)(void);
        ptr = ext_load_screen(screen);
        if(ptr == NULL) {
            halt_failure(FAILURE_EXTLOAD, screen);
        }
        screen = ptr();
    }
}

void button_isr(void) {
    task *ct = &tasks[TASK_UI];
    if(!(ct->button_waiting)) {
        return;
    }
    if(ct->timer_waiting) {
        if(ct->tevent == NULL) {
            DEBUG_RFIO("chill packet notify: no tevent but waiting on timer!\r\n");
        }
        timer_cancel(ct->tevent);
        ct->tevent = NULL;
    }
    task_wakeup_chill(TASK_UI, RET_BUTTON);
}

/* This runs as the lowest-priority task; it can't sleep.  Its job is to poll for 
 * button presses.
 */
#define BUTTONS_MASK (  \
        (1 << BUTTON_UP) | (1 << BUTTON_DOWN) | (1 << BUTTON_LEFT)  \
        | (1 << BUTTON_RIGHT) | (1 << BUTTON_A) | (1 << BUTTON_B))

#if 0
 XXX: ECONOTAG
#define BUTTONS_MASK (  \
        (1 << BUTTON_A) | (1 << BUTTON_B))
#endif


u_int32_t buttons_down;
void button_init(void) {
    buttons_down = 0;
    pinDirection(BUTTON_DOWN, PIN_INPUT);
    pinDirection(BUTTON_LEFT, PIN_INPUT);
    pinDirection(BUTTON_RIGHT, PIN_INPUT);
    pinDirection(BUTTON_B, PIN_INPUT);
    pinDirection(BUTTON_A, PIN_INPUT);
    pinDirection(BUTTON_UP, PIN_INPUT);
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

    // XXXXXXXXXXXXXXXXXXXXX ECONOTAG
    //pinPullupSelect(BUTTON_DOWN, PIN_PULLDOWN);
    //pinPullupSelect(BUTTON_UP, PIN_PULLDOWN);
    //pinPullupEnable(BUTTON_UP, 0);
    //pinPullupEnable(BUTTON_DOWN, 0);
    //pinDirection(BUTTON_UP, PIN_OUTPUT);
    //pinDirection(BUTTON_DOWN, PIN_OUTPUT);
    //setPin(BUTTON_UP, 0);
    //setPin(BUTTON_DOWN, 0);
    // XXXXXXXXXXXXXXXXXXXXX ECONOTAG
}

#define BUTTON_THRESHOLD_US 10000       /* 1ms */

u_int32_t get_cpsr(void);
void button_loop(void) {
    u_int32_t firstlow[6];
    u_int32_t is_high, is_low;
    is_high = BUTTONS_MASK;
    is_low = 0;
    enable_irq(CRM);
    while(1) {
        for(volatile int z = 0; z < 10000; z++) { }
        u_int32_t curval = *GPIO_DATA0;
        u_int32_t curbit = (1 << BUTTON_UP), shouldforce = 0;
        for(unsigned int i = 0; i < 6; (i++)) {
            if(!(curbit & BUTTONS_MASK)) { //XXX ECONOTAG
                curbit <<= 1;
                continue;   // XXX ECONOTAG
            }   // XXX ECONOTAG
            if(!(curval & curbit)) {
                if(is_low & curbit) {
                    putstr("XXX down\r\n");
                    u_int32_t downfor;
                    if(clockus < firstlow[i]) {
                        downfor = 0xffffffff - firstlow[i] + clockus;
                    } else {
                        downfor = firstlow[i] - clockus;
                    }
                    if(downfor >= BUTTON_THRESHOLD_US) {
                        buttons_down |= (curbit);
                        is_low &= ~(curbit);
                        shouldforce = 1;
                    }
                } else if(is_high & curbit) {
                    firstlow[i] = clockus;
                    is_low |= (curbit);
                    is_high &= ~(curbit);
                } else if(buttons_down & curbit) {
                } else {
                    buttons_down &= ~(curbit);
                    is_low &= ~(curbit);
                    is_high |= curbit;
                    if(i > 6) {
                        putstr("???wtf\r\n");
                    }
                    putstr("XXX none ");
                    put_hex32(i);
                    putstr(" ");
                    put_hex32((u_int32_t)curval);
                    putstr(" ");
                    put_hex32((u_int32_t)curbit);
                    putstr(" ");
                    put_hex32((u_int32_t)buttons_down);
                    putstr(" ");
                    put_hex32((u_int32_t)is_low);
                    putstr(" ");
                    put_hex32((u_int32_t)is_high);
                    putstr("\r\n");
                    for(int ix = 0; ix < NTASKS; ix++) {
                        put_hex32(tasks[ix].gpregs[13]);
                        putstr(" ");
                    }
                    putstr("\r\n");
                    putstr("!\r\n");
                }
            } else {
                buttons_down &= ~(curbit);
                is_low &= ~(curbit);
                is_high |= curbit;
            }
            curbit <<= 1;
        }
        if(shouldforce) {
            *INTFRC |= (1 << INT_NUM_CRM);
        }
    }
}
unsigned int uistack[2048];
unsigned int rfiostack[256];
unsigned int wdstack[128];

void main(void) {	
    force_maca = 0;
    maca_off();     // XXX
    trim_xtal();
    uart1_init(INC, MOD, SAMP);
    disable_irq(UART1);
    memset((char *)&globals, 0, sizeof(globals));
    pinFunc(LINK_PIN, 3);
    pinDirection(LINK_PIN, PIN_INPUT);
    pinFunc(LED_RED, 3);
    pinFunc(LED_GREEN, 3);
    pinDirection(LED_GREEN, PIN_OUTPUT);
    pinDirection(LED_RED, PIN_OUTPUT);
    setPin(LED_RED, 0);
    setPin(LED_GREEN, 0);
    ext_init();
    vreg_init();
    info_init();
    info_load();
    rfio_init();
    task_init();
    maca_init();
    timer_init();

    putstr("\r\n--- start\r\n");
    
    starttask(TASK_UI, &uistack[(sizeof(uistack)/sizeof(unsigned int))-1], ui_main);
    starttask(TASK_RFIO, &rfiostack[(sizeof(rfiostack)/sizeof(unsigned int))-1], rfio_run);
    starttask(TASK_RFIOWATCHDOG, &wdstack[(sizeof(wdstack)/sizeof(unsigned int))-1], rfio_watchdog);
    button_init();
    button_loop();
}


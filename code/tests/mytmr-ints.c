#include <mc1322x.h>
#include <board.h>

#include "tests.h"
#include "config.h"

volatile uint8_t led;

#define ECONO_RED 0x00001000
#define ECONO_GREEN 0x00002000
#define LED_VAL ECONO_RED

#define reg(x) (*(volatile uint32_t *)(x))
#define led_init() do { *GPIO_PAD_DIR1 = LED_VAL; } while(0);
#define led_on() do  { reg(GPIO_DATA1) |= LED_VAL; } while(0);
#define led_off() do { reg(GPIO_DATA1) &= ~(LED_VAL); } while(0);

//#define LED_VAL LED_PURPLE
//#define led_init() do { *GPIO_PAD_DIR0 = LED_VAL; } while(0);
//#define led_on() do  { led = 1; *GPIO_DATA0 = LED_VAL; } while(0);
//#define led_off() do { led = 0; *GPIO_DATA0 = 0x00000000; } while(0);

void toggle_led(void) {
    led = (led + 1) % 4;
	if(led == 0) {
		led_on();
	} else {
		led_off();
	}
}
static unsigned int compare = 18750;

void tmr_isr(void) {
    if(compare > 18750) {
        compare = 18750;
    }
    compare = compare - 100;
	toggle_led();
	*TMR0_SCTRL = 0;
	*TMR0_CSCTRL = 0x0041; /* clear compare flag */
    *TMR0_CMPLD1 = 46875;
	
}


void main(void) {

	/* pin direction */
	led_init();

	/* timer setup */
	/* CTRL */
#define COUNT_MODE 1      /* use rising edge of primary source */
#define PRIME_SRC  0xf    /* Perip. clock with 128 prescale (for 24Mhz = 187500Hz)*/
#define SEC_SRC    0      /* don't need this */
#define ONCE       0      /* keep counting */
#define LEN        1      /* count until compare then reload with value in LOAD */
#define DIR        0      /* count up */
#define CO_INIT    0      /* other counters cannot force a re-initialization of this counter */
#define OUT_MODE   0      /* OFLAG is asserted while counter is active */

	*TMR_ENBL     = 0;                    /* tmrs reset to enabled */
	*TMR0_SCTRL   = 0;
	*TMR0_CSCTRL  = 0x0040;
	*TMR0_LOAD    = 0;                    /* reload to zero */
	*TMR0_COMP_UP = 18750;                /* trigger a reload at the end */
	*TMR0_CMPLD1  = 18750;                /* compare 1 triggered reload level, 10HZ maybe? */
	*TMR0_CNTR    = 0;                    /* reset count register */
	*TMR0_CTRL    = (COUNT_MODE<<13) | (PRIME_SRC<<9) | (SEC_SRC<<7) | (ONCE<<6) | (LEN<<5) | (DIR<<4) | (CO_INIT<<3) | (OUT_MODE);
	*TMR_ENBL     = 0xf;                  /* enable all the timers --- why not? */

	led_on();

	enable_tmr_irq();

	while(1) {
		/* sit here and let the interrupts do the work */
	};
}

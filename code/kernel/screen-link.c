#include <mc1322x.h>
#include <board.h>
#include <ext.h>
#include <pinio.h>
#include <pinout.h>
#include "config.h"
#include <rfio.h>
#include <globals.h>
#include <misc.h>
#include <display.h>
#include "put.h"
#include <lock.h>
#include <debug.h>
#include <task.h>
#include "syscalls.h"
#include <screen-link.h>
#include <info.h>

unsigned int screen_link(void) {
    display_clear();
    smallfont_write("GET YOURSELF CONNECTED", 22, (64-(22*SMALLFONT_WIDTH/2)), 30);
    display_refresh();
    pinFunc(LINK_PIN, 3);
    pinDirection(LINK_PIN, PIN_INPUT);
    unsigned int half_bit = 400;
    unsigned int full_bit = 800;
    while(1) {
        unsigned char b = 0;
        while((*GPIO_DATA0 & (1 << LINK_PIN)) != 0) {
            if((*GPIO_DATA0 & (1 << BUTTON_B)) == 0) {
                return 1;
            }
        }
        if(escapable_clockus(half_bit)) {
            return SCREEN_MAIN;
        }
        if((*GPIO_DATA0 & (1 << LINK_PIN)) == 0) {
            for(int i = 0; i < 8; i++) {
                escapable_clockus(full_bit);
                if(*GPIO_DATA0 & (1 << LINK_PIN)) {
                    b |= (1 << i);
                }
            }
            if(escapable_clockus(full_bit * 2)) {
                return SCREEN_MAIN;
            }
        }
        if(b != 0) {
            /* Be extremely lenient; both badges can be interrupted.. */
            if(b == 'N'
                    || (b == ((('N') << 1) & 0xff))
                    || (b == ((('N') << 2) & 0xff))) {
                globals.togrant = 4;
                return SCREEN_GRANT;
            }
        }
        if((*GPIO_DATA0 & (1 << BUTTON_B)) == 0) {
            return SCREEN_MAIN;
        }
    }
    return SCREEN_MAIN;
}

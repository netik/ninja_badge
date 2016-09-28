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
#include <screen-setup.h>
#include <info.h>


unsigned int screen_setup(void) {
    char *lines[] = {
        "CHANGE NAME",
        "SOUND",
        "LINK",
    };
    unsigned int opt = 0;
    char buf[128];
    while(1) {
        display_clear();
        for(unsigned int i = 0; i < sizeof(lines)/sizeof(char *); i++) {
            unsigned int len = strlen(lines[i]);
            if(opt == i) {
                for(unsigned int c = 0; c < len; c++) {
                    buf[c] = lines[i][c] | 0x80;
                }
                smallfont_write(buf, len, (64-len*SMALLFONT_WIDTH/2), 10+i*7);
            } else {
                smallfont_write(lines[i], len, (64-len*SMALLFONT_WIDTH/2), 10+i*7);
            }
        }
        display_refresh();
        unsigned int retval = chill(
                CHILL_TIMEOUT | CHILL_BUTTON,
                10000000,
                NULL);
        if(retval == RET_BUTTON) {
            if(buttons_down & (1 << BUTTON_DOWN)) {
                if(opt < sizeof(lines)/sizeof(char *)-1) {
                    opt++;
                }
            }
            if(buttons_down & (1 << BUTTON_UP)) {
                if(opt > 0) {
                    opt--;
                }
            }
            if(buttons_down & (1 << BUTTON_A)) {
                char *msg;
                if(opt == 0) {
                    return SCREEN_INPUT_USER;
                } else if(opt == 1) {
                    if(playerinfo.buzzer_disabled) {
                        msg = "BUZZER ENABLED";
                        playerinfo.buzzer_disabled = 0;
                    } else {
                        msg = "BUZZER DISABLED";
                        playerinfo.buzzer_disabled = 1;
                    }
                    display_clear();
                    unsigned int len = strlen(msg);
                    largefont_write(msg, len, 64-(len*LARGEFONT_WIDTH/2), 31);
                    display_refresh();
                    info_commit_playerinfo();
                    chill(
                            CHILL_TIMEOUT | CHILL_BUTTON,
                            3000000,
                            NULL);
                } else if(opt == 2) {
                    return SCREEN_LINK;
                } else {
                    break;
                }
            }
            if(buttons_down & (1 << BUTTON_B)) {
                break;
            }
        } else if(retval == RET_TIMEOUT) {
            return SCREEN_MAIN;
        }
    }
    return SCREEN_MAIN;
}

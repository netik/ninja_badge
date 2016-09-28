#include <mc1322x.h>
#include <board.h>
#include <display.h>
#include <ext.h>
#include <pinio.h>
#include <pinout.h>
#include "config.h"
#include <misc.h>
#include "put.h"
#include <rfio.h>
#include <lock.h>
#include <debug.h>
#include <task.h>
#include "syscalls.h"
#include <screen-inputuser.h>
#include <info.h>

unsigned int screen_inputuser(void) {
    char buf[512];
    unsigned int width, height;
    ext_load_bitmap(NINJA_IDLE, buf, sizeof(buf), &width, &height); 
    display_clear();
    display_copy(buf, 0, 0, 0, 10, width, height);
    smallfont_write("NAME:", 5, 0, 0);
    char name[10];
    char donetxt[] = {'D', 'O', 'N', 'E' };
    int ondone = 0;
    int curchar = 0;
    memset(name, ' ', 9);
    if(playerinfo.name_set_flag) {
        memcpy(name, (char *)playerinfo.name, stringlen((char *)playerinfo.name, 9));
    } else {
        name[curchar] = 'A';
    }
    name[9] = 0;
    while(1) { 
        if(ondone == 0) {
            name[curchar] |= 0x80;
        } else {
            for(int i = 0; i < 4; i++) {
                donetxt[i] |= 0x80;
            }
        }
        largefont_write(name, 9, 30, 20);
        name[curchar] &= 0x7f;
        smallfont_write(donetxt, 4, 64, 40);
        display_refresh();
        unsigned int retval = chill(
                (CHILL_BUTTON | CHILL_TIMEOUT),
                50000,
                NULL);
        if(retval == RET_BUTTON) {
            if(buttons_down & ((1 << BUTTON_UP) | (1 << BUTTON_DOWN)))  {
                ondone ^= 1;
                name[curchar] &= 0x7f;
                for(int i = 0; i < 4; i++) {
                    donetxt[i] &= 0x7f;
                }
            }
            if(buttons_down & (1 << BUTTON_B) && ondone == 0) {
                name[curchar] &= 0x7f;
                if(curchar == 0) {
                    curchar = 8;
                } else {
                    curchar--;
                }
            }
            if(buttons_down & (1 << BUTTON_A)) {
                if(ondone == 1) {
                    unsigned int sp;
                    for(sp = 8; sp > 0; sp--) {
                        if(name[sp] != ' ') {
                            break;
                        }
                    }
                    name[sp+1] = 0;
                    memset((char *)playerinfo.name, 0, sizeof(playerinfo.name));
                    memcpy((char *)playerinfo.name, name, sp+1);
                    playerinfo.name_set_flag = 1;
                    if(playerinfo.name_set_flag == 0 && badgeinfo.is_ninja == 0) {
                        playerinfo.redbull_count = 0;
                        playerinfo.shuriken_count = 0;
                        playerinfo.item_mask = 0;
                        playerinfo.hp = 1337;
                        playerinfo.player_level = 1;
                    }
                    info_commit_playerinfo();
                    return SCREEN_MAIN;
                }
                name[curchar] &= 0x7f;
                if(curchar == 8) {
                    curchar = 0;
                } else {
                    curchar++;
                    if(name[curchar] == ' ') {
                        name[curchar] = 'A';
                    }
                }
            }
            if(buttons_down & (1 << BUTTON_RIGHT)) {
                if(name[curchar] == 0x20) {
                    name[curchar] = '(';
                } else if(name[curchar] >= 'Z') {
                    name[curchar] = 0x20;
                } else {
                    name[curchar]++;
                }
            }
            if(buttons_down & (1 << BUTTON_LEFT)) {
                if(name[curchar] <= 0x20) {
                    name[curchar] = 'Z';
                } else if(name[curchar] == '(') {
                    name[curchar] = 0x20;
                } else {
                    name[curchar]--;
                }
            }
        }
    }
    return SCREEN_MAIN;
}

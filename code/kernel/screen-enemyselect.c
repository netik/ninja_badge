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
#include <boss_skull.h>
#include <lock.h>
#include <debug.h>
#include <task.h>
#include "syscalls.h"
#include <screen-enemyselect.h>
#include <info.h>

unsigned int screen_enemyselect(void) {
    if(!lock_delay(&nearbyslock, 1000000) ) {
        globals.giveitem = 0;
        return SCREEN_MAIN;
    }
    struct entry {
        u_int32_t lastseen;
        unsigned int index; 
    } tosort[NNEARBY];
    int n = 0;
    for(int i = 0; i < NNEARBY; i++) {
        if(nearbys[i].level != 0) {
            tosort[n].index = i;
            tosort[n].lastseen = nearbys[i].lastseen;
            n++;
        }
    }
    if(n == 0) {
        lock_unlock(&nearbyslock);
        display_clear();
        smallfont_write("NO ENEMIES NEARBY", 17, 4, 28);
        display_refresh();
        while(1) {
            chill((CHILL_BUTTON | CHILL_TIMEOUT),
                    5000000,
                    NULL);
            globals.giveitem = 0;
            return SCREEN_MAIN;
        }
    }
    // gnome sort.  
    int i = 0;
    unsigned int mc = *MACA_CLK;
    while(i < n) {
        if(i == 0 || 
                (mc - tosort[i-1].lastseen) < (mc - tosort[i].lastseen)) {
            i++;
        } else {
            unsigned int xidx = tosort[i].index,
                         xlast = tosort[i].lastseen;
            tosort[i].index = tosort[i-1].index; 
            tosort[i].lastseen = tosort[i-1].lastseen;
            i--;
            tosort[i].index = xidx;
            tosort[i].lastseen = xlast;
        }
    }
    int curpos = 0;
    while(1) {
        display_clear();
        if(curpos > 0) {
            display_copy((char *)leftarrow, 0, 0, 0, 17, 10, 17);
        }
        if(curpos < (n-1)) {
            display_copy((char *)rightarrow, 0, 0, 118, 17, 10, 17);
        }
        for(i = 0; i < 64; i++) {
            unsigned char c = display_fbuf[128*3+15+i];
            c |= 0x10;
            display_fbuf[128*3+15+i] = c;
        }
        unsigned int len = stringlen(nearbys[tosort[curpos].index].name, 9);
        largefont_write(nearbys[tosort[curpos].index].name, len, 15, 18);
        char lbuf[] = { 'L', 'E', 'V', 'E', 'L', ' ', ' ', ' ', 0 };
        number_format(nearbys[tosort[curpos].index].level, &lbuf[6], 2);
        smallfont_write(lbuf, 8, 15, 29);
        char ebuf[] = { ' ', ' ', ' ', 'N', 'E', 'A', 'R', 'B', 'Y', 0 };
        number_format(n, ebuf, 2);
        smallfont_write(ebuf, 9, 2, 45);
        int enemy;
        if(nearbys[tosort[curpos].index].is_boss != 0) {
            enemy = get_enemy_id(nearbys[tosort[curpos].index].is_boss);
            display_copy((char *)boss_skull, 0, 0, 50, 4, 9, 8);
        } else if(nearbys[tosort[curpos].index].is_ninja) {
            enemy = ENEMY_RIGHTSENSEI;
        } else {
            enemy = ENEMY_RIGHTNINJA;
        }
        char buf[512];
        unsigned int width, height;
        ext_load_bitmap(enemyinfo[enemy].fighting, buf, sizeof(buf), &width, &height);
        unsigned int startx = 115-width+1;
        display_copy(buf, 0, 0, startx, 4, width, height);
        erase_bottom();
        display_copy((char *)buttona, 0, 0, 11, 55, 7, 7);
        char attbuf[] = { INV('A'), INV('T'), INV('T'), INV('A'), INV('C'), INV('K') };
        smallfont_write(attbuf, 6, 20, 56);
        char canbuf[] = { INV('C'), INV('A'), INV('N'), INV('C'), INV('E'), INV('L') };
        display_copy((char *)buttonb, 0, 0, 79, 55, 7, 7);
        smallfont_write(canbuf, 6, 88, 56);
        display_refresh();
        unsigned int timeout = 0;
        while(timeout < 100) {
            unsigned int retval = chill(
                    (CHILL_BUTTON | CHILL_TIMEOUT),
                    50000,
                    NULL);
            if(retval == RET_TIMEOUT) {
                timeout++;
            } else {
                timeout = 0;
                break;
            }
        }
        if(timeout == 100) {
            break;
        }
        if(buttons_down & (1 << BUTTON_B)) {
            lock_unlock(&nearbyslock);
            globals.giveitem = 0;
            return SCREEN_MAIN;
        }
        if(buttons_down & (1 << BUTTON_LEFT)) {
            if(curpos > 0) {
                curpos = curpos - 1;
            }
        } 
        if(buttons_down & (1 << BUTTON_RIGHT)) {
            if(curpos < (n-1)) {
                curpos++;
            }
        }
        if(buttons_down & (1 << BUTTON_DOWN) && badgeinfo.is_ninja == 1) {
            packet_t packet;
            grant_item(nearbys[tosort[curpos].index].badgeid, &packet, 11, 1);
            lock_unlock(&nearbyslock);
            globals.giveitem = 0;
            return SCREEN_MAIN;
        }
        if(buttons_down & (1 << BUTTON_A)) {
            if(badgeinfo.is_boss > 0 && badgeinfo.is_boss < 4) {
                packet_t packet;
                grant_item(nearbys[tosort[curpos].index].badgeid, &packet, badgeinfo.is_boss, 2);
                lock_unlock(&nearbyslock);
                globals.giveitem = 0;
                return SCREEN_MAIN;
            } else if(badgeinfo.is_ninja != 0 && globals.giveitem == 1) {
                globals.giveitem = 0;
                packet_t packet;
                grant_item(nearbys[tosort[curpos].index].badgeid, &packet, globals.togrant, 2);
                lock_unlock(&nearbyslock);
                return SCREEN_MAIN;
            } else {
                globals.was_attacked = 0;
                globals.giveitem = 0;
                globals.opponent = nearbys[tosort[curpos].index].badgeid;
                lock_unlock(&nearbyslock);
                return SCREEN_FIGHT;
            }
        }
    }
    lock_unlock(&nearbyslock);
    globals.giveitem = 0;
    return SCREEN_MAIN;
}


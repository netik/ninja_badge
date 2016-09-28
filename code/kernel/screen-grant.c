#include <mc1322x.h>
#include <board.h>
#include <buzzer.h>
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
#include <screen-grant.h>
#include <info.h>

unsigned int screen_grant(void) {
    if(badgeinfo.is_boss && badgeinfo.is_ninja == 0) {
        return SCREEN_MAIN;
    }
    if((globals.togrant < 10 && playerinfo.item_mask & (1 << globals.togrant)) 
            || globals.togrant > 11) {
        return SCREEN_MAIN;
    }
    if(globals.togrant == 10) {
        if(playerinfo.shuriken_count < 15) {
            playerinfo.shuriken_count++;
        } else {
            return SCREEN_MAIN;
        }
    } else if(globals.togrant == 11) {
        if(playerinfo.redbull_count < 15) {
            playerinfo.redbull_count++;
        } else {
            return SCREEN_MAIN;
        }
    } else {
        playerinfo.item_mask |= (1 << globals.togrant);
    }
    char buf[512];
    char msg[] = {
        INV('Y'), INV('O'), INV('U'),  INV(' '),  INV('G'),  INV('O'),  INV('T'),  INV(' '), 
        INV('A'), INV('N'), INV(' '),  INV('I'),  INV('T'),  INV('E'),  INV('M'),  INV('!')
    };
    unsigned int item = globals.togrant;
    unsigned int width, height;
    display_clear();
    ext_load_bitmap(items[item].image, buf, sizeof(buf), &width, &height);
    display_copy(buf, 0, 0, 43, 10, width, height);
    erase_bottom();
    erase_top();

    memset(buf, 0, sizeof(buf));
    unsigned int len = strlen(items[item].name);
    memcpy(buf, items[item].name, len);
    for(unsigned int i = 0; i < len; i++) {
        buf[i] |= 0x80;
    }
    smallfont_write(buf, len, 64-((SMALLFONT_WIDTH*len)/2), 3);
    smallfont_write(msg, sizeof(msg), 64-((SMALLFONT_WIDTH*sizeof(msg))/2), 57);
    play_victory_tone();
    display_refresh();
    usleep(2000000);
    if(globals.togrant < 5) {
        playerinfo.xp += 300;
    } else if(globals.togrant < 8) {
        playerinfo.xp += 500;
    } else if(globals.togrant < 9) {
        playerinfo.xp += 700;
    }
    if(calc_level(playerinfo.xp) > playerinfo.player_level) {
        playerinfo.player_level = calc_level(playerinfo.xp);
        display_levelup();
        display_refresh();
        play_victory_tone();
        usleep(2500000);
    }
    info_commit_playerinfo();
    display_refresh();

    chill(
            CHILL_TIMEOUT | CHILL_BUTTON,
            5000000,
            NULL);
    return SCREEN_MAIN;
}

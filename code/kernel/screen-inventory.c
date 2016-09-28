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
#include <screen-inventory.h>
#include <info.h>

int next_item(int item) {
    for(int i = item+1; i < 10; i++) {
        if(playerinfo.item_mask & (1 << i)) {
            return i;
        }
    }
    if(item < 10 && playerinfo.shuriken_count > 0) {
        return 10;
    } 
    if(item < 11 && playerinfo.redbull_count > 0) {
        return 11;
    } 
    return -1;
}
int prev_item(int item) {
    if(item == 11) {
        if(playerinfo.shuriken_count > 0) {
            return 10;
        }
        item = 10;
    }
    for(int i = item-1; i >= 0; i--) {
        if(playerinfo.item_mask & (1 << i)) {
            return i;
        }
    }
    return -1;
}

unsigned int screen_inventory(void) {
    display_clear();
    if(playerinfo.item_mask == 0 && playerinfo.shuriken_count == 0 
            && playerinfo.redbull_count == 0) {
        largefont_write("NO ITEMS!", 9, 32, 29);
        display_refresh();
        chill(
                CHILL_TIMEOUT | CHILL_BUTTON,
                3000000,
                NULL);
        return SCREEN_MAIN;
    }
    unsigned int item = next_item(-1);
    unsigned int width, height;
    while(1) {
        char buf[512];
        int prev = prev_item(item), next = next_item(item);
        if(prev != -1) {
            display_copy((char *)leftarrow, 0, 0, 0, 23, 10, 17);
        }
        if(next != -1) {
            display_copy((char *)rightarrow, 0, 0, 118, 23, 10, 17);
        }
        ext_load_bitmap(items[item].image, buf, sizeof(buf), &width, &height);
        display_copy(buf, 0, 0, 10, 10, width, height);
        erase_bottom();
        erase_top();
        unsigned int len = strlen(items[item].name);
        memcpy(buf, items[item].name, len);
        for(unsigned int i = 0; i < len; i++) {
            buf[i] |= 0x80;
        }
        smallfont_write(buf, len, 64-((SMALLFONT_WIDTH*len)/2), 3);
        len = strlen(items[item].description1);
        smallfont_write(items[item].description1, len, 8+width, 25);
        display_copy((char *)buttona, 0, 0, 54, 55, 7, 7);
        len = strlen(items[item].description2);
        if(len > 0) {
            smallfont_write(items[item].description2, len, 8+width, 34);
        }
        char ok[] = { INV('O'), INV('K') };
        smallfont_write(ok, 2, 63, 56);
        display_refresh();
        unsigned int retval = chill(
                CHILL_TIMEOUT | CHILL_BUTTON,
                5000000,
                NULL);
        if(retval == RET_BUTTON) {
            if(buttons_down & (1 << BUTTON_B)) {
                return SCREEN_MAIN;
            }
            if(buttons_down & (1 << BUTTON_LEFT) && prev != -1) {
                item = prev;
            }
            if(buttons_down & (1 << BUTTON_RIGHT) && next != -1) {
                item = next;
            }
            if(buttons_down & (1 << BUTTON_DOWN) && item < 10 && badgeinfo.is_ninja == 1) {
                globals.togrant = item+1;
                globals.giveitem = 1;
                return SCREEN_ENEMY_SELECT;
            }
        }
        if(retval == RET_TIMEOUT) {
            return SCREEN_MAIN;
        }
        display_clear();
    }
    return SCREEN_MAIN;
}

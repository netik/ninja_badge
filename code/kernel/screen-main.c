#include <mc1322x.h>
#include <board.h>
#include <ext.h>
#include <pinio.h>
#include <pinout.h>
#include <buzzer.h>
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
#include <screen-main.h>
#include "mainskel.h"
#include <info.h>

void redraw(unsigned char *optionbuf, unsigned int option, unsigned int flipped) {
    display_clear();

    char buf[2];
    memcpy(display_fbuf, mainskel, sizeof(display_fbuf));

    number_format(playerinfo.player_level, buf, 2);
    largefont_write(buf, 2, 34, 28);
    for(int i = 0; i < (playerinfo.hp*40/1337); i++) {
        unsigned char c = display_fbuf[128*6 + i];
        c |= 0xc0;
        display_fbuf[128*6 + i] = c;
    }
    unsigned int nwlen = stringlen((char *)playerinfo.name, 9);
    if(nwlen > 0) {
        largefont_write((char *)playerinfo.name, nwlen, 125-(nwlen*7), 3);
    }
    optionbuf[0] = optionbuf[8] = optionbuf[19] = INV(' ');
    switch(option) {
        case 0:
            optionbuf[0] = INV('>');
            break;
        case 1:
            optionbuf[8] = INV('>');
            break;
        default:
            optionbuf[19] = INV('>');
            break;
    }
    number_format(calc_spirit(playerinfo.player_level, playerinfo.item_mask), buf, 2);
    smallfont_write(buf, 2, 86, 39);
    number_format(calc_defense(playerinfo.player_level, playerinfo.item_mask), buf, 2);
    smallfont_write(buf, 2, 86, 46);
    number_format(calc_agility(playerinfo.player_level, playerinfo.item_mask), buf, 2);
    smallfont_write(buf, 2, 116, 39);
    number_format(calc_strength(playerinfo.player_level, playerinfo.item_mask), buf, 2);
    smallfont_write(buf, 2, 116, 46);
    number_format(playerinfo.redbull_count, buf, 2);
    smallfont_write(buf, 2, 64, 18);
    number_format(playerinfo.shuriken_count, buf, 2);
    smallfont_write(buf, 2, 64, 28);
    smallfont_write((char *)optionbuf, 25, 2, 56);
    if(flipped) {
        display_refresh_flipped();
    } else {
        display_refresh();
    }
}

const unsigned int sequence[] = {
    (1 << BUTTON_UP),
    (1 << BUTTON_UP),
    (1 << BUTTON_DOWN),
    (1 << BUTTON_DOWN),
    (1 << BUTTON_LEFT),
    (1 << BUTTON_RIGHT),
    (1 << BUTTON_LEFT),
    (1 << BUTTON_RIGHT),
    (1 << BUTTON_B),
    (1 << BUTTON_A),
};
    
#define FLIP_TIME 60000000
unsigned int screen_main(void) {
    unsigned int seqpos = 0;
    unsigned int flipped = 0;
    u_int32_t lasttouch = clockus;
    DEBUG_UI("XXX main screen\r\n");
    if(playerinfo.item_mask == 0) {
        setPin(LED_PWM, 0);
        pinPullupEnable(LED_PWM, 1);
        pinPullupSelect(LED_PWM, PIN_PULLDOWN);
        *TMR_ENBL &= ~(1 << 1);
    } else {
        pinPullupEnable(LED_PWM, 0);
        pinPullupSelect(LED_PWM, PIN_PULLDOWN);
        setPin(LED_PWM, 1);
        *TMR_ENBL &= ~(1 << 1);
        *TMR1_LOAD = 0;
        *TMR1_CNTR = 0;
        *TMR1_SCTRL = 0x5;
        *TMR1_COMP1 = 50000;
        *TMR1_CTRL = 0x3006;
        *TMR_ENBL |= (1 << 1);

        shift1((playerinfo.item_mask & 0x300) >> 2);
        shift1((playerinfo.item_mask & 0xff));
    }

    if(badgeinfo.is_boss && badgeinfo.is_ninja == 0) {
        DEBUG_UI("XXX: setting hp to 1337, we're a boss\r\n");
        playerinfo.hp = 1337;
    }
    unsigned int option = 0;
    unsigned int lastregen = clockus, regen = 0;
    unsigned char optionbuf[] = {
        INV(' '), INV('A'), INV('T'), INV('T'), INV('A'), INV('C'), INV('K'), INV(' '), 
        INV(' '), INV('I'), INV('N'), INV('V'), INV('E'), INV('N'), INV('T'), INV('O'), INV('R'), INV('Y'), INV(' '),  // starts at 8
        INV(' '), INV('S'), INV('E'), INV('T'), INV('U'), INV('P') };     // starts at 19

    redraw(optionbuf, option, flipped);

    while(1) {
        packet_t packet;

        unsigned int retval = chill(
                (CHILL_NOTIFY | CHILL_FIGHT | CHILL_STARTFIGHT | CHILL_BUTTON | CHILL_TIMEOUT),
                500000,
                &packet);
        if(retval == RET_PACKET) {
            switch(packet.data[1]) {
                case PACKET_TYPE_STARTFIGHT:
                    flipped = 0;
                    lasttouch = clockus;
                    redraw(optionbuf, option, flipped);
                    globals.was_attacked = 1;
                    memcpy((packet_t *)&globals.startfight, &packet, sizeof(packet_t));
                    return SCREEN_FIGHT; 
                    break;
                case PACKET_TYPE_GRANT:
                    {
                        rfio_grant *rg = (rfio_grant *)packet.data;
                        if(rg->item == 10 && rg->itemnonce == badgeinfo.levelnonce) {
                            if(playerinfo.player_level < 10) {
                                playerinfo.player_level++;
                                playerinfo.xp = base_xp[playerinfo.player_level-1];
                                info_commit_playerinfo();
                                display_levelup();
                                play_victory_tone();
                                display_refresh();
                                usleep(2500000);
                            }
                        } else if(rg->item > 9 || (playerinfo.item_mask & (1 << rg->item))) {
                        } else {
                            if(rg->itemnonce == badgeinfo.itemnonce[rg->item]) {
                                globals.togrant = rg->item;
                                return SCREEN_GRANT;
                            }
                        }
                    }
                    break;
                default:
                    DEBUG_UI("unknown packet type received by main screen!\r\n");
                    print_packet(&packet);
                    break;
            }
        }
        if(retval == RET_TIMEOUT) {
            if(1 == 2 && rand() % 10 == 0 && nearbys[0].level != 0) {
                putstr("attacking\r\n");
                globals.was_attacked = 0;
                globals.opponent = nearbys[0].badgeid;
                return SCREEN_FIGHT;
            }
        }
        if(playerinfo.hp < 1337 && (clockus - lastregen) > 1000000) {
            playerinfo.hp += 25;
            if(regen < 15) {
                regen++;
                playerinfo.hp += (2*calc_spirit(playerinfo.player_level, playerinfo.item_mask));
            }
            lastregen = clockus;
            if(playerinfo.hp > 1337) {
                playerinfo.hp = 1337;
            }
            flipped = 0;
            lasttouch = clockus;
            redraw(optionbuf, option, flipped);
        }
        if(retval == RET_BUTTON) {
            flipped = 0;
            lasttouch = clockus;
            if(sequence[seqpos] == buttons_down) {
                if(seqpos == sizeof(sequence)/sizeof(unsigned int)-1) {
                    return SCREEN_CRYPTIX;
                }
                seqpos++;
            } else {
                seqpos = 0;
            }
            if(buttons_down & (1 << BUTTON_LEFT)) {
                if(option == 0) {
                    option = 2;
                } else {
                    option--;
                }
                redraw(optionbuf, option, flipped);
            } else if(buttons_down & (1 << BUTTON_RIGHT)) {
                option = (option + 1) % 3;
                redraw(optionbuf, option, flipped);
            } else if(buttons_down & (1 << BUTTON_A)) {
                switch(option) {
                    case 0:
                        return SCREEN_ENEMY_SELECT;
                        break;
                    case 1:
                        return SCREEN_INVENTORY;
                        break;
                    case 2:
                        return SCREEN_SETUP;
                        break;
                }
            }
            DEBUG_UI("XXX hit button\r\n");
        }
        if(flipped == 0 && (clockus - lasttouch) > FLIP_TIME) {
            flipped = 1;
            redraw(optionbuf, option, flipped);
        }
    }
    return SCREEN_MAIN; /* NOTREACHED */
}

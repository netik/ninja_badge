#include <mc1322x.h>
#include <board.h>
#include <ext.h>
#include <pinio.h>
#include <pinout.h>
#include "config.h"
#include <display.h>
#include <misc.h>
#include "put.h"
#include <lock.h>
#include <fightskel.h>
#include <debug.h>
#include <attacked.h>
#include <task.h>
#include "syscalls.h"
#include <screen-fight.h>
#include <globals.h>
#include <info.h>
#include <rfio.h>
#include <buzzer.h>

void display_opponent_hp(unsigned int newhp) {
    DEBUG_UI("XXX Sfight: opponent hp now ");
    DEBUG32_UI(newhp);
    DEBUG_UI(NEWLINE);
    char buf[4];
    number_format(newhp, buf, 4);
    smallfont_write(buf, 4, 98, 2);
}
void display_hp(void) {
    DEBUG_UI("XXX Sfight: hp now ");
    DEBUG32_UI(playerinfo.hp);
    DEBUG_UI(NEWLINE);
    char buf[4];
    number_format(playerinfo.hp, buf, 4);
    smallfont_write(buf, 4, 10, 2);
}
unsigned int redbull_use(void) {
    if(playerinfo.item_mask & (1 << 4)) {
        return 267;
    }
    return 200;
}
unsigned int shuriken_use(unsigned int oppo_mask, unsigned int oppo_str) {
    unsigned int hithp;
    hithp = 201 + (((unsigned int)rand()) % oppo_str);
    unsigned int scale = 100;
    if(playerinfo.item_mask & (1 << 7)) {
        scale += 25;
    }
    if(oppo_mask & (1 << 6)) {
        scale -= 5;
    }
    return (hithp * scale) / 100;
}

void display_timedout(void) {
    DEBUG_UI("XXX Sfight: other player timed out\r\n");
    char msg[] = {
        INV('T'), INV('I'), INV('M'), INV('E'), INV('D'), INV(' '), 
        INV('O'), INV('U'), INV('T')};
    largefont_write(msg, 9, 31, 34);
}
void box(unsigned int x, unsigned int y, char *buf, unsigned int len) {
    display_copy((char *)msgbox, 0, 0, x, y, 34, 11);
    smallfont_write(buf, len, x+3, y+3);
    buf = buf;  //XXX remove
    len = len;  //XXX remove
}
void display_redbull_use(unsigned int boost) {
    char buf[5] = { '+', ' ', ' ', ' ', ' ' };
    number_format(boost, &buf[1], 4);
    box(2, 32, buf, 5);
}
void display_shuriken_damage(unsigned int damage) {
    char buf[5] = { '-', ' ', ' ', ' ', ' ' };
    number_format(damage, &buf[1], 4);
    box(2, 32, buf, 5);

    DEBUG_UI("XXX Sfight: got hit by shuriken for ");
    DEBUG32_UI(damage);
    DEBUG_UI(NEWLINE);
}
void display_opponent_redbull(unsigned int boost) {
    char buf[5] = { '+', ' ', ' ', ' ', ' ' };
    number_format(boost, &buf[1], 4);
    box(92, 32, buf, 5);

    DEBUG_UI("XXX Sfight: opponent used redbull for ");
    DEBUG32_UI(boost);
    DEBUG_UI(NEWLINE);
}
void display_opponent_crithit_damage(unsigned int damage) {
    char buf[5] = { ' ', ' ', ' ', ' ', '!' };
    number_format(damage, buf, 4);
    box(92, 16, buf, 5);

    DEBUG_UI("XXX Sfight: opponent got critical hit for ");
    DEBUG32_UI(damage);
    DEBUG_UI(NEWLINE);
}
void display_opponent_damage(unsigned int damage) {
    char buf[5] = { ' ', ' ', ' ', ' ', ' ' };
    number_format(damage, buf, 4);
    box(92, 16, buf, 5);

    DEBUG_UI("XXX Sfight: opponent got hit for ");
    DEBUG32_UI(damage);
    DEBUG_UI(NEWLINE);
}
void display_crithit_damage(unsigned int damage) {
    char buf[5] = { ' ', ' ', ' ', ' ', '!' };
    number_format(damage, buf, 4);
    box(2, 16, buf, 5);

    DEBUG_UI("XXX Sfight: got critical hit for ");
    DEBUG32_UI(damage);
    DEBUG_UI(NEWLINE);
}
void display_hit_damage(unsigned int damage) {
    char buf[4] = { ' ', ' ', ' ', ' ', };
    number_format(damage, buf, 4);
    box(2, 16, buf, 4);

    DEBUG_UI("XXX Sfight: got hit for ");
    DEBUG32_UI(damage);
    DEBUG_UI(NEWLINE);
}
void display_shuriken_use(unsigned int shurdmg) {
    char buf[5] = { '-', ' ', ' ', ' ', ' ' };
    number_format(shurdmg, &buf[1], 4);
    box(92, 32, buf, 5);

    DEBUG_UI("XXX Sfight: used shuriken for ");
    DEBUG32_UI(shurdmg);
    DEBUG_UI(" hp\r\n");
}

void send_yougo(u_int32_t opponent, packet_t *packet) {
    memset(packet, 0, sizeof(packet));
    packet->length = sizeof(rfio_yougo);
    rfio_yougo *ry = (rfio_yougo *)packet->data;
    ry->packettype = PACKET_TYPE_YOUGO;
    sendpacket(opponent, packet, 3);
}

void send_imdead(u_int32_t opponent, packet_t *packet) {
    memset(packet, 0, sizeof(packet));
    packet->length = sizeof(rfio_imdead);
    rfio_imdead *ri = (rfio_imdead *)packet->data;
    ri->packettype = PACKET_TYPE_IMDEAD;
    sendpacket(opponent, packet, 2);
}

unsigned int screen_fight(void) {
    save_playerinfo();
    unsigned int going_first;
    u_int32_t opponent;
    packet_t packet, rxpacket;
    memset(&rxpacket, 0, sizeof(rxpacket));
    memset(&packet, 0, sizeof(packet));
    unsigned int did_dodge = 0, used_pebble = 0;
    unsigned int oppo_str, oppo_def, oppo_agi, oppo_isninja, oppo_items,
                 oppo_level, oppo_isboss, oppo_hp, oppo_seq;
    char oppo_name[16];
    memset(oppo_name, 0, sizeof(oppo_name));
    if(globals.was_attacked) {
        DEBUG_UI("XXX Sfight: being attacked\r\n");
        if(globals.startfight.length < sizeof(rfio_startfight)) {
            DEBUG_UI("too small to be a startfight!\r\n");
            return SCREEN_MAIN;
        }
        volatile rfio_startfight *rs = (volatile rfio_startfight *)&globals.startfight.data;
        oppo_seq = rs->seqno;
        if(globals.startfight.length < (sizeof(rfio_startfight) + rs->namelen)) {
            DEBUG_UI("too small to be startfight+name!\r\n");
            return SCREEN_MAIN;
        }
        opponent = rs->badgeid;
        oppo_hp = rs->hp;
        oppo_level = (rs->playerlvl > 0 && rs->playerlvl < 11) ? rs->playerlvl : 1;
        if(badgeinfo.is_boss) {
            if(badgeinfo.is_ninja == 1) {
                playerinfo.player_level = 10;
            } else {
                playerinfo.player_level = badgeinfo.is_boss;
            }
            if(
                (badgeinfo.is_boss == 6 && oppo_level < 4)
                || (badgeinfo.is_boss == 7 && oppo_level < 5)
                || (badgeinfo.is_boss == 9 && oppo_level < 9)
                || (badgeinfo.is_boss == 10 && oppo_level < 10)
                || (badgeinfo.is_boss > 0 && badgeinfo.is_boss < 4)
              ) {
                DEBUG_UI("doesn't meet min level requirement!\r\n");
                return SCREEN_MAIN;
            }
        }
        oppo_items = rs->item_mask;
        oppo_isninja = rs->is_ninja;
        oppo_isboss = rs->is_boss;
        memcpy(oppo_name, (char *)rs->name, rs->namelen);
        globals.now_fighting = 1;
        globals.opponent = opponent;

        unsigned int len = stringlen((char *)playerinfo.name, sizeof(playerinfo.name));
        unsigned int packetsz = sizeof(rfio_startfight)+len;
        packet.length = packetsz;
        rfio_startfight *response = (rfio_startfight *)&packet.data;
        response->packettype = PACKET_TYPE_STARTFIGHTACK;
        response->playerlvl = playerinfo.player_level;
        response->namelen = len;
        memcpy(&(response->name), (void *)playerinfo.name, len);
        response->hp = playerinfo.hp;
        response->various = 0;
        response->shuriken_count = playerinfo.shuriken_count;
        response->redbull_count = playerinfo.redbull_count;
        response->item_mask = playerinfo.item_mask;
        response->is_ninja = badgeinfo.is_ninja;
        response->is_boss = badgeinfo.is_boss;

        sendpacket(opponent, &packet, 2);
        play_attacked_tone();
        if(badgeinfo.is_boss) {
            DEBUG_UI("XXX Sfight: we're a boss.  going immediately\r\n");
            usleep(1500000);
            did_dodge = 0;
        } else {
            display_copy((char *)attacked, 0, 0, 15, 8, 96, 48);
            smallfont_write(oppo_name, rs->namelen > 9 ? 9 : rs->namelen, 59, 29);
            display_refresh();
            DEBUG_UI("XXX Sfight: sent ack, waiting to see if dodged\r\n");
            unsigned int retval = chill(
                    (CHILL_BUTTON | CHILL_TIMEOUT),
                    1000000,
                    &rxpacket);
            if(retval == RET_BUTTON && buttons_down & (1 << BUTTON_B)) {
                play_dodge_tone();
                DEBUG_UI("XXX Sfight: B pressed, dodging\r\n");
                did_dodge = 1;
            }
        }
        going_first = 1;
    } else {
        oppo_items = 0;     // shh
        oppo_level = 1;
        oppo_seq = 0;
        oppo_hp = 0;
        oppo_isboss = 0;
        oppo_isninja = 0;
        opponent = globals.opponent;
        going_first = 0;
        DEBUG_UI("XXX starting fight; sending startfight\r\n");
        unsigned int len = stringlen((char *)playerinfo.name, sizeof(playerinfo.name));
        unsigned int packetsz = sizeof(rfio_startfight)+len;
        packet.length = packetsz;
        rfio_startfight *request = (rfio_startfight *)&packet.data;
        request->packettype = PACKET_TYPE_STARTFIGHT;
        request->playerlvl = playerinfo.player_level;
        request->namelen = len;
        memcpy(&(request->name), (void *)playerinfo.name, len);
        request->hp = playerinfo.hp;
        //packet.syncup = 1;
        request->various = 0;
        request->shuriken_count = playerinfo.shuriken_count;
        request->redbull_count = playerinfo.redbull_count;
        request->item_mask = playerinfo.item_mask;
        request->is_ninja = badgeinfo.is_ninja;
        request->is_boss = badgeinfo.is_boss;
        globals.now_fighting = 1;
        sendpacket(opponent, &packet, 2);

        largefont_write("ATTACKING", 9, 56, 44);
        display_refresh();

        DEBUG_UI("XXX waiting for ack\r\n");
        unsigned int i, p;
        for(p = i = 0; i < 5 && p < 50; ) {
            unsigned int retval = chill(
                    (CHILL_STARTFIGHT | CHILL_FIGHT | CHILL_TIMEOUT),
                    6000000,
                    &rxpacket);
            if(retval == RET_PACKET 
                    && rxpacket.length >= (sizeof(rfio_startfight)+1)
                    && rxpacket.data[1] == PACKET_TYPE_STARTFIGHTACK) {
                rfio_startfight *rack = (rfio_startfight *)&rxpacket.data;
                if((rxpacket.length < sizeof(rfio_startfight)+rack->namelen) 
                        || (rack->badgeid != opponent)) {
                    DEBUG_UI("XXX got invalid ack packet\r\n");
                    continue;
                } else if(retval == RET_PACKET) {
                    DEBUG_UI("XXX Sfight got unknown packet: ");
                    DEBUG32_UI(rxpacket.data[1]);
                    DEBUG_UI(NEWLINE);
                }
                oppo_seq = rack->seqno;
                oppo_hp = rack->hp;
                oppo_isninja = rack->is_ninja;
                oppo_isboss = rack->is_boss;
                oppo_items = rack->item_mask;
                oppo_level = (rack->playerlvl > 0 && rack->playerlvl < 11) ? rack->playerlvl : 1;
                memcpy(oppo_name, rack->name, rack->namelen);
                break;
            } else if(retval == RET_PACKET) {
                p++;
            } else {
                i++;
                if(i > 2) {
                    sendpacket(opponent, &packet, 2);
                }
            }
        }
        DEBUG_UI("XXX ZUH i");
        DEBUG32_UI(i);
        DEBUG_UI(" p ");
        DEBUG32_UI(p);
        DEBUG_UI(NEWLINE);
        if(i == 5 || p == 30) {
            DEBUG_UI("XXX: didn't receive a STARTFIGHTACK, exiting\r\n");
            globals.now_fighting = 0;
            return SCREEN_MAIN;
        }
        DEBUG_UI("XXX received ack; waiting for fight to start\r\n");
    }
    oppo_str = calc_strength(oppo_level, oppo_items);
    oppo_def = calc_defense(oppo_level, oppo_items);
    oppo_agi = calc_agility(oppo_level, oppo_items);
    int enemy, whoami;
    if(oppo_isboss != 0) {
        enemy = get_enemy_id(oppo_isboss);
    } else if(oppo_isninja != 0) {
        enemy = ENEMY_RIGHTSENSEI;
    } else {
        enemy = ENEMY_RIGHTNINJA;
    }
    if(badgeinfo.is_ninja != 0) {
        whoami = ENEMY_SENSEI;
    } else if(badgeinfo.is_boss) {
        whoami = get_enemy_id(badgeinfo.is_boss);
    } else {
        whoami = ENEMY_NINJA;
    }

    DEBUG_UI("XXX fight now starting\r\n");
    unsigned int timedout = 0, dead = 0, won = 0, cancancel = 0;
    unsigned int oppo_namelen = stringlen(oppo_name, 9);
    char buf[512];
    display_clear();
    memcpy(display_fbuf, fightskel, sizeof(display_fbuf));
    largefont_write((char *)playerinfo.name, stringlen((char *)playerinfo.name, 9), 0, 44);
    largefont_write(oppo_name, oppo_namelen, (121-(7*oppo_namelen)), 44);
    unsigned int width, height;
    ext_load_bitmap(enemyinfo[enemy].fighting, buf, sizeof(buf), &width, &height);
    display_copy(buf, 0, 0, 127-width+1, 7, width, height);
    ext_load_bitmap(enemyinfo[whoami].fighting, buf, sizeof(buf), &width, &height);
    display_copy(buf, 0, 0, 0, 7, width, height);
    while(!timedout) {
        unsigned int redbull = 0, shuriken = 0, packetreceived = 0;
        display_hp();
        display_opponent_hp(oppo_hp);
        if(cancancel == 0 && playerinfo.shuriken_count > 0) {
            display_copy((char *)buttona, 0, 0, 11, 55, 7, 7);
            char smsg[] = {
                INV('S'), INV('H'), INV('U'), INV('R'), INV('I'), INV('K'), INV('E'), INV('N'), 
            };
            smallfont_write(smsg, 8, 20, 56);
        }
        if(cancancel == 0 && playerinfo.redbull_count > 0) {
            display_copy((char *)buttonb, 0, 0, 68, 55, 7, 7);
            char rmsg[] = {
                INV('R'), INV('E'), INV('D'), INV('B'), INV('U'), INV('L'), INV('L'), 
            };
            smallfont_write(rmsg, 7, 77, 56);
        }
        if(cancancel == 1) {
            display_copy((char *)buttonb, 0, 0, 64, 55, 7, 7);
            char rmsg[] = {
                INV('C'), INV('A'), INV('N'), INV('C'), INV('E'), INV('L'), 
            };
            smallfont_write(rmsg, 6, 73, 56);
        }
        display_refresh();

        // If we're the attackee, we "go first".  Either send a YOUGO or 
        // a hit.
        if(going_first == 0) {
            // Wait for the hit/dead/yougo from the other player.  Catch any shuriken/
            // redbull button presses and save them for next round.
            u_int32_t now = clockus;
            u_int32_t endtime = INFIGHT_RECEIVE_TIMEOUT;
            u_int32_t next_rt = INFIGHT_RECEIVE_TIMEOUT/4;
                       
            while(packetreceived == 0 && timedout == 0) {
                unsigned int retval = chill(
                        (CHILL_BUTTON | CHILL_STARTFIGHT 
                         | CHILL_FIGHT | CHILL_TIMEOUT),
                        INFIGHT_RECEIVE_TIMEOUT/4,
                        &rxpacket);
                if(retval == RET_BUTTON && ((cancancel == 1) || (shuriken == 0 && redbull == 0))) {
                    if(cancancel == 0 && buttons_down & (1 << BUTTON_A) && playerinfo.shuriken_count > 0) {
                        erase_bottom();
                        display_refresh();
                        playerinfo.shuriken_count--;
                        shuriken = shuriken_use(oppo_items, oppo_str);
                    }
                    if(buttons_down & (1 << BUTTON_B) && cancancel == 1) {
                        timedout = 1;
                        break;
                    } else if(buttons_down & (1 << BUTTON_B) && playerinfo.redbull_count > 0) {
                        erase_bottom();
                        display_refresh();
                        playerinfo.redbull_count--;
                        redbull = redbull_use();
                    }
                } else if(retval == RET_PACKET && rxpacket.length >= sizeof(rfio_yougo)) {
                    if(cancancel == 1) {
                        cancancel = 0;
                        erase_bottom();
                    }
                    display_refresh();
                    unsigned int pkttype = rxpacket.data[1];
                    if(pkttype == PACKET_TYPE_YOUGO) {
                        rfio_yougo *ry = (rfio_yougo *)rxpacket.data;
                        if(ry->badgeid == opponent) {
                            if(ry->seqno == oppo_seq) {
                                DEBUG_RFIO("saw dup @ yougo\r\n");
                                endtime += INFIGHT_RECEIVE_TIMEOUT;
                            } else {
                                oppo_seq = ry->seqno;
                                packetreceived = 1;
                            }
                        }
                    } else if(pkttype == PACKET_TYPE_IMDEAD) {
                        rfio_imdead *ri = (rfio_imdead *)rxpacket.data;
                        if(ri->badgeid == opponent) {
                            won = 1;
                            packetreceived = 1;
                        }
                    } else if(pkttype == PACKET_TYPE_FIGHTTURN 
                            && rxpacket.length >= sizeof(rfio_fightturn)) {
                        rfio_fightturn *rf = (rfio_fightturn *)rxpacket.data;
                        if(rf->badgeid == opponent) {
                            if(rf->seqno == oppo_seq) {
                                DEBUG_RFIO("saw dup @ fight\r\n");
                            } else {
                                ext_load_bitmap(enemyinfo[enemy].attacking, buf, sizeof(buf), &width, &height);
                                display_copy(buf, 0, 0, 127-width+1, 7, width, height);
                                oppo_seq = rf->seqno;
                                oppo_hp = rf->myhp;
                                display_opponent_hp(rf->myhp);
                                if(rf->shuriken > 0) {
                                    display_shuriken_damage(rf->shuriken);
                                }
                                if(rf->rbboost > 0) {
                                    display_opponent_redbull(rf->rbboost);
                                }
                                if(rf->crit_hit) {
                                    ext_load_bitmap(enemyinfo[whoami].hit, buf, sizeof(buf), &width, &height);
                                    display_copy(buf, 0, 0, 0, 7, width, height);
                                    display_crithit_damage(rf->hithp);
                                    play_hit_tone();
                                    play_hit_tone();
                                } else if(rf->dodged) {
                                    ext_load_bitmap(enemyinfo[whoami].dodging, buf, sizeof(buf), &width, &height);
                                    display_copy(buf, 0, 0, 0, 7, width, height);
                                    play_dodge_tone();
                                } else {
                                    ext_load_bitmap(enemyinfo[whoami].hit, buf, sizeof(buf), &width, &height);
                                    display_copy(buf, 0, 0, 0, 7, width, height);
                                    display_hit_damage(rf->hithp);
                                    play_hit_tone();
                                }
                                u_int16_t damage = (rf->hithp+rf->shuriken);
                                if(damage >= playerinfo.hp) {
                                    dead = 1;
                                } else {
                                    playerinfo.hp -= damage;
                                    display_hp();
                                }
                                packetreceived = 1;
                            }
                        } else {
                            DEBUG_UI("XXX: not opponent!\r\n");
                        }
                    }
                }
                if(packetreceived == 0 && clockus - now > next_rt) {
                    cancancel = 1;
                    if(cancancel == 1) {
                        erase_bottom();
                        display_copy((char *)buttonb, 0, 0, 64, 55, 7, 7);
                        char rmsg[] = {
                            INV('C'), INV('A'), INV('N'), INV('C'), INV('E'), INV('L'), 
                        };
                        smallfont_write(rmsg, 6, 73, 56);
                        display_refresh();
                    }

                    sendpacket(opponent, &packet, 2);
                    next_rt = clockus+INFIGHT_RECEIVE_TIMEOUT/4;
                }
                if(clockus - now > endtime) {
                    timedout = 1;
                }
            }
        }
        display_refresh();
        if(timedout || won || dead) {
            break;
        }
        if(going_first && did_dodge == 0) {
            send_yougo(opponent, &packet);
            did_dodge = 1;
        } else { 
            // Wait for redbulls/shuriken button presses.  
            unsigned int retval = chill(CHILL_BUTTON | CHILL_TIMEOUT,
                    ITEM_USE_DELAY, &rxpacket);
            if(retval == RET_BUTTON && shuriken == 0 && redbull == 0) {
                if(buttons_down & (1 << BUTTON_A) && playerinfo.shuriken_count > 0) {
                    erase_bottom();
                    display_refresh();
                    playerinfo.shuriken_count--;
                        shuriken = shuriken_use(oppo_items, oppo_str);
                }
                if(buttons_down & (1 << BUTTON_B) && playerinfo.redbull_count > 0) {
                    erase_bottom();
                    display_refresh();
                    playerinfo.redbull_count--;
                    redbull = redbull_use();
                }
            }
            memset(&packet, 0, sizeof(packet));
            packet.length = sizeof(rfio_fightturn);
            rfio_fightturn *rf = (rfio_fightturn *)packet.data;
            rf->packettype = PACKET_TYPE_FIGHTTURN;
            ext_load_bitmap(enemyinfo[whoami].attacking, buf, sizeof(buf), &width, &height);
            display_copy(buf, 0, 0, 0, 7, width, height);

            /* $p = (int rand 100) + 1;
             * return if (100 * $p < 100 * 1/2 * $defender_agi);
             * if (100 * $p >= 100 * (100 - ($attacker_agi)) {
             *     $hit = 20 + (int rand $attacker_str + 1);
             * } else {
             *     for (1..10) {
             *         $hit+= (int rand 20) + 1;
             *     }
             *     $hit += $attacker_str;
             * }
             * $hit -= (int rand $defender_def) + 1;
             * $defender_hp -= ($hit < 0 ? 0 : $hit);
             */
            const unsigned int p = (((unsigned int)rand()) % 100) + 1;
            unsigned int hithp;
            unsigned int dodge_const = 100;
            if(oppo_items & (1 << 5)) {
                dodge_const -= 15;  // this isn't exactly 10%, it's more generous overall
            }
            if(((oppo_items & (1 << 8)) && (used_pebble == 0))
                    || ((dodge_const*p) < (50*oppo_agi))) {
                hithp = 0;
                rf->dodged = 1;
                used_pebble = 1;
            } else if((100*p) >= (100 * (100 - oppo_agi))) {
                rf->crit_hit = 1;
                hithp = 201 + (((unsigned int)rand()) % oppo_str);
                if(oppo_items & (1 << 9)) {
                    hithp = (hithp * 115) / 100;
                }
            } else {
                hithp = 0;
                for(unsigned int j = 0; j < 10; j++) {
                    hithp += ((((unsigned int)rand()) % 20) + 1);
                }
                hithp += oppo_str;
            }
            unsigned int defoff = ((unsigned int)rand()) % oppo_def + 1;
            if(defoff > hithp) {
                hithp = 0;
            } else {
                hithp -= defoff;
            }
            if(oppo_items & (1 << 6)) {
                hithp = (hithp * 95) / 100;
            }
            rf->hithp = hithp;
            if(redbull) {
                rf->rbboost = redbull;
                playerinfo.hp += redbull;
                if(playerinfo.hp > 1337) {
                    playerinfo.hp = 1337;
                }
                if(whoami == ENEMY_NINJA) {
                    ext_load_bitmap(NINJA_REDBULL, buf, sizeof(buf), &width, &height);
                    display_copy(buf, 0, 0, 0, 7, width, height);
                } else if(whoami == ENEMY_SENSEI) {
                    ext_load_bitmap(SENSEI_REDBULL, buf, sizeof(buf), &width, &height);
                    display_copy(buf, 0, 0, 0, 7, width, height);
                }
                display_redbull_use(redbull);
                display_hp();
            }
            unsigned int dmg = hithp;
            if(shuriken) {
                rf->shuriken = shuriken;
                dmg += shuriken;
                if(whoami == ENEMY_NINJA) {
                    ext_load_bitmap(NINJA_SHURIKEN, buf, sizeof(buf), &width, &height);
                    display_copy(buf, 0, 0, 0, 7, width, height);
                } else if(whoami == ENEMY_SENSEI) {
                    ext_load_bitmap(SENSEI_SHURIKEN, buf, sizeof(buf), &width, &height);
                    display_copy(buf, 0, 0, 0, 7, width, height);
                }
            }
            DEBUG_UI("XXX damage: ");
            DEBUG32_UI(dmg);
            DEBUG_UI(" oppo_hp ");
            DEBUG32_UI(oppo_hp);
            DEBUG_UI(NEWLINE);
            oppo_hp = ((dmg > oppo_hp) ? 0 : (oppo_hp-dmg));
            rf->myhp = playerinfo.hp;
            sendpacket(opponent, &packet, 2);
            if(rf->dodged) {
                ext_load_bitmap(enemyinfo[enemy].dodging, buf, sizeof(buf), &width, &height);
                display_copy(buf, 0, 0, 127-width+1, 7, width, height);
                play_dodge_tone();
            } else if(rf->crit_hit) {
                ext_load_bitmap(enemyinfo[enemy].hit, buf, sizeof(buf), &width, &height);
                display_copy(buf, 0, 0, 127-width+1, 7, width, height);
                display_opponent_crithit_damage(hithp);
                play_hit_tone();
                play_hit_tone();
            } else {
                ext_load_bitmap(enemyinfo[enemy].hit, buf, sizeof(buf), &width, &height);
                display_copy(buf, 0, 0, 127-width+1, 7, width, height);
                display_opponent_damage(hithp);
                play_hit_tone();
            }
            if(shuriken) {
                display_shuriken_use(shuriken);
            }
            display_opponent_hp(oppo_hp);
        }
        redbull = 0;
        shuriken = 0;
        going_first = 0;
    }
    if(timedout) {
        display_timedout();
        restore_playerinfo();
    } else {
        erase_bottom();
        if(won) {
            ext_load_bitmap(enemyinfo[whoami].victory, buf, sizeof(buf), &width, &height);
            display_copy(buf, 0, 0, 0, 7, width, height);
            ext_load_bitmap(enemyinfo[enemy].ko, buf, sizeof(buf), &width, &height);
            display_copy(buf, 0, 0, 127-width+1, 7, width, height);
            char msg[] = {
                INV('Y'), INV('O'), INV('U'), INV(' '), INV('A'), INV('R'), INV('E'), 
                INV(' '), INV('V'), INV('I'), INV('C'), INV('T'), INV('O'), INV('R'), 
                INV('I'), INV('O'), INV('U'), INV('S'), INV('!')};
            smallfont_write(msg, 19, 20, 56);
            play_victory_tone();
        } else if(dead) {
            send_imdead(opponent, &packet);
            ext_load_bitmap(enemyinfo[whoami].ko, buf, sizeof(buf), &width, &height);
            display_copy(buf, 0, 0, 0, 7, width, height);
            ext_load_bitmap(enemyinfo[enemy].victory, buf, sizeof(buf), &width, &height);
            display_copy(buf, 0, 0, 127-width+1, 7, width, height);
            char msg[] = {
                INV('Y'), INV('O'), INV('U'), INV(' '), INV('W'), INV('E'), INV('R'), 
                INV('E'), INV(' '), INV('D'), INV('E'), INV('F'), INV('E'), INV('A'), 
                INV('T'), INV('E'), INV('D'), INV('!')};
            smallfont_write(msg, 18, 23, 56);
            if(badgeinfo.is_boss) {
                grant_item(opponent, &packet, badgeinfo.is_boss, 2);
            }
            play_defeat_tone();
        }
    }
    display_refresh();
    usleep(1500000);
    // DO THIS WHEN DONE
    globals.now_fighting = 0;
    if(won && (badgeinfo.is_boss == 0 || badgeinfo.is_ninja == 1)) {
        unsigned int xp;
        if(playerinfo.player_level > oppo_level) {
            if(playerinfo.player_level-oppo_level > 1) {
                xp = 50;
            } else {
                xp = 75;
            }
        } else if(oppo_level > playerinfo.player_level) {
            if(oppo_level - playerinfo.player_level > 1) {
                xp = 150;
            } else {
                xp = 125;
            }
        } else {
            xp = 100;
        }
        
        playerinfo.xp += xp;
        if(calc_level(playerinfo.xp) > playerinfo.player_level) {
            playerinfo.player_level = calc_level(playerinfo.xp);
            display_levelup();
            display_refresh();
            play_victory_tone();
            usleep(2500000);
        }
        unsigned int x = (unsigned int)rand();
        if((x >> 8) % 6 == 0) {
            if(((unsigned int)rand()) & 4) {
                globals.togrant = 10; 
            } else {
                globals.togrant = 11; 
            }
            return SCREEN_GRANT;
        }
    }
    info_commit_playerinfo();
    return SCREEN_MAIN;
}


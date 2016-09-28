#include <mc1322x.h>
#include <board.h>
#include <pinio.h>
#include <ext.h>
#include <rfio.h>
#include <timer.h>
#include <lock.h>
#include <misc.h>
#include <task.h>
#include <globals.h>
#include <debug.h>
#include <syscalls.h>
#include <put.h>
#include <info.h>

volatile u_int8_t seqno;
volatile u_int32_t current_listen_mask;
volatile packet_t *rfioqueue, *rfiotxqueue;
volatile u_int32_t rfioqueuecount, rfiotxqueuecount, rfiotxqueuecount;

#define RFIOQUEUE_MAX 16     // XXX: best policy?
#define RFIOTXQUEUE_MAX 8     // XXX: best policy?
nearby_user nearbys[NNEARBY];
lock_t nearbyslock, rfioqueuelock, rfiotxqueuelock;

int rfio_handle_packet(volatile packet_t *p);
unsigned int calc_next_tx(u_int32_t badgeid, u_int32_t *start);

unsigned int rfio_do_tx(volatile packet_t *p, unsigned int starttime, 
        unsigned int maxretries, unsigned int retryoffset, 
        unsigned int *finishedtime) {
    unsigned int retval = 0;

    p->maxretries = maxretries;
    p->curtry = 0;
    p->retryoffset = retryoffset;
    p->starttime = starttime;
    unsigned int txstatus = txpacket(p);
    if(txstatus == TXSTATUS_SUCCESS) {
        *finishedtime = p->finishedtime;
        retval = 1;
    }
    return retval;
}

u_int32_t rfio_txbeacon(u_int32_t starttime, u_int32_t rx_off_advertised,
        u_int32_t beacon_period) {
    volatile packet_t *p = get_free_packet();
    if(p == NULL) {
        DEBUG_RFIO("can't get free packet for beacon\r\n");
        return 0;
    }
    p->timestosend = 1;
    rfio_beacon *rb = (struct rfio_beacon *)p->data;
    rb->initbyte = 0;
    rb->mbo = 1;
    rb->packettype = PACKET_TYPE_BEACON;
    rb->seqno = seqno++;
    rb->badgeid = badgeinfo.badgeid;
    rb->rxoffset_scaled = (rx_off_advertised / 100);
    rb->period_scaled = (beacon_period / 100);
    rb->item_mask = playerinfo.item_mask;
    rb->is_boss = badgeinfo.is_boss;
    rb->is_ninja = badgeinfo.is_ninja;
    rb->playerlvl = playerinfo.player_level;
    unsigned int len = stringlen((char *)playerinfo.name, sizeof(playerinfo.name));
    rb->namelen = len;
    memcpy(rb->name, (void *)playerinfo.name, len);
    p->offset = 0;
    p->length = sizeof(rfio_beacon) + len;
    u_int32_t endtime = 0;
    rfio_do_tx(p, starttime, 3, 1000, &endtime);
    free_packet(p);
    return endtime;
}
#define INITIAL_TX   (100000)        // initial offset to start clock
#define INITIAL_PERIOD  (300000)    // beacon period 
#define INITIAL_RXLEN (75000)      // length of actual rx time
#define FIGHT_RXLEN (150000)      // length of actual rx time
#define INITIAL_RXOFF  (100000)     // max offset from tx
#define START_SHIFT 5000            // amount to shift start per-beacon
void rfio_init(void) {
    memset(nearbys, 0, sizeof(nearbys));
    current_listen_mask = 0;
    lock_init(&nearbyslock);
    lock_init(&rfioqueuelock);
    lock_init(&rfiotxqueuelock);
    lock_wait(&rfioqueuelock);
    rfioqueue = 0;
    rfioqueuecount = 0;
    lock_unlock(&rfioqueuelock);
    lock_wait(&rfiotxqueuelock);
    rfiotxqueue = 0;
    rfiotxqueuecount = 0;
    lock_unlock(&rfiotxqueuelock);
    seqno = 0;
}

void rfio_run(void) {
    set_channel(0); /* channel 11 */
    set_power(0x11); /* 0x11 = 3dbm, see 3-22 */
    u_int32_t starttime = *MACA_CLK + INITIAL_TX, beacon_period = INITIAL_PERIOD + (((unsigned int)rand()) % 100000),
              rxofftime = INITIAL_RXOFF;
    volatile unsigned int currxlen;
    while(1) {
        if(globals.now_fighting) {
            currxlen = FIGHT_RXLEN;
        } else {
            currxlen = INITIAL_RXLEN;
        }
        while(!info_isvalid()) {
            putstr("XXX: info not set yet!\r\n");
            usleep(1000000);
        }
        volatile packet_t *p;
        u_int32_t endtime = 0, currxoff;
        if((p = rfio_check_txqueue()) != NULL) {
            u_int32_t txstart = 0;
            if(calc_next_tx(p->destbadgeid, &txstart) == 0) {
                DEBUG32_RFIO(p->destbadgeid);
                DEBUG32_RFIO(nearbys[0].badgeid);
                DEBUG_RFIO("can't find badgeid in nearbys!\r\n");
                free_packet(p);
                continue;
            } else {
                u_int32_t txoffset = (txstart - starttime), advrx;
                if(txoffset >= rxofftime) {
                    advrx = beacon_period + rxofftime - txoffset;
                } else {
                    advrx = rxofftime - txoffset;
                }
                while((txstart - starttime) > beacon_period) {
                    starttime += beacon_period;
                }
                if(p->syncup == 1) {
                    putstr("------- XXX: syncing up\r\n");
                    // more magic numbers.  great
                    starttime = txstart-4096;
                }
                currxoff = advrx;
                advrx += (currxlen/4);  // we advertise later, but we start rx early
                rfio_yougo *ry = (rfio_yougo *)p->data; 
                ry->rxoffset_scaled = (advrx / 100); 
                ry->period_scaled = (beacon_period / 100);
                u_int32_t txthisat = txstart, lastendtime;
                while(p->timessent < p->timestosend) {
                    rfio_do_tx(p, txthisat, 3, 1000, &lastendtime);
                    if(p->timessent == 0) {
                        endtime = lastendtime;
                    }
                    // v. rough
                    ry->rxoffset_scaled -= (10 + (lastendtime - txthisat)/100);
                    txthisat = lastendtime + 2000;
                    p->timessent++;
                }
                free_packet(p);
            }
        } else {
            currxoff = rxofftime;
            if(starttime < *MACA_CLK) {
                starttime += beacon_period; // XXX: poor around wraps
            }
            if(globals.now_fighting == 0) {
                u_int32_t offset = ((u_int32_t)rand()) % (currxoff - 15000);
                endtime = rfio_txbeacon(starttime+offset, rxofftime-offset+(currxlen/3), beacon_period);
                currxoff -= offset;
            } else {
                endtime = *MACA_CLK + 100;
            }
        }
        // start the receive sequence
        unsigned int rxstart = endtime + currxoff;
        do {
            p = rfreceive(endtime + currxoff, currxlen);
            do {
                if(p == NULL) {
                    //putstr("done reading packets\r\n");
                } else {
                    if(rfio_handle_packet(p)) {
                        free_packet(p);
                    }
                    p = rfreceive(0, 0);
                }
            } while (p != NULL);
        } while (globals.now_fighting && (*MACA_CLK - rxstart) < currxlen);
        starttime += beacon_period;
        if(globals.now_fighting == 0) {
           starttime += (((unsigned int)rand() ^ badgeinfo.badgeid) % START_SHIFT);
        }
    }
}

int find_open_nearby(u_int32_t badgeid, u_int32_t rxfinishedtime) {
    int i;
    for(i = 0; i < NNEARBY; i++) {
        if(nearbys[i].badgeid == badgeid) {
            break;
        }
    }
    if(i == NNEARBY) {
        u_int32_t lowest_wrap = 0xffffffff, lowest_nowrap = 0xffffffff;
        int lowidx = -1, wrapidx = -1;
        /* Wrap handling is hackish.  Fortunately, they don't happen that often..  */
        for(int n = 0; n < NNEARBY; n++) {
            if(globals.now_fighting && globals.opponent == nearbys[n].badgeid) {
                // don't replace the guy we're fighting
                continue;
            }
            if(nearbys[n].is_boss != 0 && nearbys[n].is_boss != 9) {
                continue;
            }
            if(nearbys[n].lastseen > rxfinishedtime && lowest_wrap > nearbys[n].lastseen) {
                wrapidx = n;
                lowest_wrap = nearbys[n].lastseen;
            } else if(nearbys[n].lastseen < lowest_nowrap) {
                lowidx = n;
                lowest_nowrap = nearbys[n].lastseen;
            }
        }
        if(lowidx == -1 && wrapidx == -1) {
            i = 0;
        } else if(lowidx == -1) {
            i = wrapidx;
        } else if(wrapidx == -1) {
            i = lowidx;
        } else {
            if((lowest_wrap - rxfinishedtime) > (rxfinishedtime - lowest_nowrap)) {
                i = wrapidx;
            } else {
                i = lowidx;
            }
        }
    }
    return i;
}
void namecpy_terminate(char *dest, char *src, unsigned int len) {
    unsigned int c;
    for(c = 0; c < (len & 0xf); c++) {
        char x = src[c];
        if(IS_VALID_NAMECHAR(x)) {
            dest[c] = x;
        } else {
            dest[c] = '.';
        }
    }
    dest[c] = 0;
}

void rfio_nearby_add(volatile packet_t *p) {
    if(p->data[1] == PACKET_TYPE_BEACON) {
        struct rfio_beacon *rb = (struct rfio_beacon *)p->data;
        if(p->length < (sizeof(rfio_beacon) + rb->namelen)) {
            DEBUG_RFIO("name len too long! ignoring beacon\r\n");
            putstr("XXX len ");
            put_hex32(p->length);
            putstr(" ");
            put_hex32(rb->namelen);
            putstr(" ");
            put_hex32(sizeof(rfio_beacon));
            putstr("\r\n");
            print_packet(p);    // XXX

            return;
        }
        if(!lock_delay(&nearbyslock, 10000)) {
            DEBUG_RFIO("can't lock nearbyslock\r\n");
            return;
        }
        const u_int32_t bid = rb->badgeid;
        int i = find_open_nearby(bid, p->finishedtime);
        if(i == -1) {
            lock_unlock(&nearbyslock);
            return;
        }
        nearbys[i].badgeid = bid;
        nearbys[i].lastseen = p->finishedtime;
        nearbys[i].rxoffset = (rb->rxoffset_scaled * 100);
        nearbys[i].period = (rb->period_scaled * 100);
        nearbys[i].level = rb->playerlvl;
        namecpy_terminate(nearbys[i].name, rb->name, rb->namelen);
        nearbys[i].playerstate = rb->playerstate;
        nearbys[i].is_ninja = rb->is_ninja;
        nearbys[i].is_boss = rb->is_boss;
        nearbys[i].item_mask = rb->item_mask;
        //DEBUG_RFIO("XXX setting nearbys i ");
        //DEBUG32_RFIO(i);
        //DEBUG_RFIO(" lastseen 1: ");
        //DEBUG32_RFIO(nearbys[i].lastseen);
        //DEBUG_RFIO(" name ");
        //DEBUG_RFIO(nearbys[i].name);
        //DEBUG_RFIO(NEWLINE);
        lock_unlock(&nearbyslock);
    } else if((p->data[1] == PACKET_TYPE_STARTFIGHTACK) 
                || (p->data[1] == PACKET_TYPE_STARTFIGHT)) {
        struct rfio_startfight *rs = (struct rfio_startfight *)p->data;
        if(p->length < (sizeof(rfio_startfight) + rs->namelen)) {
            DEBUG_RFIO("name len too long in startfight packet\r\n");
            return;
        }
        if(!lock_delay(&nearbyslock, 10000)) {
            DEBUG_RFIO("can't lock nearbyslock\r\n");
            return;
        }
        const u_int32_t bid = rs->badgeid;
        int i = find_open_nearby(bid, p->finishedtime);
        nearbys[i].badgeid = bid;
        nearbys[i].lastseen = p->finishedtime;
        nearbys[i].rxoffset = (rs->rxoffset_scaled * 100);
        nearbys[i].period = (rs->period_scaled * 100);
        nearbys[i].level = rs->playerlvl;
        namecpy_terminate(nearbys[i].name, rs->name, rs->namelen);
        nearbys[i].is_ninja = rs->is_ninja;
        nearbys[i].is_boss = rs->is_boss;
        nearbys[i].item_mask = rs->item_mask;
        //DEBUG_RFIO("XXX setting nearbys i ");
        //DEBUG32_RFIO(i);
        //DEBUG_RFIO(" lastseen 2: ");
        //DEBUG32_RFIO(nearbys[i].lastseen);
        //DEBUG_RFIO(" name ");
        //DEBUG_RFIO(nearbys[i].name);
        //DEBUG_RFIO(NEWLINE);

        lock_unlock(&nearbyslock);
    }
}

int rfio_handle_packet(volatile packet_t *p) {
    if(p->length < 4) {
        DEBUG_RFIO("too-short packet length\r\n");
        return 1;
    }
    p->length--;
    for(int i = 0; i < p->length; i++) {
        p->data[i] = p->data[i+1];
    }
    unsigned char type = p->data[1];
    rfio_nearby_add(p);
    if(type == PACKET_TYPE_BEACON) {
        if(p->length < (sizeof(rfio_beacon) + 1)) {
            DEBUG_RFIO("invalid beacon length\r\n");
            return 1;
        }
    } else if(current_listen_mask != 0 && rfio_mask_matches(current_listen_mask, p)) {
        rfio_queue_add(p);
        chill_packet_notify();
        return 0;
    } else if(!IS_VALID_PACKET_TYPE(type)) {    // XXX remove
        DEBUG_RFIO("XXX unknown packet type\r\n");
        print_packet(p);
    } else {
        if(rfio_mask_matches(CHILL_NOTIFY, p)) {
            rfio_queue_add(p);
            return 0;
        } else {
            putstr("XXX dropping packet: ");
            print_packet(p);
        }
    }
    return 1;
}

void swi_chill_packet_notify(void) {
    task *ct = &tasks[TASK_UI];
    if(!(ct->rfio_waiting)) {
        return;
    }
    volatile packet_t *p = rfio_check_queue(current_listen_mask);
    if(p == NULL) {
        DEBUG_RFIO("chill packet notify: no packets that match mask!\r\n");
        return;
    }
    if(ct->packetbuf != NULL) {
        memcpy((void *)ct->packetbuf, (void *)p, sizeof(packet_t));
        free_packet(p);
        ct->packetbuf = NULL;
    } else {
        putstr("XXX: chilling for packet, but no packetbuf!\r\n");
    }
    if(ct->timer_waiting) {
        if(ct->tevent == NULL) {
            DEBUG_RFIO("chill packet notify: no tevent but waiting on timer!\r\n");
        }
        timer_cancel(ct->tevent);
        ct->tevent = NULL;
    }
    task_wakeup_chill(TASK_UI, RET_PACKET);
}
void swi_sendpacket(u_int32_t badgeid, packet_t *packet, unsigned int tries) {
    volatile packet_t *p = get_free_packet();
    if(p == NULL) {
        DEBUG_RFIO("can't get free packet for sendpacket\r\n");
        return;
    }
    if(packet->length < sizeof(rfio_yougo) || packet->length > MAX_PAYLOAD_SIZE) {
        DEBUG_RFIO("invalid length for sendpacket!\r\n");
        return;
    }
    memcpy((char *)p->data, packet->data, packet->length);
    p->length = packet->length;
    p->syncup = packet->syncup;
    p->timessent = 0;
    p->timestosend = tries;
    p->destbadgeid = badgeid; 
    const unsigned int type = packet->data[1];
    if((type == PACKET_TYPE_STARTFIGHT)
            || (type == PACKET_TYPE_STARTFIGHTACK)
            || (type == PACKET_TYPE_YOUGO)
            || (type == PACKET_TYPE_IMDEAD)
            || (type == PACKET_TYPE_GRANT)
            || (type == PACKET_TYPE_FIGHTTURN)) {
        rfio_yougo *ry = (rfio_yougo *)p->data;
        ry->initbyte = 0;
        ry->mbo = 1;
        ry->seqno = seqno++;
        ry->badgeid = badgeinfo.badgeid;
        ry->dest_badgeid = badgeid;
    }
    rfio_txqueue_add(p);
}

unsigned int swi_chill(u_int32_t mask, unsigned int timeout, volatile packet_t *packetbuf) {
    task *ct = &tasks[curtask];
    if(ct->wakeup_reason != 0) {
        DEBUG_TASK("XXX: task already waiting\r\n");
        ct->wakeup_reason = 0;
    }
    if(mask & (CHILL_NOTIFY | CHILL_STARTFIGHT | CHILL_FIGHT)) {
        volatile packet_t *p = rfio_check_queue(mask);
        if(p != NULL) {
            memcpy((void *)packetbuf, (void *)p, sizeof(packet_t));
            free_packet(p);
            return RET_PACKET;
        }
        rfio_set_mask(mask);
        ct->packetbuf = packetbuf;
        ct->rfio_waiting = 1;
    }
    if(mask & CHILL_BUTTON) {
        ct->button_waiting = 1;
    }
    if(mask & CHILL_TIMEOUT) {
        u_int32_t finaltime = (clockus + timeout) & 0xffffffff;
        tevent *tev = timer_add_wakeup(curtask, finaltime);
        ct->tevent = tev;
        ct->timer_waiting = 1;
    }
    task_clear_ready(curtask);
    task_reschedule();
    return RET_INVALID;     // ignored
}

void rfio_watchdog(void) {
    while(1) {
        usleep(500000);
        //usleep(100000);
        //DEBUG_RFIO("--- wd\r\n");
        check_maca();
        unsigned int mc = *MACA_CLK;
        if(lock_delay(&nearbyslock, 10000)) {
            for(int i = 0; i < NNEARBY; i++) {
                if(nearbys[i].level != 0 && (mc - nearbys[i].lastseen) > (300 * 1000000)) {
                    nearbys[i].level = 0;
                }
            }
            lock_unlock(&nearbyslock);
       }
    }
}

void rfio_set_mask(u_int32_t mask) {
    current_listen_mask = mask;
}

void rfio_txqueue_add(volatile packet_t *p) {
    if(!lock_delay(&rfiotxqueuelock, 10000)) {
        DEBUG_RFIO("XXX rfio_queue_add: can't lock rfiotxqueue!\r\n");
        free_packet(p);
        return;
    }
    if(rfiotxqueuecount >= RFIOTXQUEUE_MAX) {
        DEBUG_RFIO("XXX dropping packets because txqueue is full!\r\n");
        free_packet(p);
        lock_unlock(&rfiotxqueuelock);
        return;
    }
    p->left = NULL;
    if(rfiotxqueue != NULL) {
        rfiotxqueue->left = p;
    }
    p->right = rfiotxqueue;
    rfiotxqueue = p;
    rfiotxqueuecount++;

    lock_unlock(&rfiotxqueuelock);
}

void rfio_queue_add(volatile packet_t *p) {
    if(!lock_delay(&rfioqueuelock, 10000)) {
        free_packet(p);
        DEBUG_RFIO("XXX rfio_queue_add: can't lock rfioqueue!\r\n");
        return;
    }
    if(rfioqueuecount >= RFIOQUEUE_MAX) {
        DEBUG_RFIO("XXX dropping packets because queue is full!\r\n");
        free_packet(p);
        lock_unlock(&rfioqueuelock);
        return;
    }
    if(rfioqueue == NULL) {
        p->left = p->right = NULL;
        rfioqueue = p;
    } else {
        volatile packet_t *eptr; 
        for(eptr = rfioqueue; eptr->right != NULL; eptr = eptr->right) {
        }
        eptr->right = p;
        p->left = eptr;
        p->right = NULL;
    }
    rfioqueuecount++;

    lock_unlock(&rfioqueuelock);
}
volatile packet_t *rfio_check_queue(u_int32_t mask) {
    if(!lock_delay(&rfioqueuelock, 10000)) {
        return NULL;
    }
    volatile packet_t *p = NULL;
    for(p = rfioqueue; p != NULL; p = p->right) {
        if(rfio_mask_matches(mask, p)) {
            if(rfioqueue == p) {
                rfioqueue = p->right;
            }
            if(p->left != NULL) {
                p->left->right = p->right;
            }
            if(p->right != NULL) {
                p->right->left = p->left;
            }
            if(rfioqueuecount == 0) {
                DEBUG_RFIO("XXX: rfioqueuecount wrapped!\r\n");
            }
            rfioqueuecount--;
            break;
        }
    }
    lock_unlock(&rfioqueuelock);
    return p;
}
volatile packet_t *rfio_check_txqueue(void) {
    if(!lock_delay(&rfiotxqueuelock, 10000)) {
        return NULL;
    }
    volatile packet_t *p = NULL;
    if(rfiotxqueue != NULL) {
        p = rfiotxqueue;
        rfiotxqueue = p->right;
        p->right = NULL;
        p->left = NULL;
        if(rfiotxqueuecount == 0) {
            DEBUG_RFIO("XXX: rfiotxqueuecount wrapped!\r\n");
        }
        rfiotxqueuecount--;
    }
    lock_unlock(&rfiotxqueuelock);
    return p;
}


int rfio_mask_matches(u_int32_t mask, volatile packet_t *p) {
    if(p == NULL) {
        DEBUG_RFIO("XXX: can't match, packet is null!\r\n");
        return 0;
    }
    char type = p->data[1];
    if(mask & CHILL_STARTFIGHT) {
        if(type == PACKET_TYPE_STARTFIGHT || type == PACKET_TYPE_STARTFIGHTACK) {
            if(p->length < sizeof(rfio_startfight)) {
                DEBUG_RFIO("too-short startfight packet!\r\n");
                return 0;
            } 
            rfio_startfight *rs = (rfio_startfight *)p->data;
            if(rs->dest_badgeid == badgeinfo.badgeid) {
                return 1;
            }
        }
    }
    if(mask & CHILL_FIGHT) {
        if(type == PACKET_TYPE_YOUGO 
                || type == PACKET_TYPE_IMDEAD
                || type == PACKET_TYPE_FIGHTTURN) {
            if(p->length < sizeof(rfio_yougo)) {
                DEBUG_RFIO("too-short fight packet!\r\n");
                return 0;
            }
            rfio_yougo *ry = (rfio_yougo *)p->data;
            if(ry->dest_badgeid == badgeinfo.badgeid) {
                return 1;
            }
        }
    }
    if(mask & CHILL_NOTIFY) {
        if(type == PACKET_TYPE_GRANT) {
            if(p->length < sizeof(rfio_grant)) {
                DEBUG_RFIO("grant packet too small\r\n");
                return 0;
            }
            rfio_grant *rg = (rfio_grant *)p->data;
            if(rg->dest_badgeid == badgeinfo.badgeid) {
                return 1;
            }
        } else {
            //putstr("not a grant\r\n");
            //print_packet(p);
        }
    }
    return 0;
}

unsigned int calc_next_tx(u_int32_t badgeid, u_int32_t *start) {
    if(!lock_delay(&nearbyslock, 10000)) {
        DEBUG_RFIO("can't lock nearbyslock\r\n");
        return 0;
    }
    unsigned int i;
    for(i = 0; i < NNEARBY; i++) {
        if(nearbys[i].badgeid == badgeid) {
            break;
        }
    }
    if(i == NNEARBY) {
        lock_unlock(&nearbyslock);
        return 0;
    }
    u_int32_t tx = nearbys[i].lastseen + nearbys[i].rxoffset;
    while(tx < *MACA_CLK) {
        //putstr("XXX lastseen ");
        //put_hex32(nearbys[i].lastseen);
        //putstr(" rxoffset ");
        //put_hex32(nearbys[i].rxoffset);
        //putstr(" clk ");
        //put_hex32(*MACA_CLK);
        //putstr(NEWLINE);
        
        tx += nearbys[i].period;
    }
    *start = tx;

    lock_unlock(&nearbyslock);
    return 1;
}

void grant_item(u_int32_t badgeid, packet_t *pbuf, unsigned int item, unsigned int count) {
    if(item > 11) {
        return;
    }
    u_int32_t x;
    for(unsigned int i = 0; i < 1000; i++) {
        flash_read(EXT_BADGEIDS_ADDR + (i * 4), 4, (unsigned char *)&x);
        if(x == badgeid) {
            flash_read(EXT_TABLE_ADDR + (4096*(item-1)) + (i * 4), 4, (unsigned char *)&x);

            memset(pbuf, 0, sizeof(packet_t));
            pbuf->length = sizeof(rfio_grant);
            rfio_grant *rg = (rfio_grant *)pbuf->data;
            rg->item = item-1;
            rg->itemnonce = x;

            rg->packettype = PACKET_TYPE_GRANT;
            sendpacket(badgeid, pbuf, 2);
            usleep(INITIAL_PERIOD/2);
            sendpacket(badgeid, pbuf, count);
            return;
        }
    }
}

#if 0
void grant_item_old(u_int32_t badgeid, packet_t *pbuf, unsigned int item) {
    for(unsigned int i = 0; i < 1000; i++) {
        if(bossnonces[i].badgeid == badgeid) {
            memset(pbuf, 0, sizeof(packet_t));
            pbuf->length = sizeof(rfio_grant);
            rfio_grant *rg = (rfio_grant *)pbuf->data;
            rg->item = badgeinfo.is_boss-1;
            rg->itemnonce = bossnonces[i].nonce;
            rg->packettype = PACKET_TYPE_GRANT;
            sendpacket(badgeid, pbuf, 2);
            usleep(INITIAL_PERIOD/2);
            sendpacket(badgeid, pbuf, 2);
            return;
        }
    }
    putstr("XXX grant_item: can't find item for badge ");
    put_hex32(badgeid);
    putstr(NEWLINE);
}
#endif

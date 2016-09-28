#include <mc1322x.h>
#include <board.h>
#include <pinio.h>
#include "put.h"
#include "timer.h"
#include <task.h>
#include <misc.h>
#include <debug.h>
#include "lock.h"

#define CLICKS 300    // counter val at 24MHz/8
#define NTIMERS 64   // number of timers available

volatile u_int32_t clockus;

tevent tpool[NTIMERS]; 
u_int32_t tpoolmask;
lock_t tpoollock;

tevent *tevchain, *wraptevchain;
lock_t tevchainlock;

void timer_process(tevent *tev) {
    //putstr("--- processing task: for time ");
    //put_hex32(tev->time);
    //putstr("  now ");
    //put_hex32(clockus);
    //putstr("  tev ");
    //put_hex32((u_int32_t)tev);
    //putstr("  nptr ");
    //put_hex32((u_int32_t)tev->nptr);
    //putstr("\r\n");

    if(tev->cancelled == 0 && tev->wakeup) {
        task_wakeup_timer(tev->taskid);
    }
}

void release_tevent(tevent *tev) {
    lock_wait(&tpoollock);
    int idx = (((u_int32_t)tev) - (u_int32_t)tpool)/sizeof(tevent);
    if(idx >= NTIMERS) {
        DEBUG_TASK("index too high!\r\n");
    } else {
        if((tpoolmask & (1 << idx)) == 0) {
            DEBUG_TASK("tevent already free!\r\n");
        }
        tpoolmask &= ~(1 << idx);
    }

    lock_unlock(&tpoollock);
}

tevent *get_tevent(void) {
    lock_wait(&tpoollock);
    // XXX: use FFS instruction or similar
    int i;
    for(i = 0; i < NTIMERS; i++) {
        if((tpoolmask & (1 << i)) == 0) {
            break;
        }
    }
    if(i == NTIMERS) {
        DEBUG_TASK("no tevents");
        halt_failure(FAILURE_NO_TIMERS, 1);
    }
    tpoolmask |= (1 << i);
    lock_unlock(&tpoollock);
    tevent *t = &tpool[i];
    memset(t, 0, sizeof(tevent));
    return t;
}


void tmr2_isr(void) {
    u_int32_t lastclock = clockus;
    tevent *toprocess = NULL;
    clockus += (1000000/(24000000/(8*CLICKS)));
    if(lastclock > clockus) {
        toprocess = tevchain;
        tevchain = wraptevchain;
        wraptevchain = NULL;
    }
    tevent *tp = tevchain, *lasttp = NULL;
    while(tevchain != NULL && (tevchain->time <= clockus)) {
        lasttp = tevchain;
        tevchain = tevchain->nptr;
    }
    if(lasttp) {
        lasttp->nptr = NULL;
    }
    while(toprocess != NULL) {
        tevent *tofree = toprocess;
        timer_process(toprocess);
        if(toprocess == toprocess->nptr) {
            putstr("XXX ZUHH\r\n");
        }
        toprocess = toprocess->nptr;
        release_tevent(tofree);
    }
    if(lasttp) {
        while(tp != NULL) {
            tevent *tofree = tp;
            timer_process(tp);
            if(tp == tp->nptr) {
                putstr("XXX Z2HH\r\n");
            }
            tp = tp->nptr;
            release_tevent(tofree);
        }
    }
    *TMR2_SCTRL = 0;
    *TMR2_CSCTRL = 0x0040; /* clear compare flag */

}


void timer_init(void) {
    lock_init(&tpoollock);
    lock_init(&tevchainlock);
    tevchain = wraptevchain = NULL;
    tpoolmask = 0;

    clockus = 0;
    *TMR_ENBL &= 0x0;
    *TMR2_SCTRL = 0;
    *TMR2_CSCTRL = 0x40;
    *TMR2_LOAD = 0;
    *TMR2_COMP1 = CLICKS;
    *TMR2_CMPLD1 = CLICKS;
    *TMR2_CNTR = 0;
    // set primary count source to peri_clk/8 in TMR2_CTRL
    *TMR2_CTRL = 0x20 | (0xb << 9) | (1 << 13);
    *TMR_ENBL = 0x4;
    enable_irq(TMR);
}

tevent *timer_add_wakeup(unsigned int taskid, u_int32_t eventtime) {
    tevent *t = get_tevent();
    t->wakeup = 1;
    t->taskid = taskid;
    t->time = eventtime;
    t->cancelled = 0;
    lock_wait(&tevchainlock);
    disable_irq(TMR);
    if(eventtime <= clockus) {
        tevent *tp = wraptevchain, *ltp = NULL;
        while(tp != NULL && tp->time < eventtime) {
            ltp = tp;
            tp = tp->nptr;
        }
        if(wraptevchain == tp) {
            wraptevchain = t;
        }
        t->nptr = tp;
        if(ltp) {
            ltp->nptr = t;
        }
    } else {
        tevent *tp = tevchain, *ltp = NULL;
        while(tp != NULL && tp->time < eventtime) {
            ltp = tp;
            tp = tp->nptr;
        }
        if(tevchain == tp) {
            tevchain = t;
        }
        t->nptr = tp;
        if(ltp) {
            ltp->nptr = t;
        }
    }
    lock_unlock(&tevchainlock);
    enable_irq(TMR);
    return t;
}
void timer_cancel(tevent *tev) {
    disable_irq(TMR);
    lock_wait(&tevchainlock);
    tev->cancelled = 1;
    lock_unlock(&tevchainlock);

}

#include <mc1322x.h>
#include <board.h>
#include <info.h>
#include <pinio.h>
#include "config.h"
#include <timer.h>
#include "put.h"
#include <misc.h>
#include <lock.h>
#include <debug.h>
#include <task.h>

/* task map:
 * 0: idle (always ready_low)
 * 1: UI 
 */
task tasks[NTASKS];
void swi_sleep(unsigned int usec) {
    task *ct = &tasks[curtask];
    if(ct->timer_waiting) {
        DEBUG_TASK("task already waiting");
    }
    u_int32_t finaltime = (clockus + usec) & 0xffffffff;
    timer_add_wakeup(curtask, finaltime);
    task_clear_ready(curtask);
    ct->timer_waiting = 1;
    task_reschedule();
}


volatile unsigned int ready_high, ready_low;
unsigned int curtask;

void task_wakeup_chill(unsigned int taskid, u_int32_t wakedata) {
    tasks[taskid].wakeup_reason = 0;
    tasks[taskid].chill_waiting = 1;
    tasks[taskid].wakedata = wakedata;
    tasks[taskid].packetbuf = 0;
    tasks[taskid].tevent = 0;
    ready_high |= (1 << taskid); 
    task_reschedule();
}
void task_wakeup_timer(unsigned int taskid) {
    tasks[taskid].wakeup_reason = 0;
    tasks[taskid].timer_waiting = 1;
    tasks[taskid].wakedata = 0;
    ready_high |= (1 << taskid); 
    task_reschedule();
}
void task_wakeup_txpacket(unsigned int taskid, u_int32_t wakedata) {
    tasks[taskid].wakeup_reason = 0;
    tasks[taskid].txpacket_waiting = 1;
    tasks[taskid].wakedata = wakedata;
    ready_high |= (1 << taskid); 
    task_reschedule();
}

void task_wakeup_rfreceive(unsigned int taskid, u_int32_t wakedata) {
    tasks[taskid].wakeup_reason = 0;
    tasks[taskid].rfreceive_waiting = 1;
    tasks[taskid].wakedata = wakedata;
    ready_high |= (1 << taskid); 
    task_reschedule();
}

void task_wakeup_general(unsigned int taskid) {
    tasks[taskid].wakeup_reason = 0;
    tasks[taskid].general_waiting = 1;
    tasks[taskid].wakedata = 0;
    ready_low |= (1 << taskid); 
}

void task_init(void) {
    ready_high = ready_low = 0;
    curtask = 0;

    // XXX: remove?
    memset(tasks, 0, sizeof(tasks));
    task_wakeup_general(0);
}

void task_clear_ready(unsigned int taskid) {
    ready_high &= ~(1 << taskid);
    ready_low &= ~(1 << taskid);
}

void task_reschedule(void) {
    if(curtask > NTASKS) {
        return;
    }
    if(ready_high & (1 << curtask)) {
        return;
    }
    unsigned int taskid = 0;
    if(ready_high != 0) {
        for(taskid = 0; taskid < NTASKS; taskid++) {
            if((ready_high) & (1 << taskid)) {
                curtask = taskid;
                break;
            }
        }
    } else {
        /* NOTE: intentionally off-by-one loop; task 0 is the idle loop */
        for(taskid = NTASKS-1; taskid > 0; taskid--) {
            if((ready_low) & (1 << taskid)) {
                break;
            }
        }
    }
    curtask = taskid;
    rescheduled = 1;
}

/* Called after curtask state is saved, but before it's copied back.  Changes to state 
 * should happen here.
 */
void task_will_wakeup(void) {
    tasks[curtask].gpregs[0] = tasks[curtask].wakedata;
    tasks[curtask].wakeup_reason = 0;
}


void swi_starttask(unsigned int taskid, void *stack, void *pc) {
    tasks[taskid].gpregs[13] = (u_int32_t)stack;
    tasks[taskid].wakeup_reason = 0;

    /* XXX: This is a hack.  This is just an offset for the ARM->THUMB prologue. */
    tasks[taskid].gpregs[15] = (u_int32_t)pc + 8;
    task_wakeup_general(taskid);

    //rescheduled = 1;
    task_reschedule();
}


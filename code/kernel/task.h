#ifndef TASK_H
#define TASK_H

#include <taskdefs.h>
#include <packet.h>
#include <timer.h>

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
/* NOTE: keep in sync with taskdefs.h */
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
typedef struct task {
    u_int32_t gpregs[16];
    u_int32_t cpsr;
    u_int32_t wakedata;
    union {
        u_int32_t wakeup_reason;
        struct {
            unsigned char timer_waiting : 1;
            unsigned char general_waiting : 1;
            unsigned char rfreceive_waiting: 1;
            unsigned char txpacket_waiting: 1;
            unsigned char rfio_waiting: 1;
            unsigned char button_waiting: 1;
            unsigned char chill_waiting : 1; // XXX: necessary?
        };
    };
    volatile packet_t *packetbuf;
    tevent *tevent;
} __attribute__((packed)) task;
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
/* NOTE: keep in sync with taskdefs.h */
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

extern volatile unsigned int ready_high, ready_low;
extern volatile unsigned int rescheduled;
extern unsigned int curtask;
extern task tasks[];
void task_init(void);
void task_reschedule(void);
void task_wakeup_timer(unsigned int task);
void task_wakeup_rfreceive(unsigned int task, u_int32_t wakedata);
void task_wakeup_txpacket(unsigned int task, u_int32_t wakedata);
void task_clear_ready(unsigned int task);
void task_wakeup_chill(unsigned int task, u_int32_t wakedata);

#endif

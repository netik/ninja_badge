#ifndef TIMER_H
#define TIMER_H

typedef
struct tevent {
    struct tevent *nptr;
    u_int32_t time;
    union {
        u_int32_t typeflags;
        struct {
            unsigned char wakeup : 1;
            unsigned char cancelled : 1;
        };
    };
    u_int32_t taskid;
} tevent;

void timer_init(void);
tevent *timer_add_wakeup(unsigned int task, u_int32_t eventtime);
void timer_cancel(tevent *tev);

extern volatile u_int32_t clockus;

#endif

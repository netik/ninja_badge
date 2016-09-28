#include <mc1322x.h>
#include <pinio.h>
#include <syscalls.h>
#include <pinout.h>
#include <put.h>
#include <board.h>
#include <lock.h>
#include "misc.h"

char *NEWLINE = "\r\n";

// XXX: asm
void memset(void *ptr, unsigned char v, unsigned int sz) {
    unsigned char *p = (unsigned char *)ptr;
    for(unsigned int i = 0; i < sz; i++) {
        p[i] = v;
    }
}
unsigned int strlen(const char *s) {
    unsigned int i;
    for(i = 0; s[i] != 0; i++) {
    }
    return i;
}
/* not so good for quality randomness */
unsigned int rand(void) {
    return (int)*MACA_RANDOM;
}
unsigned int stringlen(char *str, unsigned int max) {
    unsigned int i;
    for(i = 0; i < max; i++) {
        if(str[i] == 0) {
            break;
        }
    }
    return i;
}

// XXX: macroize
void lock_init(lock_t *lock) {
    *lock = 0;
}
int lock_delay(lock_t *lock, unsigned long usecs) {
    for(int i = 0; i < 10; i++) {
        if(lock_nowait(lock)) {
            return 1;
        }
        usleep(usecs/10);
    }
    return 0;
}
void print_packet(volatile packet_t *p) {
    put_hex(p->length);
    putstr(": ");
    for(unsigned int i = 0; i < p->length; i++) {
        put_hex(p->data[i]);
        putstr(" ");
    }
    putstr(NEWLINE);
}
void shift1(uint8_t c) {
    int i;
    setPin(SER1P, 0);
    setPin(SCK1P, 0);
    setPin(RCK1P, 0);
    for(i = 7; i >= 0; i--) {
        setPin(SCK1P, 0);
        setPin(RCK1P, 0);
        if(((c >> i) & 1) == 1){
            setPin(SER1P, 1);
        } else {
            setPin(SER1P, 0);
        }
        setPin(SCK1P, 1);
        setPin(RCK1P, 1);
        setPin(SER1P, 0);
    }
}


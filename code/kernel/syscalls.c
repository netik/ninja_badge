#include <mc1322x.h>
#include <board.h>
#include "syscalls.h"
void starttask(unsigned int taskid, void *stackptr, void *pc) {
    __asm__(
            "mov r0, %[taskid]\n\t"
            "mov r1, %[stackptr]\n\t"
            "mov r2, %[pc]\n\t"
            "swi #0x1\n\t"
            : 
            : [taskid] "g" (taskid), [stackptr] "g" (stackptr), [pc] "g" (pc)
            );
}

unsigned int usleep(unsigned int microsecs) {
    register unsigned int outval;
    __asm__(
            "mov r0, %[microsecs]\n\t"
            "swi #0x0\n\t"
            "mov %[out], r0\n\t"
            : [out] "=g" (outval)
            : [microsecs] "g" (microsecs)
            : "r0" 
            );
    return outval;
}
unsigned int txpacket(volatile packet_t *pack) {
    register unsigned int outval;
    __asm__(
            "mov r0, %[packetptr]\n\t"
            "swi #0x3\n\t"
            "mov %[out], r0\n\t"
            : [out] "=g" (outval)
            : [packetptr] "g" (pack)
            : "r0" 
            );
    return (unsigned int)outval;
}
packet_t *rfreceive(unsigned int starttime, unsigned int length) {
    register unsigned int outval;
    __asm__(
            "mov r0, %[starttime]\n\t"
            "mov r1, %[length]\n\t"
            "swi #0x2\n\t"
            "mov %[out], r0\n\t"
            : [out] "=g" (outval)
            : [starttime] "g" (starttime), [length] "g" (length)
            : "r0" 
            );
    return (packet_t *)outval;
}

void sendpacket(u_int32_t badgeid, packet_t *packet, unsigned int tries) {
    __asm__(
            "mov r0, %[badgeid]\n\t"
            "mov r1, %[packet]\n\t"
            "mov r2, %[tries]\n\t"
            "swi #0x6\n\t"
            : 
            : [badgeid] "g" (badgeid), [packet] "g" (packet), [tries] "g" (tries)
            : "r0" 
            );
}


unsigned int chill(u_int32_t mask, unsigned int timeout, volatile packet_t *packetbuf) {
    register unsigned int outval;
    __asm__(
            "mov r0, %[mask]\n\t"
            "mov r1, %[timeout]\n\t"
            "mov r2, %[packetbuf]\n\t"
            "swi #0x4\n\t"
            "mov %[out], r0\n\t"
            : [out] "=g" (outval)
            : [mask] "g" (mask), [timeout] "g" (timeout), [packetbuf] "g" (packetbuf)
            : "r0" 
            );
    return outval;
}

void chill_packet_notify(void) {
    __asm__(
            "swi #0x5\n\t"
            : 
            : 
            : "r0"
            );

}

int doswi(__attribute__((unused))register int a, __attribute__((unused))register int b, __attribute__((unused))register int c) {
    __asm__(
            "swi #0x42\n\t"
            "mov %[a], r0\n\t"
            : [a] "=g" (a)
            : 
            : "r5"
            );
    return a;
}



#ifndef SYSCALLS_H
#define SYSCALLS_H

unsigned int usleep(unsigned int microsecs);
packet_t *rfreceive(unsigned int starttime, unsigned int length);
unsigned int txpacket(volatile packet_t *pack);
void starttask(unsigned int taskid, void *stackptr, void *pc);
int doswi(__attribute__((unused))register int a, __attribute__((unused))register int b, __attribute__((unused))register int c);
unsigned int chill(u_int32_t mask, unsigned int timeout, volatile packet_t *packetbuf);
void sendpacket(u_int32_t badgeid, packet_t *packet, unsigned int tries);
void chill_packet_notify(void);

#endif /* SYSCALLS_H */

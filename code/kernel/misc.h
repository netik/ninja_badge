#ifndef MISC_H
#define MISC_H

enum {
    FAILURE_EXTLOAD = 1,
    FAILURE_CORRUPTSCREENTBL = 2,
    FAILURE_NVM_INIT = 3,
    FAILURE_NO_TIMERS = 4,
    FAILURE_CORRUPTINFO = 5,
};

void memset(void *ptr, unsigned char v, unsigned int sz);
unsigned int strlen(const char *s);
void memcpy(void *dest, void *src, unsigned int size);
unsigned int stringlen(char *str, unsigned int max);
void halt_failure(unsigned int red, unsigned int green);
unsigned int rand(void);
void clockusdelay(unsigned int microsecs);
unsigned int escapable_clockus(unsigned int microsecs);
void print_packet(volatile packet_t *p);
void shift1(uint8_t c);

extern char *NEWLINE;
extern volatile int force_maca;     // XXX: trial hack.
extern u_int32_t buttons_down;

#define IS_VALID_NAMECHAR(x) (((x) >= 0x20) && ((x) <= 'Z'))

#endif /* MISC_H */

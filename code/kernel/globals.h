#ifndef GLOBALS_H
#define GLOBALS_H 

#include <info.h>

struct globalvars {
    packet_t startfight;
    player_info saved_playerinfo;
    u_int32_t opponent;
    u_int32_t togrant;
    u_int32_t giveitem;

    unsigned int now_fighting;
    unsigned int was_attacked;
};

extern volatile struct globalvars globals;

#endif /* GLOBALS_H */

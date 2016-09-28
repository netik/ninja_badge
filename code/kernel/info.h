#ifndef INFO_H
#define INFO_H

#include <mc1322x.h>
#include <board.h>

extern volatile int rand_needs_seed;
extern nvmType_t nvm_type;

#define BADGEINFO_FLASH_ADDR 0x18000
#define PLAYERINFO_FLASH_ADDR 0x19000

typedef struct player_info {
    char name[16];
    union {
        u_int32_t varioux;
        struct {
            unsigned int shuriken_count : 4;
            unsigned int redbull_count : 4;
            unsigned int item_mask : 10;
            unsigned int player_level : 4;
            unsigned int name_set_flag: 1;
        };
    };
    u_int16_t hp;
    u_int32_t xp;
    u_int32_t buzzer_disabled;
} __attribute__((packed)) player_info;


#define IS_VALID_BADGETYPE(x) ((x) >= BADGE_NORMAL && (x) <= BADGE_MAINT);
typedef enum {
    BADGE_NORMAL = 1,
    BADGE_BOSS,
    BADGE_NINJA,
    BADGE_MAINT,
} badgetype_t;

typedef
struct nearby_user {
    u_int32_t badgeid;          
    u_int32_t lastseen;         // last time seen (rx time)
    u_int32_t rxoffset;         // last seen rx (offset from end of tx)
    u_int32_t period;           // last seen beacon period
    unsigned char level;        // mark as 0 when entry not used
    char name[16];
    union {
        u_int16_t playerstate;
        struct {        // SYNC UP with rfio_beacon
            unsigned char is_ninja : 1;    
            unsigned char is_boss : 4;
            unsigned int item_mask : 10;
        }__attribute__((packed));
    }__attribute__((packed));
}__attribute__((packed)) nearby_user;

typedef struct badge_info {
    u_int32_t badgeid;
    u_int32_t badgetype;
    u_int32_t is_ninja;
    u_int32_t is_boss;
    u_int32_t levelnonce;
    u_int32_t itemnonce[10];
} badge_info;
extern u_int16_t base_xp[10];

struct nonce {
    unsigned int badgeid;
    unsigned int nonce;
} __attribute__((packed));
extern volatile player_info playerinfo;
extern volatile badge_info badgeinfo;

void info_init(void);
void info_load(void);
unsigned int info_isvalid(void);
unsigned int calc_strength(unsigned int player_level, unsigned int item_mask);
unsigned int calc_defense(unsigned int player_level, unsigned int item_mask);
unsigned int calc_agility(unsigned int player_level, unsigned int item_mask);
unsigned int calc_spirit(unsigned int player_level, unsigned int item_mask);
unsigned int calc_level(unsigned int xp);
void save_playerinfo(void);
void info_commit_playerinfo(void);
void restore_playerinfo(void);

#endif

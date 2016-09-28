#include <sys/types.h>

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

typedef struct badge_info {
    u_int32_t badgeid;
    u_int32_t badgetype;
    u_int32_t is_ninja;
    u_int32_t is_boss;
    u_int32_t levelnonce;
    u_int32_t itemnonce[10];
} badge_info;

typedef enum {
    BADGE_NORMAL = 1,
    BADGE_BOSS,
    BADGE_NINJA,
    BADGE_MAINT,
} badgetype_t;


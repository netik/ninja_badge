#ifndef EXT_H
#define EXT_H

void ext_init(void);
unsigned int (*ext_load_screen(int screen))(void);
int ext_load_bitmap(int bitmap, void *bmp, unsigned int size, unsigned int *width, unsigned int *height);
void flash_secterase(int addr);
void flash_read(int addr, int len, uint8_t *buf);
void flash_bytewrite(int addr, uint8_t val);
void flash_wren(void);
u_int8_t spi_op(u_int8_t outb);
unsigned int get_enemy_id(unsigned int isboss);

enum {
    SCREEN_SPLASH = 0,
    SCREEN_LOAD_FAILURE = 1,
    SCREEN_INPUT_USER = 2,
    SCREEN_MAIN = 3,
    SCREEN_FIGHT = 4,
    SCREEN_ENEMY_SELECT = 5, 
    SCREEN_INVENTORY = 6,
    SCREEN_SETUP = 7,
    SCREEN_CRYPTIX = 8,
    SCREEN_GRANT = 9,
    SCREEN_LINK = 10,
};

enum {
    ENEMY_PIRATE = 0,
    ENEMY_REPORTER = 1,
    ENEMY_POOL2 = 2,
    ENEMY_RIGHTSENSEI = 3,
    ENEMY_ECHELON = 4,
    ENEMY_SENSEI = 5,
    ENEMY_NINJA = 6,
    ENEMY_RIGHTNINJA = 7,
};

enum {
    EXT_LOCATION_INTERNAL = 1,
    EXT_LOCATION_MCUFLASH = 2,
    EXT_LOCATION_EXTFLASH = 3
};

#define SPIFL_CMD_READID 0x90   // read-id
#define SPI_SCK 7      // SPI serial clock
#define SPI_MOSI 6     // SPI master out, slave in
#define SPI_MISO 5     // SPI master in, slave out
#define FLASH_E 4      // flash enable
#define FLASH_HOLD 6   // flash hold mode (NOT USED; tied high)

typedef
struct item_info {
    char *name;
    char *description1;
    char *description2;
    u_int32_t image;
} item_info;

extern struct item_info items[12];

typedef
struct screen_info {
    unsigned short location;    // EXT_LOCATION_*
    unsigned short size;        // not valid for internal location
    u_int32_t address;          // address
} screen_info;

typedef
struct enemy_info {
    unsigned int attacking;
    unsigned int dodging;
    unsigned int fighting;
    unsigned int hit;
    unsigned int ko;
    unsigned int victory;
} enemy_info;

extern enemy_info enemyinfo[];


typedef
struct bitmap_info {
    unsigned short location;    // EXT_LOCATION_*
    u_int32_t address;          // address
    unsigned short size;        // not valid for internal location
} bitmap_info;

#define EXT_BASE_ADDR 0x2000
#define EXT_BADGEIDS_ADDR 0x4000
#define EXT_TABLE_ADDR 0x5000
#define MCUFLASH_BASE_ADDR 0x1A000

enum {
    BOOT_UP_SCREEN = 0,
    CREDITS_BARKODE = 1,
    CREDITS_CNELSON = 2,
    CREDITS_CSTONE = 3,
    CREDITS_FARCALL = 4,
    CREDITS_SKRIKE = 5,
    CREDITS_WOZ = 6,
    SPLASH_SCREEN1 = 7,

    ECHELON_ATTACKING = 8,
    ECHELON_DODGING = 9,
    ECHELON_FIGHTING = 10,
    ECHELON_HIT = 11,
    ECHELON_KO = 12,
    ECHELON_VICTORY = 13,
    ITEM10_ECHELON = 14,
    ITEM1_MARK = 15,
    ITEM2_LIGHT = 16,
    ITEM3_ASSASSIN = 17,
    ITEM4_KNIGHT = 18,
    ITEM5_RUNEWINGS = 19,
    ITEM6_CAMERA = 20,
    ITEM7_STARFISH = 21,
    ITEM8_CAESER = 22,
    ITEM9_PEBBLE = 23,
    NINJA_ATTACKING = 24,
    NINJA_DODGING = 25,
    NINJA_FIGHTING = 26,
    NINJA_HIT = 27,
    NINJA_IDLE = 28,
    NINJA_KO = 29,
    NINJA_LARGE = 30,
    NINJA_REDBULL = 31,
    NINJA_SHURIKEN = 32,
    NINJA_VICTORY = 33,
    PIRATE_ATTACKING = 34,
    PIRATE_DODGING = 35,
    PIRATE_FIGHTING = 36,
    PIRATE_HIT = 37,
    PIRATE_KO = 38,
    PIRATE_VICTORY = 39,
    POOL2_ATTACKING = 40,
    POOL2_DODGING = 41,
    POOL2_FIGHTING = 42,
    POOL2_HIT = 43,
    POOL2_KO = 44,
    POOL2_VICTORY = 45,
    REDBULL = 46,
    REPORTER_ATTACKING = 47,
    REPORTER_DODGING = 48,
    REPORTER_FIGHTING = 49,
    REPORTER_HIT = 50,
    REPORTER_KO = 51,
    REPORTER_VICTORY = 52,
    RIGHT_NINJA_ATTACKING = 53,
    RIGHT_NINJA_DODGING = 54,
    RIGHT_NINJA_FIGHTING = 55,
    RIGHT_NINJA_HIT = 56,
    RIGHT_NINJA_KO = 57,
    RIGHT_NINJA_REDBULL = 58,
    RIGHT_NINJA_SHURIKEN = 59,
    RIGHT_NINJA_VICTORY = 60,
    RIGHT_SENSEI_ATTACKING = 61,
    RIGHT_SENSEI_DODGING = 62,
    RIGHT_SENSEI_FIGHTING = 63,
    RIGHT_SENSEI_HIT = 64,
    RIGHT_SENSEI_IDLE = 65,
    RIGHT_SENSEI_KO = 66,
    RIGHT_SENSEI_REDBULL = 67,
    RIGHT_SENSEI_SHURIKEN = 68,
    RIGHT_SENSEI_VICTORY = 69,
    SENSEI_ATTACKING = 70,
    SENSEI_DODGING = 71,
    SENSEI_FIGHTING = 72,
    SENSEI_HIT = 73,
    SENSEI_IDLE = 74,
    SENSEI_KO = 75,
    SENSEI_REDBULL = 76,
    SENSEI_SHURIKEN = 77,
    SENSEI_VICTORY = 78,
    SHURIKEN = 79,


};
#endif /* EXT_H */

#include <mc1322x.h>
#include <board.h>
#include <ext.h>
#include <pinio.h>
#include <pinout.h>
#include "config.h"
#include <timer.h>
#include <misc.h>
#include "put.h"
#include <lock.h>
#include <debug.h>
#include <task.h>
#include "syscalls.h"
#include <rfio.h>
#include <screen-splash.h>
#include <screen-loadfail.h>
#include <screen-main.h>
#include <screen-inputuser.h>
#include <screen-grant.h>
#include <screen-inventory.h>
#include <rubix.h>
#include <screen-enemyselect.h>
#include <screen-link.h>
#include <screen-setup.h>
#include <screen-fight.h>

void ext_init(void) {
    pinFunc(SPI_SCK, 3);
    pinFunc(SPI_MOSI, 3);
    pinFunc(SPI_MISO, 3);
    pinFunc(FLASH_E, 3);

    pinDirection(SPI_SCK, PIN_OUTPUT);
    pinDirection(SPI_MOSI, PIN_OUTPUT);
    pinDirection(SPI_MISO, PIN_INPUT);
    pinDirection(FLASH_E, PIN_OUTPUT);
}
item_info items[] = {
    {
        .name = "MARK OF THE DEFENDER",
        .description1 = "INCREASE",
        .description2 = "DEFENSE 10%",
        .image = ITEM1_MARK,
    },
    {
        .name = "LIGHT OF HOPE",
        .description1 = "INCREASE",
        .description2 = "SPIRIT 10%",
        .image = ITEM2_LIGHT,
    },
    {
        .name = "SILENT ASSASSIN",
        .description1 = "INCREASE",
        .description2 = "AGILITY 10%",
        .image = ITEM3_ASSASSIN,
    },
    {
        .name = "KNIGHT OF KINGPIN EMPIRE",
        .description1 = "INCREASE",
        .description2 = "STRENGTH 10%",
        .image = ITEM4_KNIGHT
    },
    {
        .name = "MARK OF THE ALCHEMIST",
        .description1 = "BOOST REDBULL",
        .description2 = "POWER BY 25%",
        .image = ITEM5_RUNEWINGS
    },
    {
        .name = "CAMERA SHY",
        .description1 = "DODGE 25%",
        .description2 = "MORE OFTEN",
        .image = ITEM6_CAMERA
    },
    {
        .name = "CHOCOLATE STARFISH",
        .description1 = "5% REDUCTION",
        .description2 = "IN ALL DAMAGE",
        .image = ITEM7_STARFISH
    },
    {
        .name = "CAEZAR'S WRATH",
        .description1 = "BOOST SHURIKEN",
        .description2 = "DAMAGE BY 25%",
        .image = ITEM8_CAESER
    },
    {
        .name = "SENSEI'S PEBBLE",
        .description1 = "DODGE FIRST",
        .description2 = "FIGHT STRIKE",
        .image = ITEM9_PEBBLE
    },
    {
        .name = "ECHELON'S REACH",
        .description1 = "INCREASE CRIT",
        .description2 = " DAMAGE 15%",
        .image = ITEM10_ECHELON
    },
    {
        .name = "SHURIKEN",
        .description1 = "INSTANT",
        .description2 = "DAMAGE",
        .image = SHURIKEN,
    },
    {
        .name = "REDBULL",
        .description1 = "INSTANT HP",
        .description2 = "RECOVERY",
        .image = REDBULL,
    },
};

bitmap_info bitmap_table[] = {
    { EXT_LOCATION_EXTFLASH, 0, 482 },      // BOOT_UP_SCREEN
    { EXT_LOCATION_EXTFLASH, 482, 1026 },   // CREDITS_BARKODE
    { EXT_LOCATION_EXTFLASH, 1508, 1026 },  // CREDITS_CNELSON
    { EXT_LOCATION_EXTFLASH, 2534, 1026 },  // CREDITS_CSTONE
    { EXT_LOCATION_EXTFLASH, 3560, 1026 },  // CREDITS_FARCALL
    { EXT_LOCATION_EXTFLASH, 4586, 1026 },  // CREDITS_SKRIKE
    { EXT_LOCATION_EXTFLASH, 5612, 1026 },  // CREDITS_WOZ
    { EXT_LOCATION_EXTFLASH, 6638, 1026 },  // SPLASH_SCREEN1

    { EXT_LOCATION_MCUFLASH, 0, 362 },   // ECHELON_ATTACKING
    { EXT_LOCATION_MCUFLASH, 362, 362 },   // ECHELON_DODGING
    { EXT_LOCATION_MCUFLASH, 724, 362 },   // ECHELON_FIGHTING
    { EXT_LOCATION_MCUFLASH, 1086, 362 },   // ECHELON_HIT
    { EXT_LOCATION_MCUFLASH, 1448, 362 },   // ECHELON_KO
    { EXT_LOCATION_MCUFLASH, 1810, 362 },   // ECHELON_VICTORY
    { EXT_LOCATION_MCUFLASH, 2172, 242 },   // ITEM10_ECHELON
    { EXT_LOCATION_MCUFLASH, 2414, 242 },   // ITEM1_MARK
    { EXT_LOCATION_MCUFLASH, 2656, 242 },   // ITEM2_LIGHT
    { EXT_LOCATION_MCUFLASH, 2898, 242 },   // ITEM3_ASSASSIN
    { EXT_LOCATION_MCUFLASH, 3140, 242 },   // ITEM4_KNIGHT
    { EXT_LOCATION_MCUFLASH, 3382, 242 },   // ITEM5_RUNEWINGS
    { EXT_LOCATION_MCUFLASH, 3624, 242 },   // ITEM6_CAMERA
    { EXT_LOCATION_MCUFLASH, 3866, 242 },   // ITEM7_STARFISH
    { EXT_LOCATION_MCUFLASH, 4108, 242 },   // ITEM8_CAESER
    { EXT_LOCATION_MCUFLASH, 4350, 242 },   // ITEM9_PEBBLE
    { EXT_LOCATION_MCUFLASH, 4592, 242 },   // NINJA_ATTACKING
    { EXT_LOCATION_MCUFLASH, 4834, 242 },   // NINJA_DODGING
    { EXT_LOCATION_MCUFLASH, 5076, 242 },   // NINJA_FIGHTING
    { EXT_LOCATION_MCUFLASH, 5318, 242 },   // NINJA_HIT
    { EXT_LOCATION_MCUFLASH, 5560, 242 },   // NINJA_IDLE
    { EXT_LOCATION_MCUFLASH, 5802, 242 },   // NINJA_KO
    { EXT_LOCATION_MCUFLASH, 6044, 338 },   // NINJA_LARGE
    { EXT_LOCATION_MCUFLASH, 6382, 242 },   // NINJA_REDBULL
    { EXT_LOCATION_MCUFLASH, 6624, 242 },   // NINJA_SHURIKEN
    { EXT_LOCATION_MCUFLASH, 6866, 242 },   // NINJA_VICTORY
    { EXT_LOCATION_MCUFLASH, 7108, 242 },   // PIRATE_ATTACKING
    { EXT_LOCATION_MCUFLASH, 7350, 242 },   // PIRATE_DODGING
    { EXT_LOCATION_MCUFLASH, 7592, 242 },   // PIRATE_FIGHTING
    { EXT_LOCATION_MCUFLASH, 7834, 242 },   // PIRATE_HIT
    { EXT_LOCATION_MCUFLASH, 8076, 242 },   // PIRATE_KO
    { EXT_LOCATION_MCUFLASH, 8318, 242 },   // PIRATE_VICTORY
    { EXT_LOCATION_MCUFLASH, 8560, 242 },   // POOL2_ATTACKING
    { EXT_LOCATION_MCUFLASH, 8802, 242 },   // POOL2_DODGING
    { EXT_LOCATION_MCUFLASH, 9044, 242 },   // POOL2_FIGHTING
    { EXT_LOCATION_MCUFLASH, 9286, 242 },   // POOL2_HIT
    { EXT_LOCATION_MCUFLASH, 9528, 242 },   // POOL2_KO
    { EXT_LOCATION_MCUFLASH, 9770, 242 },   // POOL2_VICTORY
    { EXT_LOCATION_MCUFLASH, 10012, 242 },   // REDBULL
    { EXT_LOCATION_MCUFLASH, 10254, 202 },   // REPORTER_ATTACKING
    { EXT_LOCATION_MCUFLASH, 10456, 202 },   // REPORTER_DODGING
    { EXT_LOCATION_MCUFLASH, 10658, 202 },   // REPORTER_FIGHTING
    { EXT_LOCATION_MCUFLASH, 10860, 202 },   // REPORTER_HIT
    { EXT_LOCATION_MCUFLASH, 11062, 202 },   // REPORTER_KO
    { EXT_LOCATION_MCUFLASH, 11264, 202 },   // REPORTER_VICTORY
    { EXT_LOCATION_MCUFLASH, 11466, 242 },   // RIGHT_NINJA_ATTACKING
    { EXT_LOCATION_MCUFLASH, 11708, 242 },   // RIGHT_NINJA_DODGING
    { EXT_LOCATION_MCUFLASH, 11950, 242 },   // RIGHT_NINJA_FIGHTING
    { EXT_LOCATION_MCUFLASH, 12192, 242 },   // RIGHT_NINJA_HIT
    { EXT_LOCATION_MCUFLASH, 12434, 242 },   // RIGHT_NINJA_KO
    { EXT_LOCATION_MCUFLASH, 12676, 242 },   // RIGHT_NINJA_REDBULL
    { EXT_LOCATION_MCUFLASH, 12918, 242 },   // RIGHT_NINJA_SHURIKEN
    { EXT_LOCATION_MCUFLASH, 13160, 242 },   // RIGHT_NINJA_VICTORY
    { EXT_LOCATION_MCUFLASH, 13402, 242 },   // RIGHT_SENSEI_ATTACKING
    { EXT_LOCATION_MCUFLASH, 13644, 242 },   // RIGHT_SENSEI_DODGING
    { EXT_LOCATION_MCUFLASH, 13886, 242 },   // RIGHT_SENSEI_FIGHTING
    { EXT_LOCATION_MCUFLASH, 14128, 242 },   // RIGHT_SENSEI_HIT
    { EXT_LOCATION_MCUFLASH, 14370, 242 },   // RIGHT_SENSEI_IDLE
    { EXT_LOCATION_MCUFLASH, 14612, 242 },   // RIGHT_SENSEI_KO
    { EXT_LOCATION_MCUFLASH, 14854, 242 },   // RIGHT_SENSEI_REDBULL
    { EXT_LOCATION_MCUFLASH, 15096, 242 },   // RIGHT_SENSEI_SHURIKEN
    { EXT_LOCATION_MCUFLASH, 15338, 242 },   // RIGHT_SENSEI_VICTORY
    { EXT_LOCATION_MCUFLASH, 15580, 242 },   // SENSEI_ATTACKING
    { EXT_LOCATION_MCUFLASH, 15822, 242 },   // SENSEI_DODGING
    { EXT_LOCATION_MCUFLASH, 16064, 242 },   // SENSEI_FIGHTING
    { EXT_LOCATION_MCUFLASH, 16306, 242 },   // SENSEI_HIT
    { EXT_LOCATION_MCUFLASH, 16548, 242 },   // SENSEI_IDLE
    { EXT_LOCATION_MCUFLASH, 16790, 242 },   // SENSEI_KO
    { EXT_LOCATION_MCUFLASH, 17032, 242 },   // SENSEI_REDBULL
    { EXT_LOCATION_MCUFLASH, 17274, 242 },   // SENSEI_SHURIKEN
    { EXT_LOCATION_MCUFLASH, 17516, 242 },   // SENSEI_VICTORY
    { EXT_LOCATION_MCUFLASH, 17758, 242 },   // SHURIKEN
};

enemy_info enemyinfo[] = {
    [ENEMY_PIRATE] = {
        .attacking = PIRATE_ATTACKING,
        .dodging = PIRATE_DODGING,
        .fighting = PIRATE_FIGHTING,
        .hit = PIRATE_HIT,
        .ko = PIRATE_KO,
        .victory = PIRATE_VICTORY,
    },
    [ENEMY_REPORTER] = {
        .attacking = REPORTER_ATTACKING,
        .dodging = REPORTER_DODGING,
        .fighting = REPORTER_FIGHTING,
        .hit = REPORTER_HIT,
        .ko = REPORTER_KO,
        .victory = REPORTER_VICTORY,
    },
    [ENEMY_POOL2] = {
        .attacking = POOL2_ATTACKING,
        .dodging = POOL2_DODGING,
        .fighting = POOL2_FIGHTING,
        .hit = POOL2_HIT,
        .ko = POOL2_KO,
        .victory = POOL2_VICTORY,
    },
    [ENEMY_RIGHTSENSEI] = {
        .attacking = RIGHT_SENSEI_ATTACKING,
        .dodging = RIGHT_SENSEI_DODGING,
        .fighting = RIGHT_SENSEI_FIGHTING,
        .hit = RIGHT_SENSEI_HIT,
        .ko = RIGHT_SENSEI_KO,
        .victory = RIGHT_SENSEI_VICTORY,
    },
    [ENEMY_ECHELON] = {
        .attacking = ECHELON_ATTACKING,
        .dodging = ECHELON_DODGING,
        .fighting = ECHELON_FIGHTING,
        .hit = ECHELON_HIT,
        .ko = ECHELON_KO,
        .victory = ECHELON_VICTORY,
    },
    [ENEMY_SENSEI] = {
        .attacking = SENSEI_ATTACKING,
        .dodging = SENSEI_DODGING,
        .fighting = SENSEI_FIGHTING,
        .hit = SENSEI_HIT,
        .ko = SENSEI_KO,
        .victory = SENSEI_VICTORY,
    },
    [ENEMY_NINJA] = {
        .attacking = NINJA_ATTACKING,
        .dodging = NINJA_DODGING,
        .fighting = NINJA_FIGHTING,
        .hit = NINJA_HIT,
        .ko = NINJA_KO,
        .victory = NINJA_VICTORY,
    },
    [ENEMY_RIGHTNINJA] = {
        .attacking = RIGHT_NINJA_ATTACKING,
        .dodging = RIGHT_NINJA_DODGING,
        .fighting = RIGHT_NINJA_FIGHTING,
        .hit = RIGHT_NINJA_HIT,
        .ko = RIGHT_NINJA_KO,
        .victory = RIGHT_NINJA_VICTORY,
    },
};

screen_info screen_table[] = {
    [SCREEN_SPLASH] = {
        .location = EXT_LOCATION_INTERNAL,
        .size = 0,
        .address = (u_int32_t)screen_splash
    },
    [SCREEN_LOAD_FAILURE] = {
        .location = EXT_LOCATION_INTERNAL,
        .size = 0,
        .address = (u_int32_t)screen_load_failure
    },
    [SCREEN_INPUT_USER] = {
        .location = EXT_LOCATION_INTERNAL,
        .size = 0,
        .address = (u_int32_t)screen_inputuser
    },
    [SCREEN_MAIN] = {
        .location = EXT_LOCATION_INTERNAL,
        .size = 0,
        .address = (u_int32_t)screen_main
    },
    [SCREEN_FIGHT] = {
        .location = EXT_LOCATION_INTERNAL,
        .size = 0,
        .address = (u_int32_t)screen_fight
    },
    [SCREEN_ENEMY_SELECT] = {
        .location = EXT_LOCATION_INTERNAL,
        .size = 0,
        .address = (u_int32_t)screen_enemyselect
    },
    [SCREEN_INVENTORY] = {
        .location = EXT_LOCATION_INTERNAL,
        .size = 0,
        .address = (u_int32_t)screen_inventory
    },
    [SCREEN_SETUP] = {
        .location = EXT_LOCATION_INTERNAL,
        .size = 0,
        .address = (u_int32_t)screen_setup
    },
    [SCREEN_CRYPTIX] = {
        .location = EXT_LOCATION_INTERNAL,
        .size = 0,
        .address = (u_int32_t)CC_main_loop
    },
    [SCREEN_GRANT] = {
        .location = EXT_LOCATION_INTERNAL,
        .size = 0,
        .address = (u_int32_t)screen_grant
    },
    [SCREEN_LINK] = {
        .location = EXT_LOCATION_INTERNAL,
        .size = 0,
        .address = (u_int32_t)screen_link
    },
};

void ext_get_size(int bitmap, unsigned int *width, unsigned int *height) {
    if(bitmap_table[bitmap].location == EXT_LOCATION_EXTFLASH) {
        unsigned char buf[2];
        flash_read(EXT_BASE_ADDR + bitmap_table[bitmap].address, 2, (u_int8_t *)&buf);
        if(width) {
            *width = buf[0];
        }
        if(height) {
            *height = buf[1];
        }
    } else if(bitmap_table[bitmap].location == EXT_LOCATION_MCUFLASH) {
        unsigned char buf[2];
        nvmErr_t err;
        err = nvm_read(gNvmInternalInterface_c, nvm_type, buf, 
                MCUFLASH_BASE_ADDR + bitmap_table[bitmap].address, 2);
        if(err != gNvmErrNoError_c) {
            halt_failure(FAILURE_NVM_INIT, err);
        }
    }
}
int ext_load_bitmap(int bitmap, void *bmp, unsigned int size, unsigned int *outwidth, unsigned int *outheight) {
    if(bitmap_table[bitmap].location == EXT_LOCATION_EXTFLASH) {
        unsigned char buf[2];
        flash_read(EXT_BASE_ADDR + bitmap_table[bitmap].address, 2, (u_int8_t *)&buf);
        unsigned int width = (buf[0] & 0x7) ? ((buf[0] & 0xf8) + 8) : buf[0];
        unsigned int height = (buf[1] & 0x7) ? ((buf[1] & 0xf8) + 8) : buf[1];
        unsigned int realsz = width * height;
        if((realsz / 8) > size) {
            return 0;
        }
        if(outwidth) {
            *outwidth = buf[0];
        }
        if(outheight) {
            *outheight = buf[1];
        }
        flash_read(EXT_BASE_ADDR + bitmap_table[bitmap].address+2, realsz/8, bmp);
        return 1;
    } else if(bitmap_table[bitmap].location == EXT_LOCATION_MCUFLASH) {
        unsigned char buf[2];
        nvmErr_t err;
        err = nvm_read(gNvmInternalInterface_c, nvm_type, buf, 
                MCUFLASH_BASE_ADDR + bitmap_table[bitmap].address, 2);
        if(err != gNvmErrNoError_c) {
            halt_failure(FAILURE_NVM_INIT, err);
        }
        unsigned int width = (buf[0] & 0x7) ? ((buf[0] & 0xf8) + 8) : buf[0];
        unsigned int height = (buf[1] & 0x7) ? ((buf[1] & 0xf8) + 8) : buf[1];
        unsigned int realsz = width * height;
        if((realsz / 8) > size) {
            return 0;
        }
        if(outwidth) {
            *outwidth = buf[0];
        }
        if(outheight) {
            *outheight = buf[1];
        }
        err = nvm_read(gNvmInternalInterface_c, nvm_type, bmp, 
                MCUFLASH_BASE_ADDR + bitmap_table[bitmap].address + 2, realsz/8);
        if(err != gNvmErrNoError_c) {
            halt_failure(FAILURE_NVM_INIT, err);
        }
        return 1;
    } else {
        return 0;
    }
}

unsigned int (*ext_load_screen(int screen))(void) {
    switch(screen_table[screen].location) {
        case EXT_LOCATION_INTERNAL: 
            return (unsigned int (*)(void))screen_table[screen].address;
            break;
        default:
            halt_failure(FAILURE_CORRUPTSCREENTBL, screen_table[screen].location);
            return NULL;    /* NOTREACHED */
    }
}


void flash_secterase(int addr) {
    setPin(FLASH_E, HIGH);
    setPin(SPI_SCK, LOW);
    setPin(FLASH_E, LOW);

    spi_op(0x20);
    // casts not really necessary
    spi_op((u_int8_t)((addr >> 16) & 0xff));
    spi_op((u_int8_t)((addr >> 8) & 0xff));
    spi_op((u_int8_t)(addr & 0xff));

    setPin(FLASH_E, HIGH);
}
void flash_read(int addr, int len, u_int8_t *buf) {
    setPin(FLASH_E, HIGH);
    setPin(SPI_SCK, LOW);
    setPin(FLASH_E, LOW);

    spi_op(0x3);
    spi_op((u_int8_t)(((addr >> 16) & 0xff)));
    spi_op((u_int8_t)(((addr >> 8) & 0xff)));
    spi_op((u_int8_t)(addr & 0xff));
    for(int i = 0; i < len; i++) {
        buf[i] = spi_op(0x0);
    }

    setPin(FLASH_E, HIGH);
}

void flash_bytewrite(int addr, u_int8_t val) {
    setPin(FLASH_E, HIGH);
    setPin(SPI_SCK, LOW);
    setPin(FLASH_E, LOW);

    spi_op(0x2);
    spi_op((u_int8_t)(((addr >> 16) & 0xff)));
    spi_op((u_int8_t)(((addr >> 8) & 0xff)));
    spi_op((u_int8_t)(addr & 0xff));
    spi_op(val);

    setPin(FLASH_E, HIGH);
}
void flash_wren(void) {
    setPin(FLASH_E, HIGH);
    setPin(SPI_SCK, LOW);
    setPin(FLASH_E, LOW);

    spi_op(0x6);
    setPin(FLASH_E, HIGH);
}
u_int8_t spi_op(u_int8_t outb) {
    u_int8_t inb = 0;
    u_int8_t i;
    for(i = 0; i < 8; i++) {
        setPin(SPI_SCK, LOW);
        if(getPin(SPI_MISO) == HIGH) {
            inb |= (1 << (7-i));
        }
        setPin(SPI_MOSI, ((((outb >> (7-i)) & 0x1) == 1) ? HIGH : LOW));
        setPin(SPI_SCK, HIGH);
    }
    return inb;
}

unsigned int get_enemy_id(unsigned int isboss) {
    unsigned int enemy;
    switch(isboss) {
        case 4:
            enemy = ENEMY_PIRATE;
            break;
        case 6:
            enemy = ENEMY_REPORTER;
            break;
        case 7:
            enemy = ENEMY_POOL2;
            break;
        case 9:
            enemy = ENEMY_RIGHTSENSEI;
            break;
        case 10:
            enemy = ENEMY_ECHELON;
            break;
        default:
            enemy = ENEMY_RIGHTNINJA;    // shouldn't ever happen
            break;
    }
    return enemy;
}

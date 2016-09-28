#include <mc1322x.h>
#include <globals.h>
#include <display.h>
#include <board.h>
#include <ext.h>
#include <debug.h>
#include <info.h>
#include <misc.h>
#include <put.h>

volatile player_info playerinfo;
volatile badge_info badgeinfo;
volatile int rand_needs_seed;

nvmType_t nvm_type;

void info_init(void) {
    nvmErr_t err;
    nvmType_t type=0;
    err = nvm_detect(gNvmInternalInterface_c, &type);
    if(type == 0) {
        halt_failure(FAILURE_NVM_INIT, err);
    }
    nvm_type = type;
    memset((void *)&playerinfo, 0, sizeof(playerinfo));
    memset((void *)&badgeinfo, 0, sizeof(badgeinfo));
}

int write_nvm(u_int32_t dest_addr, void *data, unsigned int len) {
    nvmErr_t err;
    uint32_t erasemask = 0, xpos = 0;
    for(xpos = dest_addr; xpos < (dest_addr + len); xpos += (0x1000 - (xpos & 0xfff))) {
        erasemask |= (1 << (xpos/0x1000));
    }
    err = nvm_erase(gNvmInternalInterface_c, nvm_type, erasemask);
    if(err != gNvmErrNoError_c) {
        DEBUG_INFO("unable to erase nvm ");
        DEBUG32_INFO(dest_addr);
        DEBUG_INFO(" len ");
        DEBUG32_INFO(len);
        return 0;
    }
    err = nvm_write(gNvmInternalInterface_c, nvm_type, data, dest_addr, len);
    if(err != gNvmErrNoError_c) {
        DEBUG_INFO("unable to write nvm ");
        DEBUG32_INFO(dest_addr);
        DEBUG_INFO(" len ");
        DEBUG32_INFO(len);
        return 0;
    }
    err = nvm_verify(gNvmInternalInterface_c, nvm_type, data, dest_addr, len);
    if(err != gNvmErrNoError_c) {
        DEBUG_INFO("unable to verify nvm ");
        DEBUG32_INFO(dest_addr);
        DEBUG_INFO(" len ");
        DEBUG32_INFO(len);
        return 0;
    }
    return 1;
}
void info_commit_playerinfo(void) {
    if(!write_nvm(PLAYERINFO_FLASH_ADDR, (void *)&playerinfo, sizeof(player_info))) {
        DEBUG_INFO("can't commit player info!\r\n");
    }
}

void info_load(void) {
    nvmErr_t err;
    err = nvm_read(gNvmInternalInterface_c, nvm_type, (uint8_t *)&badgeinfo, BADGEINFO_FLASH_ADDR, sizeof(badge_info));
    if(err != gNvmErrNoError_c) {
        halt_failure(FAILURE_NVM_INIT, err);
    }
    err = nvm_read(gNvmInternalInterface_c, nvm_type, (uint8_t *)&playerinfo, PLAYERINFO_FLASH_ADDR, sizeof(player_info));
    if(err != gNvmErrNoError_c) {
        halt_failure(FAILURE_NVM_INIT, err);
    }
    if(playerinfo.name_set_flag && playerinfo.player_level > 10) {
        /**
         * the playerinfo page is invalid.  
         */
        if(badgeinfo.badgeid == 0xffffffff) {
            display_clear();
            smallfont_write("FLASH ERROR.  SORRY", 19, 15, 20);
            display_refresh();
            halt_failure(FAILURE_CORRUPTINFO, 2);
        }
        memset((char *)&playerinfo, 0, sizeof(playerinfo));
        playerinfo.player_level = 1;
        playerinfo.hp = 1337;
        playerinfo.name_set_flag = 0;
        info_commit_playerinfo();
    }

    // XXX: remove
    //playerinfo.name[0] = 'A';
    //playerinfo.name[1] = 'B';
    //playerinfo.name[2] = 'C';
    //playerinfo.name[3] = 'D';
    //playerinfo.name[4] = 0;
    //playerinfo.redbull_count = 10;
    //playerinfo.shuriken_count = 10;
    //playerinfo.item_mask = 0;
    //playerinfo.hp = 1337;
    //playerinfo.buzzer_disabled = 0;
    //playerinfo.xp = 0;
    //playerinfo.player_level = 1;
    //playerinfo.name_set_flag = 1;
    //badgeinfo.badgeid = 0x7a691337;
    //badgeinfo.plusone = 0x12345678;
    //badgeinfo.badgetype = BADGE_NORMAL;
    //badgeinfo.is_boss = 4;
    info_commit_playerinfo();

    // XXX: pick something better
    *MACA_RANDOM = badgeinfo.badgeid;   
    rand_needs_seed = 0;
}

unsigned int info_isvalid(void) {
    return playerinfo.name_set_flag;
}

unsigned int calc_strength(unsigned int player_level, unsigned int item_mask) {
    unsigned int base = 12+((player_level-1) * 2);
    if(item_mask & (1 << 3)) {
        return base * 110 / 100;
    }
    return base;
}
unsigned int calc_defense(unsigned int player_level, unsigned int item_mask) {
    unsigned int base = 5+player_level;
    if(item_mask & 1) {
        return base * 110 / 100;
    }
    return base;
}
unsigned int calc_agility(unsigned int player_level, unsigned int item_mask) {
    unsigned int base = 7+player_level;
    if(item_mask & (1 << 2)) {
        return base * 110 / 100;
    }
    return base;
}
unsigned int calc_spirit(unsigned int player_level, unsigned int item_mask) {
    unsigned int base = 19+player_level;
    if(item_mask & (1 << 1)) {
        return base * 110 / 100;
    }
    return base;
}

void save_playerinfo(void) {
    memcpy((player_info *)&globals.saved_playerinfo, (player_info *)&playerinfo, 
            sizeof(globals.saved_playerinfo));
}
void restore_playerinfo(void) {
    memcpy((player_info *)&playerinfo, (player_info *)&globals.saved_playerinfo, 
            sizeof(playerinfo));
}
u_int16_t base_xp[10] = {
    0,
    400,
    800,
    1200,
    1800,
    2600,
    3600,
    4800,
    6200,
    7800
};
unsigned int calc_level(unsigned int xp) {
    if(xp >= 7800) {
        return 10;
    }
    if(xp >= 6200) {
        return 9;
    } 
    if(xp >= 4800) {
        return 8;
    }
    if(xp >= 3600) {
        return 7;
    }
    if(xp >= 2600) {
        return 6;
    }
    if(xp >= 1800) {
        return 5;
    }
    if(xp >= 1200) {
        return 4;
    }
    if(xp >= 800) {
        return 3;
    }
    if(xp >= 400) {
        return 2;
    }
    return 1;
}

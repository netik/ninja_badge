#include <mc1322x.h>
#include <board.h>
#include <ext.h>
#include <pinio.h>
#include <pinout.h>
#include "config.h"
#include <buzzer.h>
#include <misc.h>
#include "put.h"
#include <lock.h>
#include <debug.h>
#include <task.h>
#include "syscalls.h"
#include <screen-splash.h>
#include <display.h>
#include <info.h>
#include <boot_up_screen.h>
#include <splash_screen1.h>

unsigned int screen_splash(void) {
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    display_clear();
    display_copy((char *)boot_up_screen, 0, 0, 7, 0, 113, 27);
    display_refresh();
    for(unsigned int i = 0; i < 19; i++) {
        display_set_line(i);
        usleep(100000);
    }

    buzzer_stop(); 
    buzzer_init();
    buzzer_load(8571, 15389);
    buzzer_start();
    clockusdelay(20000);
    buzzer_reload(875, 1125);
    clockusdelay(40000);
    buzzer_reload(200, 500);
    clockusdelay(80000);
    buzzer_reload(905, 1525);
    clockusdelay(60000);
    buzzer_stop();


    clockusdelay(1500000);
    display_clear();
    display_refresh();
    display_set_line(0);
    display_copy((char *)splash_screen1, 0, 0, 0, 0, 128, 64);
    display_refresh();
    clockusdelay(5000000);
    if(playerinfo.name_set_flag == 1) {
        return SCREEN_MAIN;
    } else {
        return SCREEN_INPUT_USER;
    }
}

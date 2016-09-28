#include <mc1322x.h>
#include <board.h>
#include <ext.h>
#include <pinio.h>
#include <pinout.h>
#include "config.h"
#include <misc.h>
#include <info.h>
#include "put.h"
#include <debug.h>
#include <task.h>
#include "syscalls.h"

void buzzer_reload(unsigned short comp1, unsigned short comp2) {
    *TMR3_CMPLD1 = comp1;
    *TMR3_CMPLD2 = comp2;
}

void buzzer_load(unsigned short comp1, unsigned short comp2) {
    *TMR3_COMP1 = comp1;
    *TMR3_CMPLD1 = comp1;
    *TMR3_COMP2 = comp2;
    *TMR3_CMPLD2 = comp2;
}

void buzzer_init(void) {
    if(playerinfo.buzzer_disabled == 1) {
        return;
    }
    *TMR3_LOAD = 0;
    *TMR3_CNTR = 0;
    *TMR3_SCTRL = ((1 << 2) | 1);
    *TMR3_CSCTRL = 0x6;     // XXX: ??
    *TMR_ENBL |= (1 << 3);
}

void buzzer_start(void) {
    *TMR3_CTRL = (0x2024 | (0xb << 9));
}
void buzzer_stop(void) {
    *TMR_ENBL &= ~(1 << 3);
}
void play_attacked_tone(void) {
    buzzer_stop(); 
    buzzer_init();
    buzzer_load(200, 500);
    buzzer_start();
    for(int i = 0; i < 10; i++) {
        usleep(50000);
        buzzer_reload(875, 925);
        usleep(10000);
        buzzer_reload(875, 1500);
        usleep(20000);
        buzzer_reload(200, 500);
    }
    buzzer_stop();
}
void play_dodge_tone(void) {
    buzzer_stop(); 
    buzzer_init();
    buzzer_load(2500, 5000);
    buzzer_start();
    usleep(150000);
    buzzer_stop();
}
void play_hit_tone(void) {
    buzzer_stop(); 
    buzzer_init();
    buzzer_load(800, 1400);
    buzzer_start();
    usleep(50000);
    buzzer_reload(500, 900);
    usleep(50000);
    buzzer_reload(2500, 5000);
    usleep(50000);
    buzzer_stop();
}

void play_victory_tone(void) {
    buzzer_stop(); 
    for(int i = 0; i < 3; i++) {
        buzzer_init();
        buzzer_load(2000, 5000);
        buzzer_start();
        usleep(70000);
        buzzer_stop();
        usleep(40000);
    }
    buzzer_init();
    buzzer_load(1300, 1750);
    buzzer_start();
    usleep(150000);
    buzzer_stop();
}
void play_defeat_tone(void) {
    buzzer_stop(); 
    buzzer_init();
    buzzer_load(1400, 1950);
    buzzer_start();
    usleep(250000);
    buzzer_reload(3500, 7000);
    usleep(500000);
    buzzer_stop();

}

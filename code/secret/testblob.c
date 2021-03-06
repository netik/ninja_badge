#include <stdio.h>
#include "blobs.h"
#include <string.h>

int main(int argc, char *argv[]) {
    char buf[8192];
    memset(buf, 0, sizeof(buf));
    player_info pi;


    pi.player_level = 1;
    pi.hp = 1337;
    memcpy(&buf[4096], &pi, sizeof(pi));

    badge_info bi;
    bi.badgeid = 0x7a691337;
    bi.badgetype = BADGE_NORMAL;
    bi.is_ninja = 0;
    bi.is_boss = 0;
    bi.levelnonce = 0x12345678;
    bi.itemnonce[0] = 0x22222222;
    bi.itemnonce[1] = 0x22222222;
    bi.itemnonce[2] = 0x22222222;
    bi.itemnonce[3] = 0x22222222;
    bi.itemnonce[4] = 0x22222222;
    bi.itemnonce[5] = 0x22222222;
    bi.itemnonce[6] = 0x22222222;
    bi.itemnonce[7] = 0x22222222;
    bi.itemnonce[8] = 0x22222222;
    bi.itemnonce[9] = 0x22222222;
    memcpy(buf, &bi, sizeof(bi));
    FILE *fp = fopen("infoblob", "w+");
    if(fp == NULL) {
        perror("fopen");
        return 1;
    }
    if(fwrite(buf, sizeof(buf), 1, fp) < 1) {
        perror("fwrite");
        return 1;
    }
    fclose(fp);
    return 0;
}

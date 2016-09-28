#include <stdio.h>
#include "blobs.h"
#include "noncetable.h"
#include <string.h>

int main(int argc, char *argv[]) {
    for(int i = 0; i < 100; i++) {
        char buf[8192];
        memset(buf, 0, sizeof(buf));
        player_info pi;
        memset(&pi, 0, sizeof(pi));

        pi.item_mask = 0x3ff;
        pi.player_level = 10;
        pi.hp = 1337;
        memcpy(&buf[4096], &pi, sizeof(pi));

        badge_info bi;
        memset(&bi, 0, sizeof(bi));
        bi.badgeid = noncetable[i].badgeid;
        bi.badgetype = BADGE_NINJA;
        bi.is_ninja = 1;
        bi.is_boss = 9;
        bi.levelnonce = noncetable[i].levelnonce;
        for(int j = 0; j < 10; j++) {
            bi.itemnonce[j] = noncetable[i].itemnonce[j];
        }
        memcpy(buf, &bi, sizeof(bi));
        char name[100];
        snprintf(name, sizeof(name), "info.%d", i);
        FILE *fp = fopen(name, "w+");
        if(fp == NULL) {
            perror("fopen");
            return 1;
        }
        if(fwrite(buf, sizeof(buf), 1, fp) < 1) {
            perror("fwrite");
            return 1;
        }
        fclose(fp);
    }
    return 0;
}

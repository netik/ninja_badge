#include <stdio.h>
#include "blobs.h"
#include "noncetable.h"
#include <string.h>


int main(int argc, char *argv[]) {
    char ids[4096];
    memset(ids, 0, sizeof(ids));
    for(int i = 0; i < 1000; i++) {
        memcpy(&ids[i*4], &noncetable[i].badgeid, 4);
    }
    FILE *ifp = fopen("badgeids", "w+");
    if(ifp == NULL) {
        perror("fopen");
        return 1;
    }
    if(fwrite(ids, sizeof(ids), 1, ifp) < 1) {
        perror("fwrite");
        return 1;
    }
    fclose(ifp);

    for(int i = 0; i < 11; i++) {
        char buf[4096];
        memset(buf, 0, sizeof(buf));
        for(int j = 0; j < 1000; j++) {
            if(i == 10) {
                memcpy(&buf[j*4], &noncetable[j].levelnonce, 4);
            } else {
                memcpy(&buf[j*4], &noncetable[j].itemnonce[i], 4);
            }
        }
        char name[100];
        snprintf(name, sizeof(name), "spi.%d", i);
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
}

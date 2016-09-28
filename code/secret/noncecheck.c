#include <stdio.h>
#include <stdlib.h>
#include "noncetable.h"

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("usage: %s boss\n", argv[0]);
        exit(1);
    }
    int x = atoi(argv[1]);
    printf("/* nonces for boss %d */\n", x);
    printf("unsigned int bossnoncecount = 1000;\n");
    printf("struct nonce bossnonces[] = {\n");
    for(int i = 0; i < 1000; i++) {
        printf("    { 0x%08x, 0x%08x }, /* %d */\n", noncetable[i].badgeid, 
                (x > 9) ? noncetable[i].levelnonce : noncetable[i].itemnonce[x], i);
    }
    printf("};\n");
}

#include <stdio.h>
#include <stdlib.h>
#include "noncetable.h"

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("usage: %s badge\n", argv[0]);
        exit(1);
    }
    int x = atoi(argv[1]);
    if(x < 0 || x >= 1000) {
        return 1;
    }
    printf("struct nonce {\n"
           "    unsigned int badgeid;\n"
           "    unsigned int nonce;\n"
           "} bossnonces[] = {\n");
    for(int i = 0; i < 1000; i++) {
        printf("    { 0x%08x, 0x%08x }, /* %d */\n", noncetable[i].badgeid, 
                (x > 9) ? noncetable[i].levelnonce : noncetable[i].itemnonce[x], i);
    }
    printf("};\n");
    return 0;
}

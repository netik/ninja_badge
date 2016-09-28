#include <openssl/rand.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    unsigned char buf[16];
    if(RAND_bytes(buf, sizeof(buf))) {
        for(int i = 0; i < sizeof(buf); i++) {
            printf("%x ", buf[i]);
        }
        printf("\n");
    } else {
        printf("error\n");
    }
}


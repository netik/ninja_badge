#include <openssl/rand.h>
#include <stdio.h>

struct nonces {
    unsigned int badgeid;
    unsigned int levelnonce;
    unsigned int itemnonce[10]; 
};

int main(int argc, char *argv[]) {
    printf("struct nonces noncetable[] = {\n");
    for(unsigned int i = 0; i < 1000; i++) {
        unsigned int buf[12];
        if(RAND_bytes((unsigned char *)buf, sizeof(buf))) {
            printf("    { 0x%08x, 0x%08x, {                  /* %d */",buf[0], buf[1], i);
            for(int j = 0; j < 10; j++) {
                if(j % 5 == 0) {
                    printf("\n       ");
                }
                printf(" 0x%08x,", buf[j+2]);
            }
            printf("\n        }\n");
            printf("    },\n");
        } else {
            printf("error\n");
            exit(1);
        }
    }
    printf("};\n");
}


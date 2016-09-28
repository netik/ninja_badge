#include <mc1322x.h>
#include <types.h>
#include <put.h>
#include <stdbool.h>

bool supercheck(void) {
    register int cpsr;
    asm("mrs %[cpsr], cpsr" 
            : [cpsr] "=r" (cpsr));
    if((cpsr & 0x1f) == 0x13) {
        return true;
    }
    return false;
}

void realswi(int __attribute__((unused)) a, __attribute__((unused)) int b, __attribute__((unused)) int c) {
    __asm__("swi #0x42");
}

#if 0
__attribute__ ((section (".swi")))
__attribute__ ((interrupt("SWI"))) 
void software_interrupt(int a, int b, int c) {
    int val;
    register int retval = 0;
    register int spsr;
    asm("mrs %[spsr], spsr\n\t" 
        "tst %[spsr], #0x20\n\t"
        "ldrneh %[spsr], [lr, #-2]\n\t"
        "bicne %[spsr], %[spsr], #0xff00\n\t"
        "ldreq %[spsr], [lr, #-4]\n\t"
        "biceq %[spsr], %[spsr], #0xff000000\n\t"
            : [spsr] "=r" (spsr));
    val = spsr & 0xff;
    if(val == 0) {      /* usleep */
        putstr("hi\r\n");
    }
    a = a;
    b = b;
    c = c;

    supercheck();
    asm("mov r7, %[retval]"
            :
            : [retval] "g" (retval)
       );
}
#endif


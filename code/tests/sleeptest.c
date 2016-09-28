#include <mc1322x.h>
#include <board.h>
#include <pinio.h>
#include "config.h"
#include "put.h"

void delaycs(uint32_t centis) {
    volatile uint32_t count = *CRM_RTC_COUNT;
    count += (centis * 207);
    while((*CRM_RTC_COUNT) < count) {
        continue;
    }
}

void main(void) {
    volatile uint32_t count;
    uart1_init(INC,MOD,SAMP);
    vreg_init();
    while(1) {
        putstr("count now:");
        count = *CRM_RTC_COUNT;
        put_hex32(count);
        putstr("\r\n");

        delaycs(1000);
    }
}

#include <mc1322x.h>
#include <board.h>
#include <ext.h>
#include <pinio.h>
#include <pinout.h>
#include "config.h"
#include <misc.h>
#include "put.h"
#include <lock.h>
#include <debug.h>
#include <task.h>
#include "syscalls.h"
#include <screen-loadfail.h>
#include <info.h>

unsigned int screen_load_failure(void) {
    putstr("XXX: load failure\r\n");
    while(1) {
        continue;
    }
    return 0;   /* NOTREACHED */
}

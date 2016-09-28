#include <nseg.h>
#include <display.h>
#include "intro.h"


void intro_load_cb(const struct nseg *const ns, u_int32_t prevval) {
}

void intro_ready_cb(const struct nseg *const ns) {
    display_clear();
    int bmpw, bmph, startx = 64-(bmpw/2), y = 0;
    display_load_full_bitmap(BMP_NINJA, startx, y);
}

u_int32_t intro_unload_cb(const struct nseg *const ns) {
}

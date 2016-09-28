#include <types.h>
#include "nseg.h"

static const nseg nsegs[] = {
};
#define NNSEGS sizeof(nsegs)/sizeof(nseg);

static const nseg *curnseg = NULL;

void nseg_load(int id) {
    u_int32_t prop = 0;
    if(curnseg != NULL && curnseg->unload_cb != NULL) {
        prop = curnseg->unload_cb(curnseg);
    }
    curnseg = &nsegs[id];
    if(curnseg->load_cb != NULL) {
        curnseg->load_cb(curnseg, prop);
    }
    if(curnseg->ready_cb != NULL) {
        curnseg->ready_cb(curnseg);
    }
}

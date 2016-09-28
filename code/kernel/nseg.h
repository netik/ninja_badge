#ifndef NSEG_H
#define NSEG_H

typedef struct nseg {
    int id;
    void (*load_cb)(const struct nseg *this, u_int32_t prevval);   // for loading external data, etc.
    void (*ready_cb)(const struct nseg *this);  // when the UI is ready for control by the nseg
    u_int32_t (*unload_cb)(const struct nseg *this);   // called before seg will be unloaded
} nseg;

#endif

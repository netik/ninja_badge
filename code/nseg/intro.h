#define NSEG_INTRO 0

void intro_load_cb(const struct nseg *const ns, u_int32_t prevval);
void intro_ready_cb(const struct nseg *const ns);
u_int32_t intro_unload_cb(const struct nseg *const ns);

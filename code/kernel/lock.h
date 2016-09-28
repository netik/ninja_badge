#ifndef LOCK_H
#define LOCK_H

typedef u_int32_t lock_t;

void lock_init(lock_t *lock);
void lock_wait(lock_t *lock);
int lock_nowait(lock_t *lock);
int lock_delay(lock_t *lock, unsigned long usecs);
void lock_unlock(lock_t *unlock);

#endif /* LOCK_H */

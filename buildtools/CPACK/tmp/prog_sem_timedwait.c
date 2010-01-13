#include <semaphore.h>
sem_t *s;
const struct timespec * t;
sem_timedwait(s, t);

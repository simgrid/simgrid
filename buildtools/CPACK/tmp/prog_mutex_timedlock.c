#include <pthread.h>
pthread_mutex_t s;
const struct timespec t;
sem_timedlock(&s, &t);

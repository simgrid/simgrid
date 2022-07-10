/* SimGrid's pthread interposer. Intercepts most of the pthread and semaphore calls to model-check them.
 *
 * Intercepting on pthread is somewhat complicated by the fact that pthread is used everywhere in the system headers.
 * To reduce definition conflicts, our redefinitions of the pthread symbols (in sthread.c) load as little headers as
 * possible. Thus, the actual implementations are separated in another file (sthread_impl.cpp) and are used as black
 * boxes by our redefinitions.
 *
 * The sthread_* symbols are those actual implementations, used in the pthread_* redefinitions. */

#ifndef SIMGRID_STHREAD_H
#define SIMGRID_STHREAD_H

#include <sys/time.h>

#if defined(__ELF__)
#define XBT_PUBLIC __attribute__((visibility("default")))
#else
#define XBT_PUBLIC
#endif

#if defined(__cplusplus)
extern "C" {
#endif

// Launch the simulation. The old main function (passed as a parameter) is launched as an actor
int sthread_main(int argc, char** argv, char** envp, int (*raw_main)(int, char**, char**));
XBT_PUBLIC void sthread_enable(void);  // Start intercepting all pthread calls
XBT_PUBLIC void sthread_disable(void); // Stop intercepting all pthread calls

typedef unsigned long int sthread_t;
int sthread_create(sthread_t* thread, const /*pthread_attr_t*/ void* attr, void* (*start_routine)(void*), void* arg);
int sthread_join(sthread_t thread, void** retval);

typedef struct {
  void* mutex;
} sthread_mutex_t;
int sthread_mutex_init(sthread_mutex_t* mutex, const /*pthread_mutexattr_t*/ void* attr);
int sthread_mutex_lock(sthread_mutex_t* mutex);
int sthread_mutex_trylock(sthread_mutex_t* mutex);
int sthread_mutex_unlock(sthread_mutex_t* mutex);
int sthread_mutex_destroy(sthread_mutex_t* mutex);

int sthread_gettimeofday(struct timeval* tv, struct timezone* tz);
void sthread_sleep(double seconds);

#if defined(__cplusplus)
}
#endif

#endif
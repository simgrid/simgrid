/* SimGrid's pthread interposer. Intercepts most of the pthread and semaphore calls to model-check them.
 *
 * Intercepting on pthread is somewhat complicated by the fact that pthread is used everywhere in the system headers.
 * To reduce definition conflicts, our redefinitions of the pthread symbols (in sthread.c) load as little headers as
 * possible. Thus, the actual implementations are separated in another file (sthread_impl.cpp) and are used as black
 * boxes by our redefinitions.
 *
 * The sthread_* symbols are those actual implementations, used in the pthread_* redefinitions. */

#if defined(__cplusplus)
extern "C" {
#endif
extern volatile int sthread_inside_simgrid; // Only intercept pthread calls in user code

int sthread_main(int argc, char** argv, char** envp, int (*raw_main)(int, char**, char**));

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

#if defined(__cplusplus)
}
#endif

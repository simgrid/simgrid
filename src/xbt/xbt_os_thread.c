/* xbt_os_thread -- portability layer over the pthread API                  */
/* Used in RL to get win/lin portability, and in SG when CONTEXT_THREAD     */
/* in SG, when using CONTEXT_UCONTEXT, xbt_os_thread_stub is used instead   */

/* Copyright (c) 2007-2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "internal_config.h"
#include "xbt/sysdep.h"
#include "xbt/ex.h"
#include "xbt/ex_interface.h"   /* We play crude games with exceptions */
#include "portable.h"
#include "xbt/xbt_os_time.h"    /* Portable time facilities */
#include "xbt/xbt_os_thread.h"  /* This module */
#include "xbt_modinter.h"       /* Initialization/finalization of this module */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_sync_os, xbt,
                                "Synchronization mechanism (OS-level)");

/* ********************************* PTHREAD IMPLEMENTATION ************************************ */
#ifdef HAVE_PTHREAD_H

#include <semaphore.h>

#ifdef HAVE_MUTEX_TIMEDLOCK
/* redefine the function header since we fail to get this from system headers on amd (at least) */
int pthread_mutex_timedlock(pthread_mutex_t * mutex,
                            const struct timespec *abs_timeout);
#endif


/* use named sempahore when sem_init() does not work */
#ifndef HAVE_SEM_INIT
static int next_sem_ID = 0;
static xbt_os_mutex_t next_sem_ID_lock;
#endif

typedef struct xbt_os_thread_ {
  pthread_t t;
  int detached;
  char *name;
  void *param;
  pvoid_f_pvoid_t start_routine;
  xbt_running_ctx_t *running_ctx;
  void *extra_data;
} s_xbt_os_thread_t;
static xbt_os_thread_t main_thread = NULL;

/* thread-specific data containing the xbt_os_thread_t structure */
static pthread_key_t xbt_self_thread_key;
static int thread_mod_inited = 0;

/* attribute structure to handle pthread stack size changing */
//FIXME: find where to put this
static pthread_attr_t attr;
static int thread_attr_inited = 0;

/* frees the xbt_os_thread_t corresponding to the current thread */
static void xbt_os_thread_free_thread_data(xbt_os_thread_t thread)
{
  if (thread == main_thread)    /* just killed main thread */
    main_thread = NULL;

  free(thread->running_ctx);
  free(thread->name);
  free(thread);
}

/* callback: context fetching */
static xbt_running_ctx_t *_os_thread_get_running_ctx(void)
{
  return xbt_os_thread_self()->running_ctx;
}

/* callback: termination */
static void _os_thread_ex_terminate(xbt_ex_t * e)
{
  xbt_ex_display(e);
  xbt_abort();
  /* FIXME: there should be a configuration variable to choose to kill everyone or only this one */
}

void xbt_os_thread_mod_preinit(void)
{
  int errcode;

  if (thread_mod_inited)
    return;

  if ((errcode = pthread_key_create(&xbt_self_thread_key, NULL)))
    THROWF(system_error, errcode,
           "pthread_key_create failed for xbt_self_thread_key");
  
  main_thread = xbt_new(s_xbt_os_thread_t, 1);
  main_thread->name = (char *) "main";
  main_thread->start_routine = NULL;
  main_thread->param = NULL;
  main_thread->running_ctx = xbt_new(xbt_running_ctx_t, 1);
  XBT_RUNNING_CTX_INITIALIZE(main_thread->running_ctx);

  if ((errcode = pthread_setspecific(xbt_self_thread_key, main_thread)))
    THROWF(system_error, errcode,
           "pthread_setspecific failed for xbt_self_thread_key");

  
  __xbt_running_ctx_fetch = _os_thread_get_running_ctx;
  __xbt_ex_terminate = _os_thread_ex_terminate;

  thread_mod_inited = 1;

#ifndef HAVE_SEM_INIT
  next_sem_ID_lock = xbt_os_mutex_init();
#endif

}

void xbt_os_thread_mod_postexit(void)
{
  /* FIXME: don't try to free our key on shutdown.
     Valgrind detects no leak if we don't, and whine if we try to */
  //   int errcode;

  //   if ((errcode=pthread_key_delete(xbt_self_thread_key)))
  //     THROWF(system_error,errcode,"pthread_key_delete failed for xbt_self_thread_key");
  free(main_thread->running_ctx);
  free(main_thread);
  main_thread = NULL;
  thread_mod_inited = 0;
#ifndef HAVE_SEM_INIT
  xbt_os_mutex_destroy(next_sem_ID_lock);
#endif

  /* Restore the default exception setup */
  __xbt_running_ctx_fetch = &__xbt_ex_ctx_default;
  __xbt_ex_terminate = &__xbt_ex_terminate_default;
}

/* this function is critical to tesh+mmalloc, don't mess with it */
int xbt_os_thread_atfork(void (*prepare)(void),
                         void (*parent)(void), void (*child)(void))
{
#ifdef WIN32
  THROW_UNIMPLEMENTED; //pthread_atfork is not implemented in pthread.h on windows
#else
  return pthread_atfork(prepare, parent, child);
#endif
}

static void *wrapper_start_routine(void *s)
{
  xbt_os_thread_t t = s;
  int errcode;

  if ((errcode = pthread_setspecific(xbt_self_thread_key, t)))
    THROWF(system_error, errcode,
           "pthread_setspecific failed for xbt_self_thread_key");

  void *res = t->start_routine(t->param);
  if (t->detached)
    xbt_os_thread_free_thread_data(t);
  return res;
}


xbt_os_thread_t xbt_os_thread_create(const char *name,
                                     pvoid_f_pvoid_t start_routine,
                                     void *param,
                                     void *extra_data)
{
  int errcode;

  xbt_os_thread_t res_thread = xbt_new(s_xbt_os_thread_t, 1);
  res_thread->detached = 0;
  res_thread->name = xbt_strdup(name);
  res_thread->start_routine = start_routine;
  res_thread->param = param;
  res_thread->running_ctx = xbt_new(xbt_running_ctx_t, 1);
  XBT_RUNNING_CTX_INITIALIZE(res_thread->running_ctx);
  res_thread->extra_data = extra_data;
  
  if ((errcode = pthread_create(&(res_thread->t), thread_attr_inited!=0? &attr: NULL,
                                wrapper_start_routine, res_thread)))
    THROWF(system_error, errcode,
           "pthread_create failed: %s", strerror(errcode));



  return res_thread;
}


void xbt_os_thread_setstacksize(int stack_size)
{
  size_t def=0;
  if(stack_size<0)xbt_die("stack size is negative, maybe it exceeds MAX_INT?\n");
  pthread_attr_init(&attr);
  pthread_attr_getstacksize (&attr, &def);
  int res = pthread_attr_setstacksize (&attr, stack_size);
  if ( res!=0 ) {
    if(res==EINVAL)XBT_WARN("Thread stack size is either < PTHREAD_STACK_MIN, > the max limit of the system, or perhaps not a multiple of PTHREAD_STACK_MIN - The parameter was ignored");
    else XBT_WARN("unknown error in pthread stacksize setting");

    pthread_attr_setstacksize (&attr, def);
  }
  thread_attr_inited=1;
}

const char *xbt_os_thread_name(xbt_os_thread_t t)
{
  return t->name;
}

const char *xbt_os_thread_self_name(void)
{
  xbt_os_thread_t me = xbt_os_thread_self();
  return me ? me->name : "main";
}

void xbt_os_thread_join(xbt_os_thread_t thread, void **thread_return)
{

  int errcode;

  if ((errcode = pthread_join(thread->t, thread_return)))
    THROWF(system_error, errcode, "pthread_join failed: %s",
           strerror(errcode));
  xbt_os_thread_free_thread_data(thread);
}

void xbt_os_thread_exit(int *retval)
{
  pthread_exit(retval);
}

xbt_os_thread_t xbt_os_thread_self(void)
{
  xbt_os_thread_t res;

  if (!thread_mod_inited)
    return NULL;

  res = pthread_getspecific(xbt_self_thread_key);

  return res;
}

void xbt_os_thread_key_create(xbt_os_thread_key_t* key) {

  int errcode;
  if ((errcode = pthread_key_create(key, NULL)))
    THROWF(system_error, errcode, "pthread_key_create failed");
}

void xbt_os_thread_set_specific(xbt_os_thread_key_t key, void* value) {

  int errcode;
  if ((errcode = pthread_setspecific(key, value)))
    THROWF(system_error, errcode, "pthread_setspecific failed");
}

void* xbt_os_thread_get_specific(xbt_os_thread_key_t key) {
  return pthread_getspecific(key);
}

void xbt_os_thread_detach(xbt_os_thread_t thread)
{
  thread->detached = 1;
  pthread_detach(thread->t);
}

#include <sched.h>
void xbt_os_thread_yield(void)
{
  sched_yield();
}

void xbt_os_thread_cancel(xbt_os_thread_t t)
{
  pthread_cancel(t->t);
}

/****** mutex related functions ******/
typedef struct xbt_os_mutex_ {
  /* KEEP IT IN SYNC WITH xbt_thread.c */
  pthread_mutex_t m;
} s_xbt_os_mutex_t;

#include <time.h>
#include <math.h>

xbt_os_mutex_t xbt_os_mutex_init(void)
{
  xbt_os_mutex_t res = xbt_new(s_xbt_os_mutex_t, 1);
  int errcode;

  if ((errcode = pthread_mutex_init(&(res->m), NULL)))
    THROWF(system_error, errcode, "pthread_mutex_init() failed: %s",
           strerror(errcode));

  return res;
}

void xbt_os_mutex_acquire(xbt_os_mutex_t mutex)
{
  int errcode;

  if ((errcode = pthread_mutex_lock(&(mutex->m))))
    THROWF(system_error, errcode, "pthread_mutex_lock(%p) failed: %s",
           mutex, strerror(errcode));
}


void xbt_os_mutex_timedacquire(xbt_os_mutex_t mutex, double delay)
{
  int errcode;

  if (delay < 0) {
    xbt_os_mutex_acquire(mutex);

  } else if (delay == 0) {
    errcode = pthread_mutex_trylock(&(mutex->m));

    switch (errcode) {
    case 0:
      return;
    case ETIMEDOUT:
      THROWF(timeout_error, 0, "mutex %p not ready", mutex);
    default:
      THROWF(system_error, errcode,
             "xbt_os_mutex_timedacquire(%p) failed: %s", mutex,
             strerror(errcode));
    }


  } else {

#ifdef HAVE_MUTEX_TIMEDLOCK
    struct timespec ts_end;
    double end = delay + xbt_os_time();

    ts_end.tv_sec = (time_t) floor(end);
    ts_end.tv_nsec = (long) ((end - ts_end.tv_sec) * 1000000000);
    XBT_DEBUG("pthread_mutex_timedlock(%p,%p)", &(mutex->m), &ts_end);

    errcode = pthread_mutex_timedlock(&(mutex->m), &ts_end);

#else                           /* Well, let's reimplement it since those lazy libc dudes didn't */
    double start = xbt_os_time();
    do {
      errcode = pthread_mutex_trylock(&(mutex->m));
      if (errcode == EBUSY)
        xbt_os_thread_yield();
    } while (errcode == EBUSY && xbt_os_time() - start < delay);

    if (errcode == EBUSY)
      errcode = ETIMEDOUT;

#endif                          /* HAVE_MUTEX_TIMEDLOCK */

    switch (errcode) {
    case 0:
      return;

    case ETIMEDOUT:
      THROWF(timeout_error, delay,
             "mutex %p wasn't signaled before timeout (%f)", mutex, delay);

    default:
      THROWF(system_error, errcode,
             "pthread_mutex_timedlock(%p,%f) failed: %s", mutex, delay,
             strerror(errcode));
    }
  }
}

void xbt_os_mutex_release(xbt_os_mutex_t mutex)
{
  int errcode;

  if ((errcode = pthread_mutex_unlock(&(mutex->m))))
    THROWF(system_error, errcode, "pthread_mutex_unlock(%p) failed: %s",
           mutex, strerror(errcode));
}

void xbt_os_mutex_destroy(xbt_os_mutex_t mutex)
{
  int errcode;

  if (!mutex)
    return;

  if ((errcode = pthread_mutex_destroy(&(mutex->m))))
    THROWF(system_error, errcode, "pthread_mutex_destroy(%p) failed: %s",
           mutex, strerror(errcode));
  free(mutex);
}

/***** condition related functions *****/
typedef struct xbt_os_cond_ {
  /* KEEP IT IN SYNC WITH xbt_thread.c */
  pthread_cond_t c;
} s_xbt_os_cond_t;

xbt_os_cond_t xbt_os_cond_init(void)
{
  xbt_os_cond_t res = xbt_new(s_xbt_os_cond_t, 1);
  int errcode;
  if ((errcode = pthread_cond_init(&(res->c), NULL)))
    THROWF(system_error, errcode, "pthread_cond_init() failed: %s",
           strerror(errcode));

  return res;
}

void xbt_os_cond_wait(xbt_os_cond_t cond, xbt_os_mutex_t mutex)
{
  int errcode;
  if ((errcode = pthread_cond_wait(&(cond->c), &(mutex->m))))
    THROWF(system_error, errcode, "pthread_cond_wait(%p,%p) failed: %s",
           cond, mutex, strerror(errcode));
}


void xbt_os_cond_timedwait(xbt_os_cond_t cond, xbt_os_mutex_t mutex,
                           double delay)
{
  int errcode;
  struct timespec ts_end;
  double end = delay + xbt_os_time();

  if (delay < 0) {
    xbt_os_cond_wait(cond, mutex);
  } else {
    ts_end.tv_sec = (time_t) floor(end);
    ts_end.tv_nsec = (long) ((end - ts_end.tv_sec) * 1000000000);
    XBT_DEBUG("pthread_cond_timedwait(%p,%p,%p)", &(cond->c), &(mutex->m),
           &ts_end);
    switch ((errcode =
             pthread_cond_timedwait(&(cond->c), &(mutex->m), &ts_end))) {
    case 0:
      return;
    case ETIMEDOUT:
      THROWF(timeout_error, errcode,
             "condition %p (mutex %p) wasn't signaled before timeout (%f)",
             cond, mutex, delay);
    default:
      THROWF(system_error, errcode,
             "pthread_cond_timedwait(%p,%p,%f) failed: %s", cond, mutex,
             delay, strerror(errcode));
    }
  }
}

void xbt_os_cond_signal(xbt_os_cond_t cond)
{
  int errcode;
  if ((errcode = pthread_cond_signal(&(cond->c))))
    THROWF(system_error, errcode, "pthread_cond_signal(%p) failed: %s",
           cond, strerror(errcode));
}

void xbt_os_cond_broadcast(xbt_os_cond_t cond)
{
  int errcode;
  if ((errcode = pthread_cond_broadcast(&(cond->c))))
    THROWF(system_error, errcode, "pthread_cond_broadcast(%p) failed: %s",
           cond, strerror(errcode));
}

void xbt_os_cond_destroy(xbt_os_cond_t cond)
{
  int errcode;

  if (!cond)
    return;

  if ((errcode = pthread_cond_destroy(&(cond->c))))
    THROWF(system_error, errcode, "pthread_cond_destroy(%p) failed: %s",
           cond, strerror(errcode));
  free(cond);
}

void *xbt_os_thread_getparam(void)
{
  xbt_os_thread_t t = xbt_os_thread_self();
  return t ? t->param : NULL;
}

typedef struct xbt_os_sem_ {
#ifndef HAVE_SEM_INIT
  char *name;
#endif
  sem_t s;
  sem_t *ps;
} s_xbt_os_sem_t;

#ifndef SEM_FAILED
#define SEM_FAILED (-1)
#endif

xbt_os_sem_t xbt_os_sem_init(unsigned int value)
{
  xbt_os_sem_t res = xbt_new(s_xbt_os_sem_t, 1);

  /* On some systems (MAC OS X), only the stub of sem_init is to be found.
   * Any attempt to use it leads to ENOSYS (function not implemented).
   * If such a prehistoric system is detected, do the job with sem_open instead
   */
#ifdef HAVE_SEM_INIT
  if (sem_init(&(res->s), 0, value) != 0)
    THROWF(system_error, errno, "sem_init() failed: %s", strerror(errno));
  res->ps = &(res->s);

#else                           /* damn, no sem_init(). Reimplement it */

  xbt_os_mutex_acquire(next_sem_ID_lock);
  res->name = bprintf("/%d", ++next_sem_ID);
  xbt_os_mutex_release(next_sem_ID_lock);

  res->ps = sem_open(res->name, O_CREAT, 0644, value);
  if ((res->ps == (sem_t *) SEM_FAILED) && (errno == ENAMETOOLONG)) {
    /* Old darwins only allow 13 chars. Did you create *that* amount of semaphores? */
    res->name[13] = '\0';
    res->ps = sem_open(res->name, O_CREAT, 0644, value);
  }
  if ((res->ps == (sem_t *) SEM_FAILED))
    THROWF(system_error, errno, "sem_open() failed: %s", strerror(errno));

  /* Remove the name from the semaphore namespace: we never join on it */
  if (sem_unlink(res->name) < 0)
    THROWF(system_error, errno, "sem_unlink() failed: %s",
           strerror(errno));

#endif

  return res;
}

void xbt_os_sem_acquire(xbt_os_sem_t sem)
{
  if (!sem)
    THROWF(arg_error, EINVAL, "Cannot acquire of the NULL semaphore");
  if (sem_wait(sem->ps) < 0)
    THROWF(system_error, errno, "sem_wait() failed: %s", strerror(errno));
}

void xbt_os_sem_timedacquire(xbt_os_sem_t sem, double delay)
{
  int errcode;

  if (!sem)
    THROWF(arg_error, EINVAL, "Cannot acquire of the NULL semaphore");

  if (delay < 0) {
    xbt_os_sem_acquire(sem);
  } else if (delay == 0) {
    errcode = sem_trywait(sem->ps);

    switch (errcode) {
    case 0:
      return;
    case ETIMEDOUT:
      THROWF(timeout_error, 0, "semaphore %p not ready", sem);
    default:
      THROWF(system_error, errcode,
             "xbt_os_sem_timedacquire(%p) failed: %s", sem,
             strerror(errcode));
    }

  } else {
#ifdef HAVE_SEM_WAIT
    struct timespec ts_end;
    double end = delay + xbt_os_time();

    ts_end.tv_sec = (time_t) floor(end);
    ts_end.tv_nsec = (long) ((end - ts_end.tv_sec) * 1000000000);
    XBT_DEBUG("sem_timedwait(%p,%p)", sem->ps, &ts_end);
    errcode = sem_timedwait(sem->s, &ts_end);

#else                           /* Okay, reimplement this function then */
    double start = xbt_os_time();
    do {
      errcode = sem_trywait(sem->ps);
      if (errcode == EBUSY)
        xbt_os_thread_yield();
    } while (errcode == EBUSY && xbt_os_time() - start < delay);

    if (errcode == EBUSY)
      errcode = ETIMEDOUT;
#endif

    switch (errcode) {
    case 0:
      return;

    case ETIMEDOUT:
      THROWF(timeout_error, delay,
             "semaphore %p wasn't signaled before timeout (%f)", sem,
             delay);

    default:
      THROWF(system_error, errcode, "sem_timedwait(%p,%f) failed: %s", sem,
             delay, strerror(errcode));
    }
  }
}

void xbt_os_sem_release(xbt_os_sem_t sem)
{
  if (!sem)
    THROWF(arg_error, EINVAL, "Cannot release of the NULL semaphore");

  if (sem_post(sem->ps) < 0)
    THROWF(system_error, errno, "sem_post() failed: %s", strerror(errno));
}

void xbt_os_sem_destroy(xbt_os_sem_t sem)
{
  if (!sem)
    THROWF(arg_error, EINVAL, "Cannot destroy the NULL sempahore");

#ifdef HAVE_SEM_INIT
  if (sem_destroy(sem->ps) < 0)
    THROWF(system_error, errno, "sem_destroy() failed: %s",
           strerror(errno));
#else
  if (sem_close(sem->ps) < 0)
    THROWF(system_error, errno, "sem_close() failed: %s", strerror(errno));
  xbt_free(sem->name);

#endif
  xbt_free(sem);
}

void xbt_os_sem_get_value(xbt_os_sem_t sem, int *svalue)
{
  if (!sem)
    THROWF(arg_error, EINVAL,
           "Cannot get the value of the NULL semaphore");

  if (sem_getvalue(&(sem->s), svalue) < 0)
    THROWF(system_error, errno, "sem_getvalue() failed: %s",
           strerror(errno));
}

/* ********************************* WINDOWS IMPLEMENTATION ************************************ */

#elif defined(_XBT_WIN32)

#include <math.h>

typedef struct xbt_os_thread_ {
  char *name;
  HANDLE handle;                /* the win thread handle        */
  unsigned long id;             /* the win thread id            */
  pvoid_f_pvoid_t start_routine;
  void *param;
  void *extra_data;
} s_xbt_os_thread_t;

/* so we can specify the size of the stack of the threads */
#ifndef STACK_SIZE_PARAM_IS_A_RESERVATION
#define STACK_SIZE_PARAM_IS_A_RESERVATION 0x00010000
#endif

/* the default size of the stack of the threads (in bytes)*/
#define XBT_DEFAULT_THREAD_STACK_SIZE  4096
static int stack_size=0;
/* key to the TLS containing the xbt_os_thread_t structure */
static unsigned long xbt_self_thread_key;

void xbt_os_thread_mod_preinit(void)
{
  xbt_self_thread_key = TlsAlloc();
}

void xbt_os_thread_mod_postexit(void)
{

  if (!TlsFree(xbt_self_thread_key))
    THROWF(system_error, (int) GetLastError(),
           "TlsFree() failed to cleanup the thread submodule");
}

int xbt_os_thread_atfork(void (*prepare)(void),
                         void (*parent)(void), void (*child)(void))
{
  return 0;
}

static DWORD WINAPI wrapper_start_routine(void *s)
{
  xbt_os_thread_t t = (xbt_os_thread_t) s;
  DWORD *rv;

  if (!TlsSetValue(xbt_self_thread_key, t))
    THROWF(system_error, (int) GetLastError(),
           "TlsSetValue of data describing the created thread failed");

  rv = (DWORD *) ((t->start_routine) (t->param));

  return rv ? *rv : 0;

}


xbt_os_thread_t xbt_os_thread_create(const char *name,
                                     pvoid_f_pvoid_t start_routine,
                                     void *param,
                                     void *extra_data)
{

  xbt_os_thread_t t = xbt_new(s_xbt_os_thread_t, 1);

  t->name = xbt_strdup(name);
  t->start_routine = start_routine;
  t->param = param;
  t->extra_data = extra_data;
  t->handle = CreateThread(NULL, stack_size==0 ? XBT_DEFAULT_THREAD_STACK_SIZE : stack_size,
                           (LPTHREAD_START_ROUTINE) wrapper_start_routine,
                           t, STACK_SIZE_PARAM_IS_A_RESERVATION, &(t->id));

  if (!t->handle) {
    xbt_free(t);
    THROWF(system_error, (int) GetLastError(), "CreateThread failed");
  }

  return t;
}

void xbt_os_thread_setstacksize(int size)
{
  stack_size = size;
}

const char *xbt_os_thread_name(xbt_os_thread_t t)
{
  return t->name;
}

const char *xbt_os_thread_self_name(void)
{
  xbt_os_thread_t t = xbt_os_thread_self();
  return t ? t->name : "main";
}

void xbt_os_thread_join(xbt_os_thread_t thread, void **thread_return)
{

  if (WAIT_OBJECT_0 != WaitForSingleObject(thread->handle, INFINITE))
    THROWF(system_error, (int) GetLastError(),
           "WaitForSingleObject failed");

  if (thread_return) {

    if (!GetExitCodeThread(thread->handle, (DWORD *) (*thread_return)))
      THROWF(system_error, (int) GetLastError(),
             "GetExitCodeThread failed");
  }

  CloseHandle(thread->handle);

  free(thread->name);

  free(thread);
}

void xbt_os_thread_exit(int *retval)
{
  if (retval)
    ExitThread(*retval);
  else
    ExitThread(0);
}

void xbt_os_thread_key_create(xbt_os_thread_key_t* key) {

  *key = TlsAlloc();
}

void xbt_os_thread_set_specific(xbt_os_thread_key_t key, void* value) {

  if (!TlsSetValue(key, value))
    THROWF(system_error, (int) GetLastError(), "TlsSetValue() failed");
}

void* xbt_os_thread_get_specific(xbt_os_thread_key_t key) {
  return TlsGetValue(key);
}

void xbt_os_thread_detach(xbt_os_thread_t thread)
{
  THROW_UNIMPLEMENTED;
}


xbt_os_thread_t xbt_os_thread_self(void)
{
  return TlsGetValue(xbt_self_thread_key);
}

void *xbt_os_thread_getparam(void)
{
  xbt_os_thread_t t = xbt_os_thread_self();
  return t->param;
}


void xbt_os_thread_yield(void)
{
  Sleep(0);
}

void xbt_os_thread_cancel(xbt_os_thread_t t)
{
  if (!TerminateThread(t->handle, 0))
    THROWF(system_error, (int) GetLastError(), "TerminateThread failed");
}

/****** mutex related functions ******/
typedef struct xbt_os_mutex_ {
  /* KEEP IT IN SYNC WITH xbt_thread.c */
  CRITICAL_SECTION lock;
} s_xbt_os_mutex_t;

xbt_os_mutex_t xbt_os_mutex_init(void)
{
  xbt_os_mutex_t res = xbt_new(s_xbt_os_mutex_t, 1);

  /* initialize the critical section object */
  InitializeCriticalSection(&(res->lock));

  return res;
}

void xbt_os_mutex_acquire(xbt_os_mutex_t mutex)
{
  EnterCriticalSection(&mutex->lock);
}

void xbt_os_mutex_timedacquire(xbt_os_mutex_t mutex, double delay)
{
  THROW_UNIMPLEMENTED;
}

void xbt_os_mutex_release(xbt_os_mutex_t mutex)
{

  LeaveCriticalSection(&mutex->lock);

}

void xbt_os_mutex_destroy(xbt_os_mutex_t mutex)
{

  if (!mutex)
    return;

  DeleteCriticalSection(&mutex->lock);
  free(mutex);
}

/***** condition related functions *****/
enum {                          /* KEEP IT IN SYNC WITH xbt_thread.c */
  SIGNAL = 0,
  BROADCAST = 1,
  MAX_EVENTS = 2
};

typedef struct xbt_os_cond_ {
  /* KEEP IT IN SYNC WITH xbt_thread.c */
  HANDLE events[MAX_EVENTS];

  unsigned int waiters_count;   /* the number of waiters                        */
  CRITICAL_SECTION waiters_count_lock;  /* protect access to waiters_count  */
} s_xbt_os_cond_t;

xbt_os_cond_t xbt_os_cond_init(void)
{

  xbt_os_cond_t res = xbt_new0(s_xbt_os_cond_t, 1);

  memset(&res->waiters_count_lock, 0, sizeof(CRITICAL_SECTION));

  /* initialize the critical section object */
  InitializeCriticalSection(&res->waiters_count_lock);

  res->waiters_count = 0;

  /* Create an auto-reset event */
  res->events[SIGNAL] = CreateEvent(NULL, FALSE, FALSE, NULL);

  if (!res->events[SIGNAL]) {
    DeleteCriticalSection(&res->waiters_count_lock);
    free(res);
    THROWF(system_error, 0, "CreateEvent failed for the signals");
  }

  /* Create a manual-reset event. */
  res->events[BROADCAST] = CreateEvent(NULL, TRUE, FALSE, NULL);

  if (!res->events[BROADCAST]) {

    DeleteCriticalSection(&res->waiters_count_lock);
    CloseHandle(res->events[SIGNAL]);
    free(res);
    THROWF(system_error, 0, "CreateEvent failed for the broadcasts");
  }

  return res;
}

void xbt_os_cond_wait(xbt_os_cond_t cond, xbt_os_mutex_t mutex)
{

  unsigned long wait_result;
  int is_last_waiter;

  /* lock the threads counter and increment it */
  EnterCriticalSection(&cond->waiters_count_lock);
  cond->waiters_count++;
  LeaveCriticalSection(&cond->waiters_count_lock);

  /* unlock the mutex associate with the condition */
  LeaveCriticalSection(&mutex->lock);

  /* wait for a signal (broadcast or no) */
  wait_result = WaitForMultipleObjects(2, cond->events, FALSE, INFINITE);

  if (wait_result == WAIT_FAILED)
    THROWF(system_error, 0,
           "WaitForMultipleObjects failed, so we cannot wait on the condition");

  /* we have a signal lock the condition */
  EnterCriticalSection(&cond->waiters_count_lock);
  cond->waiters_count--;

  /* it's the last waiter or it's a broadcast ? */
  is_last_waiter = ((wait_result == WAIT_OBJECT_0 + BROADCAST - 1)
                    && (cond->waiters_count == 0));

  LeaveCriticalSection(&cond->waiters_count_lock);

  /* yes it's the last waiter or it's a broadcast
   * only reset the manual event (the automatic event is reset in the WaitForMultipleObjects() function
   * by the system.
   */
  if (is_last_waiter)
    if (!ResetEvent(cond->events[BROADCAST]))
      THROWF(system_error, 0, "ResetEvent failed");

  /* relock the mutex associated with the condition in accordance with the posix thread specification */
  EnterCriticalSection(&mutex->lock);
}

void xbt_os_cond_timedwait(xbt_os_cond_t cond, xbt_os_mutex_t mutex,
                           double delay)
{

  unsigned long wait_result = WAIT_TIMEOUT;
  int is_last_waiter;
  unsigned long end = (unsigned long) (delay * 1000);


  if (delay < 0) {
    xbt_os_cond_wait(cond, mutex);
  } else {
    XBT_DEBUG("xbt_cond_timedwait(%p,%p,%lu)", &(cond->events),
           &(mutex->lock), end);

    /* lock the threads counter and increment it */
    EnterCriticalSection(&cond->waiters_count_lock);
    cond->waiters_count++;
    LeaveCriticalSection(&cond->waiters_count_lock);

    /* unlock the mutex associate with the condition */
    LeaveCriticalSection(&mutex->lock);
    /* wait for a signal (broadcast or no) */

    wait_result = WaitForMultipleObjects(2, cond->events, FALSE, end);

    switch (wait_result) {
    case WAIT_TIMEOUT:
      THROWF(timeout_error, GetLastError(),
             "condition %p (mutex %p) wasn't signaled before timeout (%f)",
             cond, mutex, delay);
    case WAIT_FAILED:
      THROWF(system_error, GetLastError(),
             "WaitForMultipleObjects failed, so we cannot wait on the condition");
    }

    /* we have a signal lock the condition */
    EnterCriticalSection(&cond->waiters_count_lock);
    cond->waiters_count--;

    /* it's the last waiter or it's a broadcast ? */
    is_last_waiter = ((wait_result == WAIT_OBJECT_0 + BROADCAST - 1)
                      && (cond->waiters_count == 0));

    LeaveCriticalSection(&cond->waiters_count_lock);

    /* yes it's the last waiter or it's a broadcast
     * only reset the manual event (the automatic event is reset in the WaitForMultipleObjects() function
     * by the system.
     */
    if (is_last_waiter)
      if (!ResetEvent(cond->events[BROADCAST]))
        THROWF(system_error, 0, "ResetEvent failed");

    /* relock the mutex associated with the condition in accordance with the posix thread specification */
    EnterCriticalSection(&mutex->lock);
  }
  /*THROW_UNIMPLEMENTED; */
}

void xbt_os_cond_signal(xbt_os_cond_t cond)
{
  int have_waiters;

  EnterCriticalSection(&cond->waiters_count_lock);
  have_waiters = cond->waiters_count > 0;
  LeaveCriticalSection(&cond->waiters_count_lock);

  if (have_waiters)
    if (!SetEvent(cond->events[SIGNAL]))
      THROWF(system_error, 0, "SetEvent failed");

  xbt_os_thread_yield();
}

void xbt_os_cond_broadcast(xbt_os_cond_t cond)
{
  int have_waiters;

  EnterCriticalSection(&cond->waiters_count_lock);
  have_waiters = cond->waiters_count > 0;
  LeaveCriticalSection(&cond->waiters_count_lock);

  if (have_waiters)
    SetEvent(cond->events[BROADCAST]);
}

void xbt_os_cond_destroy(xbt_os_cond_t cond)
{
  int error = 0;

  if (!cond)
    return;

  if (!CloseHandle(cond->events[SIGNAL]))
    error = 1;

  if (!CloseHandle(cond->events[BROADCAST]))
    error = 1;

  DeleteCriticalSection(&cond->waiters_count_lock);

  xbt_free(cond);

  if (error)
    THROWF(system_error, 0, "Error while destroying the condition");
}

typedef struct xbt_os_sem_ {
  HANDLE h;
  unsigned int value;
  CRITICAL_SECTION value_lock;  /* protect access to value of the semaphore  */
} s_xbt_os_sem_t;

#ifndef INT_MAX
# define INT_MAX 32767          /* let's be safe by underestimating this value: this is for 16bits only */
#endif

xbt_os_sem_t xbt_os_sem_init(unsigned int value)
{
  xbt_os_sem_t res;

  if (value > INT_MAX)
    THROWF(arg_error, value,
           "Semaphore initial value too big: %ud cannot be stored as a signed int",
           value);

  res = (xbt_os_sem_t) xbt_new0(s_xbt_os_sem_t, 1);

  if (!(res->h = CreateSemaphore(NULL, value, (long) INT_MAX, NULL))) {
    THROWF(system_error, GetLastError(), "CreateSemaphore() failed: %s",
           strerror(GetLastError()));
    return NULL;
  }

  res->value = value;

  InitializeCriticalSection(&(res->value_lock));

  return res;
}

void xbt_os_sem_acquire(xbt_os_sem_t sem)
{
  if (!sem)
    THROWF(arg_error, EINVAL, "Cannot acquire the NULL semaphore");

  /* wait failure */
  if (WAIT_OBJECT_0 != WaitForSingleObject(sem->h, INFINITE))
    THROWF(system_error, GetLastError(),
           "WaitForSingleObject() failed: %s", strerror(GetLastError()));
  EnterCriticalSection(&(sem->value_lock));
  sem->value--;
  LeaveCriticalSection(&(sem->value_lock));
}

void xbt_os_sem_timedacquire(xbt_os_sem_t sem, double timeout)
{
  long seconds;
  long milliseconds;
  double end = timeout + xbt_os_time();

  if (!sem)
    THROWF(arg_error, EINVAL, "Cannot acquire the NULL semaphore");

  if (timeout < 0) {
    xbt_os_sem_acquire(sem);
  } else {                      /* timeout can be zero <-> try acquire ) */


    seconds = (long) floor(end);
    milliseconds = (long) ((end - seconds) * 1000);
    milliseconds += (seconds * 1000);

    switch (WaitForSingleObject(sem->h, milliseconds)) {
    case WAIT_OBJECT_0:
      EnterCriticalSection(&(sem->value_lock));
      sem->value--;
      LeaveCriticalSection(&(sem->value_lock));
      return;

    case WAIT_TIMEOUT:
      THROWF(timeout_error, GetLastError(),
             "semaphore %p wasn't signaled before timeout (%f)", sem,
             timeout);
      return;

    default:
      THROWF(system_error, GetLastError(),
             "WaitForSingleObject(%p,%f) failed: %s", sem, timeout,
             strerror(GetLastError()));
    }
  }
}

void xbt_os_sem_release(xbt_os_sem_t sem)
{
  if (!sem)
    THROWF(arg_error, EINVAL, "Cannot release the NULL semaphore");

  if (!ReleaseSemaphore(sem->h, 1, NULL))
    THROWF(system_error, GetLastError(), "ReleaseSemaphore() failed: %s",
           strerror(GetLastError()));
  EnterCriticalSection(&(sem->value_lock));
  sem->value++;
  LeaveCriticalSection(&(sem->value_lock));
}

void xbt_os_sem_destroy(xbt_os_sem_t sem)
{
  if (!sem)
    THROWF(arg_error, EINVAL, "Cannot destroy the NULL semaphore");

  if (!CloseHandle(sem->h))
    THROWF(system_error, GetLastError(), "CloseHandle() failed: %s",
           strerror(GetLastError()));

  DeleteCriticalSection(&(sem->value_lock));

  xbt_free(sem);

}

void xbt_os_sem_get_value(xbt_os_sem_t sem, int *svalue)
{
  if (!sem)
    THROWF(arg_error, EINVAL,
           "Cannot get the value of the NULL semaphore");

  EnterCriticalSection(&(sem->value_lock));
  *svalue = sem->value;
  LeaveCriticalSection(&(sem->value_lock));
}


#endif


/** @brief Returns the amount of cores on the current host */
int xbt_os_get_numcores(void) {
#ifdef WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#elif MACOS
    int nm[2];
    size_t len = 4;
    uint32_t count;

    nm[0] = CTL_HW; nm[1] = HW_AVAILCPU;
    sysctl(nm, 2, &count, &len, NULL, 0);

    if(count < 1) {
        nm[1] = HW_NCPU;
        sysctl(nm, 2, &count, &len, NULL, 0);
        if(count < 1) { count = 1; }
    }
    return count;
#else
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}


/***** reentrant mutexes *****/
typedef struct xbt_os_rmutex_ {
  xbt_os_mutex_t mutex;
  xbt_os_thread_t owner;
  int count;
} s_xbt_os_rmutex_t;

void xbt_os_thread_set_extra_data(void *data)
{
  xbt_os_thread_self()->extra_data = data;
}

void *xbt_os_thread_get_extra_data(void)
{
  xbt_os_thread_t self = xbt_os_thread_self();
  return self? self->extra_data : NULL;
}

xbt_os_rmutex_t xbt_os_rmutex_init(void)
{
  xbt_os_rmutex_t rmutex = xbt_new0(struct xbt_os_rmutex_, 1);
  rmutex->mutex = xbt_os_mutex_init();
  rmutex->owner = NULL;
  rmutex->count = 0;
  return rmutex;
}

void xbt_os_rmutex_acquire(xbt_os_rmutex_t rmutex)
{
  xbt_os_thread_t self = xbt_os_thread_self();

  if (self == NULL) {
    /* the thread module is not initialized yet */
    rmutex->owner = NULL;
    return;
  }

  if (self != rmutex->owner) {
    xbt_os_mutex_acquire(rmutex->mutex);
    rmutex->owner = self;
    rmutex->count = 1;
  } else {
    rmutex->count++;
 }
}

void xbt_os_rmutex_release(xbt_os_rmutex_t rmutex)
{
  if (rmutex->owner == NULL) {
    /* the thread module was not initialized */
    return;
  }

  xbt_assert(rmutex->owner == xbt_os_thread_self());

  if (--rmutex->count == 0) {
    rmutex->owner = NULL;
    xbt_os_mutex_release(rmutex->mutex);
  }
}

void xbt_os_rmutex_destroy(xbt_os_rmutex_t rmutex)
{
  xbt_os_mutex_destroy(rmutex->mutex);
  xbt_free(rmutex);
}

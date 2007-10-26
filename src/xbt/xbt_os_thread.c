/* $Id$ */

/* xbt_os_thread -- portability layer over the pthread API                  */
/* Used in RL to get win/lin portability, and in SG when CONTEXT_THREAD     */
/* in SG, when using CONTEXT_UCONTEXT, xbt_os_thread_stub is used instead   */

/* Copyright 2006,2007 Malek Cherier, Martin Quinson
 * All right reserved.                                                      */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/ex.h"
#include "xbt/ex_interface.h" /* We play crude games with exceptions */
#include "portable.h"
#include "xbt/xbt_os_time.h" /* Portable time facilities */
#include "xbt/xbt_os_thread.h" /* This module */
#include "xbt_modinter.h" /* Initialization/finalization of this module */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_sync_os,xbt,"Synchronization mechanism (OS-level)");

/* ********************************* PTHREAD IMPLEMENTATION ************************************ */
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#include <semaphore.h>

/* use named sempahore instead */
#ifndef HAVE_SEM_WAIT
#  define MAX_SEM_NAME ((size_t)13)
   static int __next_sem_ID = 0;
   static pthread_mutex_t __next_sem_ID_lock;
#endif

typedef struct xbt_os_thread_ {
   pthread_t t;
   char *name;
   void *param;
   pvoid_f_pvoid_t start_routine;
   ex_ctx_t *exception;
} s_xbt_os_thread_t ;
static xbt_os_thread_t main_thread = NULL;

/* thread-specific data containing the xbt_os_thread_t structure */
static pthread_key_t xbt_self_thread_key;
static int thread_mod_inited = 0;

/* frees the xbt_os_thread_t corresponding to the current thread */
static void xbt_os_thread_free_thread_data(void*d){
   free(d);
}

/* callback: context fetching */
static ex_ctx_t *_os_thread_ex_ctx(void) {
  return xbt_os_thread_self()->exception;
}

/* callback: termination */
static void _os_thread_ex_terminate(xbt_ex_t * e) {
  xbt_ex_display(e);

  abort();
  /* FIXME: there should be a configuration variable to choose to kill everyone or only this one */
}

void xbt_os_thread_mod_init(void) {
   int errcode;

   if (thread_mod_inited)
     return;

   if ((errcode=pthread_key_create(&xbt_self_thread_key, NULL)))
     THROW0(system_error,errcode,"pthread_key_create failed for xbt_self_thread_key");

   main_thread=xbt_new(s_xbt_os_thread_t,1);
   main_thread->name = (char*)"main";
   main_thread->start_routine = NULL;
   main_thread->param = NULL;
   main_thread->exception = xbt_new(ex_ctx_t, 1);
   XBT_CTX_INITIALIZE(main_thread->exception);

   __xbt_ex_ctx = _os_thread_ex_ctx;
   __xbt_ex_terminate = _os_thread_ex_terminate;

   #ifndef HAVE_SEM_WAIT

   /* initialize the mutex use to protect the incrementation of the variable __next_sem_ID
    * used to build the name of the named sempahore
    */
   if ((errcode = pthread_mutex_init(&__next_sem_ID_lock,NULL)))
     THROW1(system_error,errcode,"pthread_mutex_init() failed: %s",strerror(errcode));

   #endif

   thread_mod_inited = 1;
}
void xbt_os_thread_mod_exit(void) {
   /* FIXME: don't try to free our key on shutdown.
      Valgrind detects no leak if we don't, and whine if we try to */
//   int errcode;

//   if ((errcode=pthread_key_delete(xbt_self_thread_key)))
//     THROW0(system_error,errcode,"pthread_key_delete failed for xbt_self_thread_key");
}

static void * wrapper_start_routine(void *s) {
  xbt_os_thread_t t = s;
  int errcode;

  if ((errcode=pthread_setspecific(xbt_self_thread_key,t)))
    THROW0(system_error,errcode,
	   "pthread_setspecific failed for xbt_self_thread_key");

  return (*(t->start_routine))(t->param);
}

xbt_os_thread_t xbt_os_thread_create(const char*name,
				     pvoid_f_pvoid_t start_routine,
				     void* param)  {
   int errcode;

   xbt_os_thread_t res_thread=xbt_new(s_xbt_os_thread_t,1);
   res_thread->name = xbt_strdup(name);
   res_thread->start_routine = start_routine;
   res_thread->param = param;
   res_thread->exception = xbt_new(ex_ctx_t, 1);
   XBT_CTX_INITIALIZE(res_thread->exception);

   if ((errcode = pthread_create(&(res_thread->t), NULL,
				 wrapper_start_routine, res_thread)))
     THROW1(system_error,errcode,
	    "pthread_create failed: %s",strerror(errcode));

   return res_thread;
}

const char* xbt_os_thread_name(xbt_os_thread_t t) {
   return t->name;
}

const char* xbt_os_thread_self_name(void) {
   xbt_os_thread_t self = xbt_os_thread_self();
   return self?self->name:"main";
}
void
xbt_os_thread_join(xbt_os_thread_t thread,void ** thread_return) {

  int errcode;

  if ((errcode = pthread_join(thread->t,thread_return)))
    THROW1(system_error,errcode, "pthread_join failed: %s",
	   strerror(errcode));
   if (thread->exception)
     free(thread->exception);

   if (thread == main_thread) /* just killed main thread */
     main_thread = NULL;

   free(thread);
}

void xbt_os_thread_exit(int *retval) {
   pthread_exit(retval);
}

xbt_os_thread_t xbt_os_thread_self(void) {
  xbt_os_thread_t res;

  if (!thread_mod_inited)
    return NULL;

  res = pthread_getspecific(xbt_self_thread_key);
  if (!res)
    res = main_thread;

  return res;
}

#include <sched.h>
void xbt_os_thread_yield(void) {
   sched_yield();
}
void xbt_os_thread_cancel(xbt_os_thread_t t) {
   pthread_cancel(t->t);
}
/****** mutex related functions ******/
typedef struct xbt_os_mutex_ {
  /* KEEP IT IN SYNC WITH xbt_thread.c */
   pthread_mutex_t m;
} s_xbt_os_mutex_t;

#include <time.h>
#include <math.h>

xbt_os_mutex_t xbt_os_mutex_init(void) {
   xbt_os_mutex_t res = xbt_new(s_xbt_os_mutex_t,1);
   int errcode;

   if ((errcode = pthread_mutex_init(&(res->m),NULL)))
     THROW1(system_error,errcode,"pthread_mutex_init() failed: %s",
	    strerror(errcode));

   return res;
}

void xbt_os_mutex_acquire(xbt_os_mutex_t mutex) {
   int errcode;

   if ((errcode=pthread_mutex_lock(&(mutex->m))))
     THROW2(system_error,errcode,"pthread_mutex_lock(%p) failed: %s",
	    mutex, strerror(errcode));
}



#ifndef HAVE_PTHREAD_MUTEX_TIMEDLOCK
/* if the function is not availabled or if is MAC OS X use pthread_mutex_trylock() to define
 * it.
 */
#include <time.h> /* declaration of the timespec structure and of the nanosleep() function */
int pthread_mutex_timedlock(pthread_mutex_t * mutex, const struct timespec * abs_timeout)
{
	int rv;
	long ellapsed_time = 0;
	struct timespec ts;

	do
	{
		/* the mutex could not be acquired because it was already locked by an other thread */
		rv = pthread_mutex_trylock(mutex);

		ts.tv_sec = 0;
		ts.tv_nsec = 2.5e7 /* (25 ms (2 context switch + 5 ms)) */;

		do
		{
			nanosleep(&ts, &ts);
		}while(EINTR == errno);

		ellapsed_time += ts.tv_nsec;

	}
	while (/* locked */ (EBUSY == rv) && /* !timeout */ (ellapsed_time < ((long)(abs_timeout->tv_sec * 1e9) + abs_timeout->tv_nsec)));

	return (EBUSY == rv) ? ETIMEDOUT : rv;
}

#endif

void xbt_os_mutex_timedacquire(xbt_os_mutex_t mutex, double delay) {
   int errcode;
   struct timespec ts_end;
   double end = delay + xbt_os_time();
	
   if (delay < 0) {
      xbt_os_mutex_acquire(mutex);
   }
   else if(!delay /* equals 0 */)
   	{
   		if ((errcode=pthread_mutex_trylock(&(mutex->m))))
     		THROW2(system_error,errcode,"pthread_mutex_trylock(%p) failed: %s",mutex, strerror(errcode));
   		
   } else {
      ts_end.tv_sec = (time_t) floor(end);
      ts_end.tv_nsec = (long)  ( ( end - ts_end.tv_sec) * 1000000000);
      DEBUG2("pthread_mutex_timedlock(%p,%p)",&(mutex->m), &ts_end);

   switch ((errcode=pthread_mutex_timedlock(&(mutex->m),&ts_end)))
     {
     	case 0:
		return;

		case ETIMEDOUT:
		THROW2(timeout_error,errcode,"mutex %p wasn't signaled before timeout (%f)",mutex,delay);

		default:
		THROW3(system_error,errcode,"pthread_mutex_timedlock(%p,%f) failed: %s",mutex,delay, strerror(errcode));
		}
    }
}

void xbt_os_mutex_release(xbt_os_mutex_t mutex) {
   int errcode;

   if ((errcode=pthread_mutex_unlock(&(mutex->m))))
     THROW2(system_error,errcode,"pthread_mutex_unlock(%p) failed: %s",
	    mutex, strerror(errcode));
}

void xbt_os_mutex_destroy(xbt_os_mutex_t mutex) {
   int errcode;

   if (!mutex) return;

   if ((errcode=pthread_mutex_destroy(&(mutex->m))))
     THROW2(system_error,errcode,"pthread_mutex_destroy(%p) failed: %s",
	    mutex, strerror(errcode));
   free(mutex);
}

/***** condition related functions *****/
typedef struct xbt_os_cond_ {
  /* KEEP IT IN SYNC WITH xbt_thread.c */
   pthread_cond_t c;
} s_xbt_os_cond_t;

xbt_os_cond_t xbt_os_cond_init(void) {
   xbt_os_cond_t res = xbt_new(s_xbt_os_cond_t,1);
   int errcode;
   if ((errcode=pthread_cond_init(&(res->c),NULL)))
     THROW1(system_error,errcode,"pthread_cond_init() failed: %s",
	    strerror(errcode));

   return res;
}

void xbt_os_cond_wait(xbt_os_cond_t cond, xbt_os_mutex_t mutex) {
   int errcode;
   if ((errcode=pthread_cond_wait(&(cond->c),&(mutex->m))))
     THROW3(system_error,errcode,"pthread_cond_wait(%p,%p) failed: %s",
	    cond,mutex, strerror(errcode));
}


void xbt_os_cond_timedwait(xbt_os_cond_t cond, xbt_os_mutex_t mutex, double delay) {
   int errcode;
   struct timespec ts_end;
   double end = delay + xbt_os_time();

   if (delay < 0) {
      xbt_os_cond_wait(cond,mutex);
   } else {
      ts_end.tv_sec = (time_t) floor(end);
      ts_end.tv_nsec = (long)  ( ( end - ts_end.tv_sec) * 1000000000);
      DEBUG3("pthread_cond_timedwait(%p,%p,%p)",&(cond->c),&(mutex->m), &ts_end);
      switch ( (errcode=pthread_cond_timedwait(&(cond->c),&(mutex->m), &ts_end)) ) {
       case 0:
	 return;
       case ETIMEDOUT:
	 THROW3(timeout_error,errcode,"condition %p (mutex %p) wasn't signaled before timeout (%f)",
		cond,mutex, delay);
       default:
	 THROW4(system_error,errcode,"pthread_cond_timedwait(%p,%p,%f) failed: %s",
		cond,mutex, delay, strerror(errcode));
      }
   }
}

void xbt_os_cond_signal(xbt_os_cond_t cond) {
   int errcode;
   if ((errcode=pthread_cond_signal(&(cond->c))))
     THROW2(system_error,errcode,"pthread_cond_signal(%p) failed: %s",
	    cond, strerror(errcode));
}

void xbt_os_cond_broadcast(xbt_os_cond_t cond){
   int errcode;
   if ((errcode=pthread_cond_broadcast(&(cond->c))))
     THROW2(system_error,errcode,"pthread_cond_broadcast(%p) failed: %s",
	    cond, strerror(errcode));
}
void xbt_os_cond_destroy(xbt_os_cond_t cond){
   int errcode;

   if (!cond) return;

   if ((errcode=pthread_cond_destroy(&(cond->c))))
     THROW2(system_error,errcode,"pthread_cond_destroy(%p) failed: %s",
	    cond, strerror(errcode));
   free(cond);
}

void *xbt_os_thread_getparam(void) {
   xbt_os_thread_t t = xbt_os_thread_self();
   return t?t->param:NULL;
}

typedef struct xbt_os_sem_ {
   #ifndef HAVE_SEM_WAIT
   char* name;
   sem_t* s;
   #else
   sem_t s;
   #endif
}s_xbt_os_sem_t ;

xbt_os_sem_t
xbt_os_sem_init(unsigned int value)
{
	xbt_os_sem_t res = xbt_new(s_xbt_os_sem_t,1);
	int errcode;

	/* On MAC OS X, it seems that the sem_init is failing with ENOSYS,
	 * which means the sem_init function is not implemented use sem_open()
	 * instead
	 */
	#ifndef HAVE_SEM_INIT
	res->name = (char*) calloc(MAX_SEM_NAME + 1,sizeof(char));

	if((errcode = pthread_mutex_lock(&__next_sem_ID_lock)))
		THROW1(system_error,errcode,"pthread_mutex_lock() failed: %s", strerror(errcode));

	__next_sem_ID++;

	if((errcode = pthread_mutex_unlock(&__next_sem_ID_lock)))
		THROW1(system_error,errcode,"pthread_mutex_unlock() failed: %s", strerror(errcode));

	sprintf(res->name,"/%d.%d",(*xbt_getpid)(),__next_sem_ID);

	if((res->s = sem_open(res->name, O_CREAT, 0644, value)) == (sem_t*)SEM_FAILED)
		THROW1(system_error,errno,"sem_open() failed: %s",strerror(errno));
	
	if(sem_unlink(res->name) < 0)
		THROW1(system_error,errno,"sem_unlink() failed: %s", strerror(errno));


	#else
	/* sem_init() is implemented, use it */
	if(sem_init(&(res->s),0,value) < 0)
		THROW1(system_error,errno,"sem_init() failed: %s",
	    strerror(errno));
	#endif

   return res;
}

void
xbt_os_sem_acquire(xbt_os_sem_t sem)
{
	if(!sem)
		THROW0(arg_error,EINVAL,"Cannot acquire of the NULL semaphore");
	#ifndef HAVE_SEM_WAIT
	if(sem_wait((sem->s)) < 0)
		THROW1(system_error,errno,"sem_wait() failed: %s",
	    strerror(errno));
	#else
	if(sem_wait(&(sem->s)) < 0)
		THROW1(system_error,errno,"sem_wait() failed: %s",
	    strerror(errno));
	#endif
}

#ifndef HAVE_SEM_TIMEDWAIT
/* if the function is not availabled or if is MAC OS X use sem_trywait() to define
 * it.
 */
#include <time.h> /* declaration of the timespec structure and of the nanosleep() function */
int sem_timedwait(sem_t* sem, const struct timespec * abs_timeout)
{
	int rv;
	long ellapsed_time = 0;
	struct timespec ts;

	do
	{
		rv = sem_trywait(sem);
		ts.tv_sec = 0;
		ts.tv_nsec = 2.5e7 /* (25 ms (2 * context switch + 5 ms)) */;

		do
		{
			nanosleep(&ts, &ts);
		}while(EINTR == errno);

		ellapsed_time += ts.tv_nsec;

	}
	while(/* locked */ (EAGAIN == rv) && /* !timeout */ ellapsed_time < ((long)(abs_timeout->tv_sec * 1e9) + abs_timeout->tv_nsec));

	return (EAGAIN == rv) ? ETIMEDOUT : rv;
}

#endif

void xbt_os_sem_timedacquire(xbt_os_sem_t sem,double timeout)
{
	int errcode;
	struct timespec ts_end;
	double end = timeout + xbt_os_time();

	if(!sem)
		THROW0(arg_error,EINVAL,"Cannot acquire of the NULL semaphore");

	if (timeout < 0)
	{
		xbt_os_sem_acquire(sem);
	}
	else if(!timeout)
	{
		#ifndef HAVE_SEM_TIMEDWAIT
		if(sem_trywait(sem->s) < 0)
			THROW2(system_error,errno,"sem_trywait(%p) failed: %s",sem,strerror(errno));
		#else
		if(sem_trywait(&(sem->s) < 0)
			THROW2(system_error,errno,"sem_trywait(%p) failed: %s",sem,strerror(errno));
		#endif		
	}
	else
	{
		ts_end.tv_sec = (time_t) floor(end);
		ts_end.tv_nsec = (long)  ( ( end - ts_end.tv_sec) * 1000000000);
		DEBUG2("sem_timedwait(%p,%p)",&(sem->s),&ts_end);

		#ifndef HAVE_SEM_WAIT
		switch ((errcode = sem_timedwait(sem->s,&ts_end)))
		#else
		switch ((errcode = sem_timedwait(&(sem->s),&ts_end)))
		#endif
		{
			case 0:
			return;

			case ETIMEDOUT:
			THROW2(timeout_error,errcode,"semaphore %p wasn't signaled before timeout (%f)",sem,timeout);

			default:
			THROW3(system_error,errcode,"sem_timedwait(%p,%f) failed: %s",sem,timeout, strerror(errcode));
		}
	}

}

void
xbt_os_sem_release(xbt_os_sem_t sem)
{
	if(!sem)
		THROW0(arg_error,EINVAL,"Cannot release of the NULL semaphore");

	#ifndef HAVE_SEM_WAIT
	if(sem_post((sem->s)) < 0)
		THROW1(system_error,errno,"sem_post() failed: %s",
	    strerror(errno));
	#else
	if(sem_post(&(sem->s)) < 0)
		THROW1(system_error,errno,"sem_post() failed: %s",
	    strerror(errno));
	#endif
}

void
xbt_os_sem_destroy(xbt_os_sem_t sem)
{
	if(!sem)
		THROW0(arg_error,EINVAL,"Cannot destroy the NULL sempahore");

	#ifndef HAVE_SEM_WAIT
	/* MAC OS X does not implement the sem_init() function so,
	 * we use the named semaphore (sem_open)
	 */
	if(sem_close((sem->s)) < 0)
		THROW1(system_error,errno,"sem_close() failed: %s",
	    strerror(errno));

	xbt_free(sem->name);

	#else
	if(sem_destroy(&(sem->s)) < 0)
		THROW1(system_error,errno,"sem_destroy() failed: %s",
	    strerror(errno));
	#endif

	xbt_free(sem);
}

void
xbt_os_sem_get_value(xbt_os_sem_t sem, int* svalue)
{
	if(!sem)
		THROW0(arg_error,EINVAL,"Cannot get the value of the NULL semaphore");

	#ifndef HAVE_SEM_WAIT
	if(sem_getvalue((sem->s),svalue) < 0)
		THROW1(system_error,errno,"sem_getvalue() failed: %s",
	    strerror(errno));
	#else
	if(sem_getvalue(&(sem->s),svalue) < 0)
		THROW1(system_error,errno,"sem_getvalue() failed: %s",
	    strerror(errno));
	#endif
}

/* ********************************* WINDOWS IMPLEMENTATION ************************************ */

#elif defined(WIN32)

#include <math.h>

typedef struct xbt_os_thread_ {
  char *name;
  HANDLE handle;                  /* the win thread handle        */
  unsigned long id;               /* the win thread id            */
  pvoid_f_pvoid_t start_routine;
  void* param;
} s_xbt_os_thread_t ;

/* so we can specify the size of the stack of the threads */
#ifndef STACK_SIZE_PARAM_IS_A_RESERVATION
#define STACK_SIZE_PARAM_IS_A_RESERVATION 0x00010000
#endif

/* the default size of the stack of the threads (in bytes)*/
#define XBT_DEFAULT_THREAD_STACK_SIZE	4096

/* key to the TLS containing the xbt_os_thread_t structure */
static unsigned long xbt_self_thread_key;

void xbt_os_thread_mod_init(void) {
   xbt_self_thread_key = TlsAlloc();
}
void xbt_os_thread_mod_exit(void) {

   if (!TlsFree(xbt_self_thread_key))
     THROW0(system_error,(int)GetLastError(),"TlsFree() failed to cleanup the thread submodule");
}

static DWORD WINAPI  wrapper_start_routine(void *s) {
  xbt_os_thread_t t = (xbt_os_thread_t)s;
  void* rv;

    if(!TlsSetValue(xbt_self_thread_key,t))
     THROW0(system_error,(int)GetLastError(),"TlsSetValue of data describing the created thread failed");

   rv = (*(t->start_routine))(t->param);

   return *((DWORD*)rv);
}


xbt_os_thread_t xbt_os_thread_create(const char *name,pvoid_f_pvoid_t start_routine,
			       void* param)  {

   xbt_os_thread_t t = xbt_new(s_xbt_os_thread_t,1);

   t->name = xbt_strdup(name);
   t->start_routine = start_routine ;
   t->param = param;

   t->handle = CreateThread(NULL,XBT_DEFAULT_THREAD_STACK_SIZE,
			    (LPTHREAD_START_ROUTINE)wrapper_start_routine,
			    t,STACK_SIZE_PARAM_IS_A_RESERVATION,&(t->id));

   if(!t->handle) {
     xbt_free(t);
     THROW0(system_error,(int)GetLastError(),"CreateThread failed");
   }

   return t;
}

const char* xbt_os_thread_name(xbt_os_thread_t t) {
   return t->name;
}

const char* xbt_os_thread_self_name(void) {
   xbt_os_thread_t t = xbt_os_thread_self();
   return t?t->name:"main";
}

void
xbt_os_thread_join(xbt_os_thread_t thread,void ** thread_return) {

	if(WAIT_OBJECT_0 != WaitForSingleObject(thread->handle,INFINITE))
		THROW0(system_error,(int)GetLastError(), "WaitForSingleObject failed");

	if(thread_return){

		if(!GetExitCodeThread(thread->handle,(DWORD*)(*thread_return)))
			THROW0(system_error,(int)GetLastError(), "GetExitCodeThread failed");
	}

	CloseHandle(thread->handle);
	free(thread->name);
	free(thread);
}

void xbt_os_thread_exit(int *retval) {
   if(retval)
   	ExitThread(*retval);
   else
   	ExitThread(0);
}

xbt_os_thread_t xbt_os_thread_self(void) {
   return TlsGetValue(xbt_self_thread_key);
}

void *xbt_os_thread_getparam(void) {
   xbt_os_thread_t t = xbt_os_thread_self();
   return t->param;
}


void xbt_os_thread_yield(void) {
    Sleep(0);
}
void xbt_os_thread_cancel(xbt_os_thread_t t) {
   THROW_UNIMPLEMENTED;
}

/****** mutex related functions ******/
typedef struct xbt_os_mutex_ {
  /* KEEP IT IN SYNC WITH xbt_thread.c */
   CRITICAL_SECTION lock;
} s_xbt_os_mutex_t;

xbt_os_mutex_t xbt_os_mutex_init(void) {
   xbt_os_mutex_t res = xbt_new(s_xbt_os_mutex_t,1);

   /* initialize the critical section object */
   InitializeCriticalSection(&(res->lock));

   return res;
}

void xbt_os_mutex_acquire(xbt_os_mutex_t mutex) {

   EnterCriticalSection(& mutex->lock);
}

void xbt_os_mutex_tryacquire(xbt_os_mutex_t mutex)
{
	TryEnterCriticalSection(&mutex->lock);
}

void xbt_os_mutex_timedacquire(xbt_os_mutex_t mutex, double delay) {
	THROW_UNIMPLEMENTED;
}

void xbt_os_mutex_release(xbt_os_mutex_t mutex) {

   LeaveCriticalSection (&mutex->lock);

}

void xbt_os_mutex_destroy(xbt_os_mutex_t mutex) {

   if (!mutex) return;

   DeleteCriticalSection(& mutex->lock);
   free(mutex);
}

/***** condition related functions *****/
 enum { /* KEEP IT IN SYNC WITH xbt_thread.c */
    SIGNAL = 0,
    BROADCAST = 1,
    MAX_EVENTS = 2
 };

typedef struct xbt_os_cond_ {
  /* KEEP IT IN SYNC WITH xbt_thread.c */
   HANDLE events[MAX_EVENTS];

   unsigned int waiters_count;           /* the number of waiters                        */
   CRITICAL_SECTION waiters_count_lock;  /* protect access to waiters_count  */
} s_xbt_os_cond_t;

xbt_os_cond_t xbt_os_cond_init(void) {

   xbt_os_cond_t res = xbt_new0(s_xbt_os_cond_t,1);

   memset(& res->waiters_count_lock,0,sizeof(CRITICAL_SECTION));

   /* initialize the critical section object */
   InitializeCriticalSection(& res->waiters_count_lock);

   res->waiters_count = 0;

   /* Create an auto-reset event */
   res->events[SIGNAL] = CreateEvent (NULL, FALSE, FALSE, NULL);

   if(!res->events[SIGNAL]){
      DeleteCriticalSection(& res->waiters_count_lock);
      free(res);
      THROW0(system_error,0,"CreateEvent failed for the signals");
   }

   /* Create a manual-reset event. */
   res->events[BROADCAST] = CreateEvent (NULL, TRUE, FALSE,NULL);

   if(!res->events[BROADCAST]){

      DeleteCriticalSection(& res->waiters_count_lock);
      CloseHandle(res->events[SIGNAL]);
      free(res);
      THROW0(system_error,0,"CreateEvent failed for the broadcasts");
   }

   return res;
}

void xbt_os_cond_wait(xbt_os_cond_t cond, xbt_os_mutex_t mutex) {

   unsigned long wait_result;
   int is_last_waiter;

   /* lock the threads counter and increment it */
   EnterCriticalSection (& cond->waiters_count_lock);
   cond->waiters_count++;
   LeaveCriticalSection (& cond->waiters_count_lock);

   /* unlock the mutex associate with the condition */
   LeaveCriticalSection (& mutex->lock);

   /* wait for a signal (broadcast or no) */
   wait_result = WaitForMultipleObjects (2, cond->events, FALSE, INFINITE);

   if(wait_result == WAIT_FAILED)
     THROW0(system_error,0,"WaitForMultipleObjects failed, so we cannot wait on the condition");

   /* we have a signal lock the condition */
   EnterCriticalSection (& cond->waiters_count_lock);
   cond->waiters_count--;

   /* it's the last waiter or it's a broadcast ? */
   is_last_waiter = ((wait_result == WAIT_OBJECT_0 + BROADCAST - 1) && (cond->waiters_count == 0));

   LeaveCriticalSection (& cond->waiters_count_lock);

   /* yes it's the last waiter or it's a broadcast
    * only reset the manual event (the automatic event is reset in the WaitForMultipleObjects() function
    * by the system.
    */
   if (is_last_waiter)
      if(!ResetEvent (cond->events[BROADCAST]))
	THROW0(system_error,0,"ResetEvent failed");

   /* relock the mutex associated with the condition in accordance with the posix thread specification */
   EnterCriticalSection (& mutex->lock);
}
void xbt_os_cond_timedwait(xbt_os_cond_t cond, xbt_os_mutex_t mutex, double delay) {

   unsigned long wait_result = WAIT_TIMEOUT;
   int is_last_waiter;
   unsigned long end = (unsigned long)(delay * 1000);


   if (delay < 0) {
      xbt_os_cond_wait(cond,mutex);
   } else {
	  DEBUG3("xbt_cond_timedwait(%p,%p,%ul)",&(cond->events),&(mutex->lock),end);

   /* lock the threads counter and increment it */
   EnterCriticalSection (& cond->waiters_count_lock);
   cond->waiters_count++;
   LeaveCriticalSection (& cond->waiters_count_lock);

   /* unlock the mutex associate with the condition */
   LeaveCriticalSection (& mutex->lock);
   /* wait for a signal (broadcast or no) */

   wait_result = WaitForMultipleObjects (2, cond->events, FALSE, end);

   switch(wait_result) {
     case WAIT_TIMEOUT:
   	THROW3(timeout_error,GetLastError(),"condition %p (mutex %p) wasn't signaled before timeout (%f)",cond,mutex, delay);
	case WAIT_FAILED:
     THROW0(system_error,GetLastError(),"WaitForMultipleObjects failed, so we cannot wait on the condition");
   }

   /* we have a signal lock the condition */
   EnterCriticalSection (& cond->waiters_count_lock);
   cond->waiters_count--;

   /* it's the last waiter or it's a broadcast ? */
   is_last_waiter = ((wait_result == WAIT_OBJECT_0 + BROADCAST - 1) && (cond->waiters_count == 0));

   LeaveCriticalSection (& cond->waiters_count_lock);

   /* yes it's the last waiter or it's a broadcast
    * only reset the manual event (the automatic event is reset in the WaitForMultipleObjects() function
    * by the system.
    */
   if (is_last_waiter)
      if(!ResetEvent (cond->events[BROADCAST]))
	THROW0(system_error,0,"ResetEvent failed");

   /* relock the mutex associated with the condition in accordance with the posix thread specification */
   EnterCriticalSection (& mutex->lock);
   }
	/*THROW_UNIMPLEMENTED;*/
}

void xbt_os_cond_signal(xbt_os_cond_t cond) {
   int have_waiters;

   EnterCriticalSection (& cond->waiters_count_lock);
   have_waiters = cond->waiters_count > 0;
   LeaveCriticalSection (& cond->waiters_count_lock);

   if (have_waiters)
     if(!SetEvent(cond->events[SIGNAL]))
       THROW0(system_error,0,"SetEvent failed");

   xbt_os_thread_yield();
}

void xbt_os_cond_broadcast(xbt_os_cond_t cond){
   int have_waiters;

   EnterCriticalSection (& cond->waiters_count_lock);
   have_waiters = cond->waiters_count > 0;
   LeaveCriticalSection (& cond->waiters_count_lock);

   if (have_waiters)
     SetEvent(cond->events[BROADCAST]);
}

void xbt_os_cond_destroy(xbt_os_cond_t cond){
   int error = 0;

   if (!cond) return;

   if(!CloseHandle(cond->events[SIGNAL]))
     error = 1;

   if(!CloseHandle(cond->events[BROADCAST]))
     error = 1;

   DeleteCriticalSection(& cond->waiters_count_lock);

   xbt_free(cond);

   if (error)
     THROW0(system_error,0,"Error while destroying the condition");
}

typedef struct xbt_os_sem_ {
   HANDLE h;
   unsigned int value;
   CRITICAL_SECTION value_lock;  /* protect access to value of the semaphore  */
}s_xbt_os_sem_t ;

xbt_os_sem_t
xbt_os_sem_init(unsigned int value)
{
	xbt_os_sem_t res;

	if(value > INT_MAX)
	THROW1(arg_error,value,"Semaphore initial value too big: %ud cannot be stored as a signed int",value);

   	res = (xbt_os_sem_t)xbt_new0(s_xbt_os_sem_t,1);

	if(!(res->h = CreateSemaphore(NULL,value,(long)INT_MAX,NULL))) {
		THROW1(system_error,GetLastError(),"CreateSemaphore() failed: %s",
	    strerror(GetLastError()));
	    return NULL;
	}

  	res->value = value;

  	InitializeCriticalSection(&(res->value_lock));

   	return res;
}

void
xbt_os_sem_acquire(xbt_os_sem_t sem)
{
	if(!sem)
		THROW0(arg_error,EINVAL,"Cannot acquire the NULL semaphore");

	/* wait failure */
	if(WAIT_OBJECT_0 != WaitForSingleObject(sem->h,INFINITE))
		THROW1(system_error,GetLastError(),"WaitForSingleObject() failed: %s",
	    	strerror(GetLastError()));
	EnterCriticalSection(&(sem->value_lock));
	sem->value--;
	LeaveCriticalSection(&(sem->value_lock));
}

void xbt_os_sem_timedacquire(xbt_os_sem_t sem, double timeout)
{
	long seconds;
	long milliseconds;
	double end = timeout + xbt_os_time();

	if(!sem)
		THROW0(arg_error,EINVAL,"Cannot acquire the NULL semaphore");

	if (timeout < 0)
	{
		xbt_os_sem_acquire(sem);
	}
	else /* timeout can be zero <-> try acquire ) */
	{

		seconds = (long) floor(end);
		milliseconds = (long)( ( end - seconds) * 1000);
		milliseconds += (seconds * 1000);

		switch(WaitForSingleObject(sem->h,milliseconds))
		{
			case WAIT_OBJECT_0:
			EnterCriticalSection(&(sem->value_lock));
			sem->value--;
			LeaveCriticalSection(&(sem->value_lock));
			return;

			case WAIT_TIMEOUT:
			THROW2(timeout_error,GetLastError(),"semaphore %p wasn't signaled before timeout (%f)",sem,timeout);
			return;

			default:
			THROW3(system_error,GetLastError(),"WaitForSingleObject(%p,%f) failed: %s",sem,timeout, strerror(GetLastError()));
		}
	}
}

void
xbt_os_sem_release(xbt_os_sem_t sem)
{
	if(!sem)
		THROW0(arg_error,EINVAL,"Cannot release the NULL semaphore");

	if(!ReleaseSemaphore(sem->h,1, NULL))
		THROW1(system_error,GetLastError(),"ReleaseSemaphore() failed: %s",
	    	strerror(GetLastError()));
	EnterCriticalSection (&(sem->value_lock));
	sem->value++;
	LeaveCriticalSection(&(sem->value_lock));
}

void
xbt_os_sem_destroy(xbt_os_sem_t sem)
{
	if(!sem)
		THROW0(arg_error,EINVAL,"Cannot destroy the NULL semaphore");

	if(!CloseHandle(sem->h))
		THROW1(system_error,GetLastError(),"CloseHandle() failed: %s",
	    	strerror(GetLastError()));

	 DeleteCriticalSection(&(sem->value_lock));

	 xbt_free(sem);

}

void
xbt_os_sem_get_value(xbt_os_sem_t sem, int* svalue)
{
	if(!sem)
		THROW0(arg_error,EINVAL,"Cannot get the value of the NULL semaphore");

	EnterCriticalSection(&(sem->value_lock));
	*svalue = sem->value;
	LeaveCriticalSection(&(sem->value_lock));
}

#endif

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
#include "portable.h"
#include "xbt/xbt_os_thread.h" /* This module */
#include "xbt_modinter.h" /* Initialization/finalization of this module */



/* ********************************* PTHREAD IMPLEMENTATION ************************************ */
#ifdef HAVE_PTHREAD_H
#include <pthread.h>

typedef struct xbt_os_thread_ {
   pthread_t t;
   void *param;
   pvoid_f_pvoid_t *start_routine;
} s_xbt_os_thread_t ;

/* thread-specific data containing the xbt_os_thread_t structure */
static pthread_key_t xbt_self_thread_key;

/* frees the xbt_os_thread_t corresponding to the current thread */
static void xbt_os_thread_free_thread_data(void*d){
   free(d);
}

void xbt_os_thread_mod_init(void) {
   int errcode;
   
   if ((errcode=pthread_key_create(&xbt_self_thread_key, NULL)))
     THROW0(system_error,errcode,"pthread_key_create failed for xbt_self_thread_key");
}
void xbt_os_thread_mod_exit(void) {
   /* FIXME: don't try to free our key on shutdown. Valgrind detects no leak if we don't, and whine if we try to */
//   int errcode;
   
//   if ((errcode=pthread_key_delete(xbt_self_thread_key)))
//     THROW0(system_error,errcode,"pthread_key_delete failed for xbt_self_thread_key");
}

static void * wrapper_start_routine(void *s) {
  xbt_os_thread_t t = s;   
  int errcode;

  if ((errcode=pthread_setspecific(xbt_self_thread_key,t)))
    THROW0(system_error,errcode,"pthread_setspecific failed for xbt_self_thread_key");   
  return t->start_routine(t->param);
}

xbt_os_thread_t xbt_os_thread_create(pvoid_f_pvoid_t start_routine,
				     void* param)  {
   int errcode;

   xbt_os_thread_t res_thread=xbt_new(s_xbt_os_thread_t,1);
   res_thread->start_routine = start_routine;
   res_thread->param = param;

   
   if ((errcode = pthread_create(&(res_thread->t), NULL, wrapper_start_routine, res_thread)))
     THROW1(system_error,errcode, "pthread_create failed: %s",strerror(errcode));

   return res_thread;
}

void 
xbt_os_thread_join(xbt_os_thread_t thread,void ** thread_return) {
	
	int errcode;   
	
	if ((errcode = pthread_join(thread->t,thread_return)))
		THROW1(system_error,errcode, "pthread_join failed: %s",
		       strerror(errcode));
	free(thread);   
}		       

void xbt_os_thread_exit(int *retval) {
   pthread_exit(retval);
}

xbt_os_thread_t xbt_os_thread_self(void) {
   return pthread_getspecific(xbt_self_thread_key);
}

#include <sched.h>
void xbt_os_thread_yield(void) {
   sched_yield();
}
/****** mutex related functions ******/
typedef struct xbt_os_mutex_ {
   pthread_mutex_t m;
} s_xbt_os_mutex_t;

xbt_os_mutex_t xbt_os_mutex_init(void) {
   xbt_os_mutex_t res = xbt_new(s_xbt_os_mutex_t,1);
   int errcode;
   
   if ((errcode = pthread_mutex_init(&(res->m),NULL)))
     THROW1(system_error,errcode,"pthread_mutex_init() failed: %s",
	    strerror(errcode));
   
   return res;
}

void xbt_os_mutex_lock(xbt_os_mutex_t mutex) {
   int errcode;
   
   if ((errcode=pthread_mutex_lock(&(mutex->m))))
     THROW2(system_error,errcode,"pthread_mutex_lock(%p) failed: %s",
	    mutex, strerror(errcode));
}

void xbt_os_mutex_unlock(xbt_os_mutex_t mutex) {
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

/* ********************************* WINDOWS IMPLEMENTATION ************************************ */

#elif defined(WIN32)

typedef struct xbt_os_thread_ {
  HANDLE handle;                  /* the win thread handle        */
  unsigned long id;               /* the win thread id            */
  pvoid_f_pvoid_t *start_routine;
  void* param;
} s_xbt_os_thread_t ;

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
 
    if(!TlsSetValue(xbt_self_thread_key,t))
     THROW0(system_error,(int)GetLastError(),"TlsSetValue of data describing the created thread failed");
   
   return (DWORD)t->start_routine(t->param);
}


xbt_os_thread_t xbt_os_thread_create(pvoid_f_pvoid_t start_routine,
			       void* param)  {
   
   xbt_os_thread_t t = xbt_new(s_xbt_os_thread_t,1);

   t->start_routine = start_routine ;
   t->param = param;
   
   t->handle = CreateThread(NULL,0,
			    (LPTHREAD_START_ROUTINE)wrapper_start_routine,
			    t,0,&(t->id));
	
   if(!t->handle) {
     xbt_free(t);
     THROW0(system_error,(int)GetLastError(),"CreateThread failed");
   }
   
   return t;
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

void xbt_os_thread_yield(void) {
    Sleep(0);
}

/****** mutex related functions ******/
typedef struct xbt_os_mutex_ {
   CRITICAL_SECTION lock;   
} s_xbt_os_mutex_t;

xbt_os_mutex_t xbt_os_mutex_init(void) {
   xbt_os_mutex_t res = xbt_new(s_xbt_os_mutex_t,1);

   /* initialize the critical section object */
   InitializeCriticalSection(&(res->lock));
   
   return res;
}

void xbt_os_mutex_lock(xbt_os_mutex_t mutex) {

   EnterCriticalSection(& mutex->lock);
}

void xbt_os_mutex_unlock(xbt_os_mutex_t mutex) {

   LeaveCriticalSection (& mutex->lock);

}

void xbt_os_mutex_destroy(xbt_os_mutex_t mutex) {

   if (!mutex) return;
   
   DeleteCriticalSection(& mutex->lock);		
   free(mutex);
}

/***** condition related functions *****/
 enum {
    SIGNAL = 0,
    BROADCAST = 1,
    MAX_EVENTS = 2
 };

typedef struct xbt_os_cond_ {
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

#endif

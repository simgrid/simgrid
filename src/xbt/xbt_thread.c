/* $Id$ */

/* xbt_thread -- portability layer over the pthread API                     */

/* Copyright 2006,2007 Malek Cherier, Martin Quinson          
 * All right reserved.                                                      */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/ex.h"
#include "portable.h"
#include "xbt/xbt_thread.h" /* This module */
#include "xbt_modinter.h" /* Initialization/finalization of this module */



/* ********************************* PTHREAD IMPLEMENTATION ************************************ */
#ifdef HAVE_PTHREAD_H
#include <pthread.h>

typedef struct xbt_thread_ {
   pthread_t t;
} s_xbt_thread_t ;

/* thread-specific data containing the xbt_thread_t structure */
pthread_key_t xbt_self_thread_key;

/* frees the xbt_thread_t corresponding to the current thread */
static void xbt_thread_free_thread_data(void*d){
   free(d);
}

void xbt_thread_mod_init(void) {
   int errcode;
   
   if ((errcode=pthread_key_create(&thread_data, &xbt_thread_free_thread_data)))
     THROW0(system_error,errcode,"pthread_key_create failed for thread_data");
}
void xbt_thread_mod_exit(void) {
   int errcode;
   
   if ((errcode=pthread_key_delete(thread_data)))
     THROW0(system_error,errcode,"pthread_key_delete failed for thread_data");
}


xbt_thread_t xbt_thread_create(pvoid_f_pvoid_t start_routine,
			       void* param)  {
   xbt_thread_t res = xbt_new(s_xbt_thread_t,1);
   int errcode;

   if ((errcode=pthread_setspecific(thread_data,res)))
     THROW0(system_error,errcode,"pthread_setspecific failed for thread_data");
      
   if ((errcode = pthread_create(&(res->t), NULL, start_routine, param)))
     THROW0(system_error,errcode, "pthread_create failed");
   
   return res;
}		       
void xbt_thread_exit(int *retval) {
   pthread_exit(retval);
}
xbt_thread_t xbt_thread_self(void) {
   return pthread_getspecific(thread_data);
}

#include <sched.h>
void xbt_thread_yield(void) {
   sched_yield();
}
/****** mutex related functions ******/
typedef struct xbt_mutex_ {
   pthread_mutex_t m;
} s_xbt_mutex_t;

xbt_mutex_t xbt_mutex_init(void) {
   xbt_mutex_t res = xbt_new(s_xbt_mutex_t,1);
   int errcode;
   
   if ((errcode = pthread_mutex_init(&(res->m),NULL)))
     THROW0(system_error,errcode,"pthread_mutex_init() failed");
   
   return res;
}

void xbt_mutex_lock(xbt_mutex_t mutex) {
   int errcode;
   
   if ((errcode=pthread_mutex_lock(&(mutex->m))))
     THROW1(system_error,errcode,"pthread_mutex_lock(%p) failed",mutex);
}

void xbt_mutex_unlock(xbt_mutex_t mutex) {
   int errcode;
   
   if ((errcode=pthread_mutex_unlock(&(mutex->m))))
     THROW1(system_error,errcode,"pthread_mutex_unlock(%p) failed",mutex);
}

void xbt_mutex_destroy(xbt_mutex_t mutex) {
   int errcode;
   
   if ((errcode=pthread_mutex_destroy(&(mutex->m))))
     THROW1(system_error,errcode,"pthread_mutex_destroy(%p) failed",mutex);
   free(mutex);
}

/***** condition related functions *****/
typedef struct xbt_thcond_ {
   pthread_cond_t c;
} s_xbt_thcond_t;

xbt_thcond_t xbt_thcond_init(void) {
   xbt_thcond_t res = xbt_new(s_xbt_thcond_t,1);
   int errcode;
   if ((errcode=pthread_cond_init(&(res->c),NULL)))
     THROW0(system_error,errcode,"pthread_cond_init() failed");

   return res;
}

void xbt_thcond_wait(xbt_thcond_t cond, xbt_mutex_t mutex) {
   int errcode;
   if ((errcode=pthread_cond_wait(&(cond->c),&(mutex->m))))
     THROW2(system_error,errcode,"pthread_cond_wait(%p,%p) failed",cond,mutex);
}

void xbt_thcond_signal(xbt_thcond_t cond) {
   int errcode;
   if ((errcode=pthread_cond_signal(&(cond->c))))
     THROW1(system_error,errcode,"pthread_cond_signal(%p) failed",cond);
}
	 
void xbt_thcond_broadcast(xbt_thcond_t cond){
   int errcode;
   if ((errcode=pthread_cond_broadcast(&(cond->c))))
     THROW1(system_error,errcode,"pthread_cond_broadcast(%p) failed",cond);
}
void xbt_thcond_destroy(xbt_thcond_t cond){
   int errcode;
   if ((errcode=pthread_cond_destroy(&(cond->c))))
     THROW1(system_error,errcode,"pthread_cond_destroy(%p) failed",cond);
   free(cond);
}

/* ********************************* WINDOWS IMPLEMENTATION ************************************ */

#elif defined(WIN32)

typedef struct xbt_thread_ {
   HANDLE handle;                  /* the win thread handle        */
   unsigned long id;               /* the win thread id            */
} s_xbt_thread_t ;

/* key to the TLS containing the xbt_thread_t structure */
unsigned long xbt_self_thread_key;

void xbt_thread_mod_init(void) {
   xbt_self_thread_key = TlsAlloc();
}
void xbt_thread_mod_exit(void) {
   int errcode;
   if (!(errcode = TlsFree(xbt_self_thread_key))) 
     THROW0(system_error,errcode,"TlsFree() failed to cleanup the thread submodule");
}

xbt_thread_t xbt_thread_create(pvoid_f_pvoid_t start_routine,
			       void* param)  {
   
   xbt_thread_t res = xbt_new(s_xbt_thread_t,1);
   
   res->handle = CreateThread(NULL,0, 
			      (LPTHREAD_START_ROUTINE)start_routine,
			      param,0,& res->id);
	
   if(!res->handle) {
     xbt_free(res);
     THROW0(system_error,0,"CreateThread failed");
   }
   
   if(!TlsSetValue(xbt_self_thread_key,res))
     THROW0(system_error,0,"TlsSetValue of data describing the created thread failed");
     
   return res;
}

void xbt_thread_exit(int *retval) {
   xbt_thread_t self = xbt_thread_self();
   
   CloseHandle(self->handle);
   free(self);
   
   ExitThread(*retval);
}

xbt_thread_t xbt_thread_self(void) {
   return TlsGetValue(xbt_self_thread_key);
}

void xbt_thread_yield(void) {
    Sleep(0);
}

/****** mutex related functions ******/
typedef struct xbt_mutex_ {
   CRITICAL_SECTION lock;   
} s_xbt_mutex_t;

xbt_mutex_t xbt_mutex_init(void) {
   xbt_mutex_t res = xbt_new(s_xbt_mutex_t,1);

   /* initialize the critical section object */
   InitializeCriticalSection(&(res->lock));
   
   return res;
}

void xbt_mutex_lock(xbt_mutex_t mutex) {

   EnterCriticalSection(& mutex->lock);
}

void xbt_mutex_unlock(xbt_mutex_t mutex) {

   LeaveCriticalSection (& mutex->lock);

}

void xbt_mutex_destroy(xbt_mutex_t mutex) {

   DeleteCriticalSection(& mutex->lock);
		
   free(mutex);
}

/***** condition related functions *****/
 enum {
    SIGNAL = 0,
    BROADCAST = 1,
    MAX_EVENTS = 2
 };

typedef struct xbt_thcond_ {
   HANDLE events[MAX_EVENTS];
   
   unsigned int waiters_count;           /* the number of waiters                        */
   CRITICAL_SECTION waiters_count_lock;  /* protect access to waiters_count  */
} s_xbt_thcond_t;

xbt_thcond_t xbt_thcond_init(void) {

   xbt_thcond_t res = xbt_new0(s_xbt_thcond_t,1);
	
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

void xbt_thcond_wait(xbt_thcond_t cond, xbt_mutex_t mutex) {
	
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

void xbt_thcond_signal(xbt_thcond_t cond) {
   int have_waiters;

   EnterCriticalSection (& cond->waiters_count_lock);
   have_waiters = cond->waiters_count > 0;
   LeaveCriticalSection (& cond->waiters_count_lock);
	
   if (have_waiters)
     if(!SetEvent(cond->events[SIGNAL]))
       THROW0(system_error,0,"SetEvent failed");
}

void xbt_thcond_broadcast(xbt_thcond_t cond){
   int have_waiters;

   EnterCriticalSection (& cond->waiters_count_lock);
   have_waiters = cond->waiters_count > 0;
   LeaveCriticalSection (& cond->waiters_count_lock);
	
   if (have_waiters)
     SetEvent(cond->events[BROADCAST]);
}

void xbt_thcond_destroy(xbt_thcond_t cond){
   int error = 0;
   
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

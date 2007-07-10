/* $Id$ */

/* xbt_synchro -- Synchronization virtualized depending on whether we are   */
/*                in simulation or real life (act on simulated processes)   */

/* This is the real life implementation, using xbt_os_thread to be portable */
/* to windows and linux.                                                    */

/* Copyright 2006,2007 Malek Cherier, Martin Quinson          
 * All right reserved.                                                      */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/ex.h"
#include "portable.h"

#include "xbt/synchro.h"       /* This module */
#include "xbt/xbt_os_thread.h" /* The implementation we use */

/* the implementation would be cleaner (and faster) with ELF symbol aliasing */


struct xbt_thread_ {
   /* KEEP IT IN SYNC WITH xbt_os_thread (both win and lin parts) */
#ifdef HAVE_PTHREAD_H
   pthread_t t;
   void *param;
   pvoid_f_pvoid_t *start_routine;
#elif defined(WIN32)
   HANDLE handle;                  /* the win thread handle        */
   unsigned long id;               /* the win thread id            */
   pvoid_f_pvoid_t *start_routine;
   void* param;
#endif
};

xbt_thread_t xbt_thread_create(pvoid_f_pvoid_t start_routine,
			       void* param)  {
   return (xbt_thread_t)xbt_os_thread_create(start_routine,param);
}

void 
xbt_thread_join(xbt_thread_t thread,void ** thread_return) {
   xbt_os_thread_join( (xbt_os_thread_t)thread, thread_return );
}		       

void xbt_thread_exit(int *retval) {
   xbt_os_thread_exit( retval );
}

xbt_thread_t xbt_thread_self(void) {
   return (xbt_thread_t)xbt_os_thread_self();
}

void xbt_thread_yield(void) {
   xbt_os_thread_yield();
}
/****** mutex related functions ******/
struct xbt_mutex_ {
   /* KEEP IT IN SYNC WITH OS IMPLEMENTATION (both win and lin) */
#ifdef HAVE_PTHREAD_H   
   pthread_mutex_t m;
#elif defined(WIN32)
   CRITICAL_SECTION lock;   
#endif   
};

xbt_mutex_t xbt_mutex_init(void) {
   return (xbt_mutex_t)xbt_os_mutex_init();
}

void xbt_mutex_lock(xbt_mutex_t mutex) {
   xbt_os_mutex_lock( (xbt_os_mutex_t)mutex );
}

void xbt_mutex_unlock(xbt_mutex_t mutex) {
   xbt_os_mutex_unlock( (xbt_os_mutex_t)mutex );
}

void xbt_mutex_destroy(xbt_mutex_t mutex) {
   xbt_os_mutex_destroy( (xbt_os_mutex_t)mutex );
}

#ifdef WIN32
 enum { /* KEEP IT IN SYNC WITH OS IMPLEM */
    SIGNAL = 0,
    BROADCAST = 1,
    MAX_EVENTS = 2
 };
#endif

/***** condition related functions *****/
typedef struct xbt_cond_ {
   /* KEEP IT IN SYNC WITH OS IMPLEMENTATION (both win and lin) */
#ifdef HAVE_PTHREAD_H   
   pthread_cond_t c;
#elif defined(WIN32)   
   HANDLE events[MAX_EVENTS];
   
   unsigned int waiters_count;           /* the number of waiters                        */
   CRITICAL_SECTION waiters_count_lock;  /* protect access to waiters_count  */
#endif
} s_xbt_cond_t;

xbt_cond_t xbt_cond_init(void) {
   return (xbt_cond_t) xbt_os_cond_init();
}

void xbt_cond_wait(xbt_cond_t cond, xbt_mutex_t mutex) {
   xbt_os_cond_wait( (xbt_os_cond_t)cond, (xbt_os_mutex_t)mutex );
}

void xbt_cond_signal(xbt_cond_t cond) {
   xbt_os_cond_signal( (xbt_os_cond_t)cond );
}
	 
void xbt_cond_broadcast(xbt_cond_t cond){
   xbt_os_cond_broadcast( (xbt_os_cond_t)cond );
}
void xbt_cond_destroy(xbt_cond_t cond){
   xbt_os_cond_destroy( (xbt_os_cond_t)cond );
}

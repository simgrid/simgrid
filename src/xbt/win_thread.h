/* 
 * Copyright (c) 2007 Malek CHERIER. All rights reserved.					
 */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.	
 */
 
#ifndef XBT_WIN_THREAD_H
#define XBT_WIN_THREAD_H

#include <windows.h>
#include "xbt/sysdep.h"

#ifdef __cplusplus
extern "C" {
#endif

/* type of function pointeur used */
typedef  DWORD WINAPI (*pfn_start_routine_t)(void*);

/*
 * This structure represents a windows thread.
 */
typedef struct s_win_thread
{
	HANDLE handle;			/* the win thread handle	*/
	unsigned long id;		/* the win thread id		*/	
}s_win_thread_t,* win_thread_t;

/*
 * This structure simulates a pthread cond.
 */

 enum
 {
    SIGNAL = 0,
    BROADCAST = 1,
    MAX_EVENTS = 2
 };

typedef struct s_win_thread_cond
{
  	HANDLE events[MAX_EVENTS];

	unsigned int waiters_count;			/* the number of waiters			*/
	CRITICAL_SECTION waiters_count_lock;/* protect access to waiters_count	*/
}s_win_thread_cond_t,* win_thread_cond_t;

/*
 * This structure represents a windows mutext
 * remark : only encapsulate the mutext handle.
 */
typedef struct s_win_thread_mutex
{
	CRITICAL_SECTION lock;
}s_win_thread_mutex_t,* win_thread_mutex_t;

/*
 * Windows thread connected functions.
 */

/*
 * Create a win thread.
 * @param thread The address of the thread to create
 * @param start_routine The thread function.
 * @param param A optional pointer to the thread function parameters.
 * @return If successful the function returns 0. Otherwise the function
 *  return 1.
 */ 
int
win_thread_create(win_thread_t* thread, pfn_start_routine_t start_routine,void* param);

/*
 * Terminate the win thread.
 * @param thread An address to the thread to exit.
 * @param exit_code The exit code returned by the thread.
 * @return This function always returns 0;
 */ 
int
win_thread_exit(win_thread_t* thread,unsigned long exit_code);

/*
 * Return the identifier of the current thread.
 */
unsigned long
win_thread_self(void);

/*
 * Windows mutex connected functions;
 */
 
/*
 * Create a windows mutex.
 * @param mutex The address to the mutex to create.
 * @return If successful the function returns 0. Otherwise
 *	the function returns 1.
 */
int
win_thread_mutex_init(win_thread_mutex_t* mutex);

/*
 * Lock a windows mutex.
 * @param mutex The address to the mutex to lock.
 * @return If successful the function returns 0.
 *  Otherwise the function return 1.
 */
int
win_thread_mutex_lock(win_thread_mutex_t* mutex);

/*
 * Unlock a windows mutex.
 * @param mutex The address to the mutex to unlock.
 * @return If successful the function returns 0.
 *  Otherwise the function return 1.
 */
int
win_thread_mutex_unlock(win_thread_mutex_t* mutex);

/* 
 * Destroy a windows mutex.
 * @param mutex The address of the mutex to destroy.
 * @return If successful the function return 0.
 *  Otherwise the function returns 1.
 */
int
win_thread_mutex_destroy(win_thread_mutex_t* mutex);

/*
 * Condition connected functions.
 */

/*
 * Create a condition.
 * @param cond The address to the condition to create.
 * @return If successful the function returns 0.
 *  Otherwise the function returns 1.
 */
int
win_thread_cond_init(win_thread_cond_t* cond);

/*
 * Wait about a condition.
 * @param cond The address to the condition to wait about.
 * @param mutex The address to the mutex associated to the condition.
 * @return If successful the function returns 0.
 *  Otherwise the function return 1.
 */
int
win_thread_cond_wait(win_thread_cond_t* cond,win_thread_mutex_t* mutex);

/*
 * Signal that a condition is verified.
 * @param cond The address of the condition concerned by the signal.
 * @return If successful the function return 0.
 *  Otherwise the function return 1.
 */
int
win_thread_cond_signal(win_thread_cond_t* cond);

/*
 * Unblock all threads waiting for a condition.
 * @param cond The address of the condition to broadcast.
 * @return If successful the function return 0.
 * Otherwise the function return 1.
 */

int 
win_thread_cond_broadcast(win_thread_cond_t* cond);

/*
 * Destroy a condition.
 * @param The address to the condition to destroy.
 * @return If successful the function returns 0.
 *  Otherwise the function return 1.
 */
int
win_thread_cond_destroy(win_thread_cond_t* cond);

#ifdef __cplusplus
extern }
#endif


#endif /* !XBT_WIN_THREAD_H */
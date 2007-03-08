#include "win_thread.h"

#include <string.h>
#include <stdio.h>

int                       
win_thread_create(win_thread_t* thread,pfn_start_routine_t start_routine,void* param){

	*thread = xbt_new0(s_win_thread_t,1);
	
	if(!*thread)
		return 1;
		
	(*thread)->handle = CreateThread(NULL,NULL,start_routine,param,0,&((*thread)->id));
	
	if(!((*thread)->handle)){
		xbt_free(*thread);
		*thread = NULL;
		return 1;
	}
	
	return 0;	
}

int
win_thread_exit(win_thread_t* thread,unsigned long exit_code)
{
    win_thread_t ___thread = *thread;

	CloseHandle(___thread->handle);
    free(___thread);
	___thread = NULL;

	ExitThread(exit_code);


    
    return 0;
}

unsigned long
win_thread_self(void){
	return GetCurrentThreadId();
}

int
win_thread_mutex_init(win_thread_mutex_t* mutex)
{

	*mutex = xbt_new0(s_win_thread_mutex_t,1);
	
	if(!*mutex)
		return 1;
		
	/* initialize the critical section object */
	InitializeCriticalSection(&((*mutex)->lock));
	return 0;
}

int
win_thread_mutex_lock(win_thread_mutex_t* mutex){

   	EnterCriticalSection(&((*mutex)->lock));
   	
	return 0;
}

int
win_thread_mutex_unlock(win_thread_mutex_t* mutex){

	LeaveCriticalSection (&(*mutex)->lock);
	return 0;		
}

int
win_thread_mutex_destroy(win_thread_mutex_t* mutex){

	DeleteCriticalSection(&((*mutex)->lock));	
		
	free(*mutex);
	*mutex = NULL;
	
	return 0;
}

int
win_thread_cond_init(win_thread_cond_t* cond){

	*cond = xbt_new0(s_win_thread_cond_t,1);
	
	if(!*cond)
		return 1;

	memset(&((*cond)->waiters_count_lock),0,sizeof(CRITICAL_SECTION));
	
	/* initialize the critical section object */
	InitializeCriticalSection(&((*cond)->waiters_count_lock));
	
	(*cond)->waiters_count = 0;
	
	/* Create an auto-reset event */
	(*cond)->events[SIGNAL] = CreateEvent (NULL, FALSE, FALSE, NULL); 
	
	if(!(*cond)->events[SIGNAL]){
		DeleteCriticalSection(&((*cond)->waiters_count_lock));
		free(*cond);
		return 1;
	}
	
	/* Create a manual-reset event. */
	(*cond)->events[BROADCAST] = CreateEvent (NULL, TRUE, FALSE,NULL);
	
	if(!(*cond)->events[BROADCAST]){
		
		DeleteCriticalSection(&((*cond)->waiters_count_lock));
		
		if(!CloseHandle((*cond)->events[SIGNAL]))
			return 1;
		
		free(*cond);
		return 1;
	}

	return 0;	
}

int
win_thread_cond_wait(win_thread_cond_t* cond,win_thread_mutex_t* mutex){
	
	unsigned long wait_result;
	int is_last_waiter;

	 /* lock the threads counter and increment it */
	EnterCriticalSection (&((*cond)->waiters_count_lock));
	(*cond)->waiters_count++;
	LeaveCriticalSection (&((*cond)->waiters_count_lock));
	
	
	 /* unlock the mutex associate with the condition */
	LeaveCriticalSection (&(*mutex)->lock);
	
	/* wait for a signal (broadcast or no) */
	wait_result = WaitForMultipleObjects (2, (*cond)->events, FALSE, INFINITE);
	
	if(WAIT_FAILED == wait_result)
		return 1;
	
	/* we have a signal lock the condition */
	EnterCriticalSection (&((*cond)->waiters_count_lock));
	(*cond)->waiters_count--;
	
	/* it's the last waiter or it's a broadcast ? */
	is_last_waiter = ((wait_result == WAIT_OBJECT_0 + BROADCAST - 1) && ((*cond)->waiters_count == 0));
	
	LeaveCriticalSection (&((*cond)->waiters_count_lock));
	
	/* yes it's the last waiter or it's a broadcast
	 * only reset the manual event (the automatic event is reset in the WaitForMultipleObjects() function
	 * by the système. 
	 */
	if (is_last_waiter){
		
		if(!ResetEvent ((*cond)->events[BROADCAST]))
			return 1;
	} 
	
	/* relock the mutex associated with the condition in accordance with the posix thread specification */
	EnterCriticalSection (&(*mutex)->lock);

	return 0;
}

int
win_thread_cond_signal(win_thread_cond_t* cond)
{
	int have_waiters;

	EnterCriticalSection (&((*cond)->waiters_count_lock));
	have_waiters = (*cond)->waiters_count > 0;
	LeaveCriticalSection (&((*cond)->waiters_count_lock));
	
	if (have_waiters){
		
		if(!SetEvent((*cond)->events[SIGNAL]))
			return 1;
			
	}
	
	return 0;
}

int 
win_thread_cond_broadcast(win_thread_cond_t* cond)
{
	int have_waiters;

	EnterCriticalSection (&((*cond)->waiters_count_lock));
	have_waiters = (*cond)->waiters_count > 0;
	LeaveCriticalSection (&((*cond)->waiters_count_lock));
	
	if (have_waiters)
		SetEvent((*cond)->events[BROADCAST]);
	
	return 0;
}



int
win_thread_cond_destroy(win_thread_cond_t* cond)
{
	int result = 0;

	if(!CloseHandle((*cond)->events[SIGNAL]))
		result= 1;
	
	if(!CloseHandle((*cond)->events[BROADCAST]))
		result = 1;
	
	DeleteCriticalSection(&((*cond)->waiters_count_lock));	
	
	xbt_free(*cond);
	*cond = NULL;
	
	return result;
}

void
win_thread_yield(void)
{
    Sleep(0);
}



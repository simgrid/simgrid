#include "win_thread.h"

#include <string.h>
#include <stdio.h>


static win_threads_t __win_threads = NULL;

static win_threads_t
__win_threads_instance(void);

static int
__win_threads_attach(win_thread_t thread);

static int
__win_threads_detach(win_thread_t thread);

static void
__win_threads_free(void);

static void
__win_threads_lock(void);

static void
__win_threads_unlock(void);

static win_thread_t
__win_thread_self(void);

static int
__win_thread_key_attach(win_thread_t thread,win_thread_key_t key);

static int
__win_thread_key_detach(win_thread_t thread,win_thread_key_t key);

int                       
win_thread_create(win_thread_t* thread,pfn_start_routine_t start_routine,void* param){

	*thread = xbt_new0(s_win_thread_t,1);
	
	if(!*thread)
		return 1;
		
	(*thread)->keys = xbt_new0(s_win_thread_keys_t,1);
	
	if(!((*thread)->keys)){
		xbt_free(*thread);
		return 1;
	}
	
	(*thread)->handle = CreateThread(NULL,NULL,start_routine,param,0,&((*thread)->id));
	
	if(!((*thread)->handle)){
		xbt_free(*thread);
		*thread = NULL;
		return 1;
	}
	
	__win_threads_attach(*thread);
	
	return 0;	
}

int
win_thread_exit(win_thread_t* thread,unsigned long exit_code)
{
    win_thread_key_t cur,next;
	win_thread_t ___thread = *thread;
	
	__win_threads_detach(*thread);
	
	CloseHandle(___thread->handle);
    
    cur = ___thread->keys->front;
    
	 while(cur){
	 	
	 	next = cur->next;
	 	
	 	if(cur->dstr_func){
	 		
	 		(*(cur->dstr_func))(win_thread_getspecific(cur));
	 		win_thread_key_delete(cur);
	 		
	 	}
	 	
	 	cur = next;
	}
	
	xbt_free(___thread->keys);
    xbt_free(___thread);
	___thread = NULL;
	
	ExitThread(exit_code);

    return 0;
}

win_thread_t
win_thread_self(void){
	return __win_thread_self();
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

	LeaveCriticalSection (&((*mutex)->lock));
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
	LeaveCriticalSection(&((*cond)->waiters_count_lock));
	
	
	 /* unlock the mutex associate with the condition */
	LeaveCriticalSection(&(*mutex)->lock);
	
	/* wait for a signal (broadcast or no) */
	wait_result = WaitForMultipleObjects (2, (*cond)->events, FALSE, INFINITE);
	
	if(WAIT_FAILED == wait_result)
		return 1;
	
	/* we have a signal lock the condition */
	EnterCriticalSection (&((*cond)->waiters_count_lock));
	(*cond)->waiters_count--;
	
	/* it's the last waiter or it's a broadcast ? */
	is_last_waiter = ((wait_result == WAIT_OBJECT_0 + BROADCAST - 1) && ((*cond)->waiters_count == 0));
	
	LeaveCriticalSection(&((*cond)->waiters_count_lock));
	
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
	LeaveCriticalSection(&((*cond)->waiters_count_lock));
	
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
	LeaveCriticalSection(&((*cond)->waiters_count_lock));
	
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


int 
win_thread_key_create(win_thread_key_t* key, pfn_key_dstr_func_t dstr_func)
{
	*key = xbt_new0(s_win_thread_key_t,1);
	
	if(!(*key))
		return 1;
		
	if(__win_thread_key_attach(__win_thread_self(),*key){
		xbt_free(*key);
		*key = NULL;
		return 1;
	}
	
	(*key)->value = TlsAlloc();
	(*key)->dstr_func = dstr_func;
	
	return 0; 
}

int 
win_thread_key_delete(win_thread_key_t key)
{
	void* p;
	int success;
	
	if(!key)
		return 1;
		
	if(!TlsFree(key->value))
		return 1;
	
	success = __win_thread_key_detach(__win_thread_self(),key);
	
	xbt_free(key);
	
	return success;	
} 

int 
win_thread_setspecific(win_thread_key_t key, const void* p)
{
	if(!key)
		return 1;
		
	if(!TlsSetValue(key->key,(void*)p))
		return 1;
	
	return 0;
} 

void* 
win_thread_getspecific(win_thread_key_t key)
{
	if(!key)
		return NULL;
		
	return TlsGetValue(key->value);	
} 


static win_threads_t
__win_threads_instance(void){
	
	if(!__win_threads){
		__win_threads = xbt_new0(s_win_threads_t,1);
		
		if(!__win_threads)
			return NULL;
			
		InitializeCriticalSection(&(__win_threads->lock));
	}
	
	return __win_threads;		
}

static int
__win_threads_attach(win_thread_t thread)
{
	win_thread_entry_t entry;
	win_threads_t threads;
	
	__win_threads_lock();
	
	threads = __win_threads_instance();
	
	if(!threads){
		__win_threads_unlock();
		return 1;
	}
	
		
	entry = xbt_new0(win_thread_entry_t,1);
	
	if(!entry){
		__win_threads_unlock();
		return 1;
	}
		
	entry->thread = thread; 
	
	if(threads->front){
		entry->next = threads->front; 
		threads->front->prev = entry;
	}
	
	threads->front = entry;
	threads->size++;
	
	__win_threads_unlock();	
	return 0;	
}

static int
__win_threads_detach(win_thread_t thread)
{
	win_thread_entry_t cur;
	win_threads_t threads ;
	
	
	__win_threads_lock();
	
	threads = __win_threads_instance();
	
	if(!threads){
		__win_threads_unlock();
		return 1;
	}
	
	cur = threads->front;
		
	while(cur){
		
		if(cur->thread == thread){
			
			if(cur->next)
				cur->next->prev = cur->prev;
				
			if(cur->prev)
				cur->prev->next = cur->next;
				
			xbt_free(cur); 
			
			threads->size--;
			
			if(!(threads->size))
				__win_threads_free();
			else
				__win_threads_unlock();
				
			return 0;
		}
		
		cur = cur->next;
	}
	
	__win_threads_unlock();
	return 1;
}

static win_thread_t
__win_thread_self(void)
{
	win_thread_entry_t cur;
	win_threads_t threads ;
	win_thread_t thread;
	unsigned long thread_id;
	
	__win_threads_lock();
	
	thread_id = GetCurrentThreadId();
	
	threads = __win_threads_instance();
	
	if(!threads){
		__win_threads_unlock();
		return NULL;
	}
	
	cur = threads->front;
		
	while(cur){
		
		if(cur->thread->id == thread_id){
			
			thread = cur->thread;
			__win_threads_unlock();
			return thread;
		}
		
		cur = cur->next;
	}
	
	__win_threads_unlock();
	return NULL;
}

static void
__win_threads_free(void)
{
	CRITICAL_SECTION lock;
	
	if(__win_threads){
		lock = __win_threads->lock;
		free(__win_threads);
		__win_threads = NULL;
		LeaveCriticalSection(&lock);
		DeleteCriticalSection(&lock);	
	}			
}

static void
__win_threads_lock(void)
{
	EnterCriticalSection(&(__win_threads->lock));
}

static void
__win_threads_unlock(void)
{
	LeaveCriticalSection(&(__win_threads->lock));
}

static int
__win_thread_key_attach(win_thread_t thread,win_thread_key_t key)
{
	win_thread_keys_t keys;
	
	keys = thread->keys;
	
	if(!keys)
		return 1;
	
	
	if(keys->front){
		key->next = keys->front; 
		keys->front->prev = key;
	}
	
	keys->front = key;
	
	return 0;	
}

static int
__win_thread_key_detach(win_thread_t thread,win_thread_key_t key)
{
	win_thread_key_t cur;
	win_thread_keys_t keys ;
	
	keys = thread->keys;
	
	if(!keys)
		return 1;
	
	cur = keys->front;
		
	while(cur){
		
		if(cur == key){
			
			if(cur->next)
				cur->next->prev = cur->prev;
				
			if(cur->prev)
				cur->prev->next = cur->next;
				
			return 0;
		}
		
		cur = cur->next;
	}
	
	return 1;
}





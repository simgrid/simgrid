/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donnassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/log.h"


XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_synchro, simix,
				"Logging specific to SIMIX (synchronization)");


/****************************** Synchronization *******************************/

/*********************************** Mutex ************************************/

/**
 * \brief Initialize a mutex.
 *
 * Allocs and creates the data for the mutex. It have to be called before the utilisation of the mutex.
 * \return A mutex
 */
smx_mutex_t SIMIX_mutex_init()
{
	smx_mutex_t m = xbt_new0(s_smx_mutex_t,1);
	s_smx_process_t p; /* useful to initialize sleeping swag */
	/* structures initialization */
	m->using = 0;
	m->sleeping = xbt_swag_new(xbt_swag_offset(p, synchro_hookup));
	return m;
}

/**
 * \brief Locks a mutex.
 *
 * Tries to lock a mutex, if the mutex isn't used yet, the process can continue its execution, else it'll be blocked here. You have to call #SIMIX_mutex_unlock to free the mutex.
 * \param mutex The mutex
 */
void SIMIX_mutex_lock(smx_mutex_t mutex)
{
	smx_process_t self = SIMIX_process_self();
	xbt_assert0((mutex != NULL), "Invalid parameters");
	

	if (mutex->using) {
		/* somebody using the mutex, block */
		xbt_swag_insert(self, mutex->sleeping);
		self->simdata->mutex = mutex;
		/* wait for some process make the unlock and wake up me from mutex->sleeping */
		xbt_context_yield();
		self->simdata->mutex = NULL;

		/* verify if the process was suspended */
		while (self->simdata->suspended) {
			xbt_context_yield();
		}

		mutex->using = 1;
	}
	else {
		/* mutex free */
		mutex->using = 1;
	}
	return;
}

/**
 * \brief Tries to lock a mutex.
 *
 * Tries to lock a mutex, return 1 if the mutex is free, 0 else. This function does not block the process if the mutex is used.
 * \param mutex The mutex
 * \return 1 - mutex free, 0 - mutex used
 */
int SIMIX_mutex_trylock(smx_mutex_t mutex)
{
	xbt_assert0((mutex != NULL), "Invalid parameters");
	
	if (mutex->using) 
		return 0;
	else {
		mutex->using = 1;
		return 1;
	}
}

/**
 * \brief Unlocks a mutex.
 *
 * Unlocks the mutex and wakes up a process blocked on it. If there are no process sleeping, only sets the mutex as free.
 * \param mutex The mutex
 */
void SIMIX_mutex_unlock(smx_mutex_t mutex)
{
	smx_process_t p;	/*process to wake up */
	
	xbt_assert0((mutex != NULL), "Invalid parameters");
	
	if (xbt_swag_size(mutex->sleeping) > 0) {
		p = xbt_swag_extract(mutex->sleeping);
		mutex->using = 0;
		xbt_swag_insert(p, simix_global->process_to_run);
	}
	else {
		/* nobody to wake up */
		mutex->using = 0;
	}
	return;
}

/**
 * \brief Destroys a mutex.
 *
 * Destroys and frees the mutex's memory. 
 * \param mutex A mutex
 */
void SIMIX_mutex_destroy(smx_mutex_t mutex)
{
	if ( mutex == NULL )
		return ;
	else {
		xbt_swag_free(mutex->sleeping);
		xbt_free(mutex);
		return ;
	}
}

/******************************** Conditional *********************************/

/**
 * \brief Initialize a condition.
 *
 * Allocs and creates the data for the condition. It have to be called before the utilisation of the condition.
 * \return A condition
 */
smx_cond_t SIMIX_cond_init()
{
	smx_cond_t cond = xbt_new0(s_smx_cond_t,1);
	s_smx_process_t p;
	
	cond->sleeping = xbt_swag_new(xbt_swag_offset(p,synchro_hookup));
	cond->actions = xbt_fifo_new();
	cond->mutex = NULL;
	return cond;
}

/**
 * \brief Signalizes a condition.
 *
 * Signalizes a condition and wakes up a sleping process. If there are no process sleeping, no action is done.
 * \param cond A condition
 */
void SIMIX_cond_signal(smx_cond_t cond)
{
	xbt_assert0((cond != NULL), "Invalid parameters");
	smx_process_t proc = NULL;

	if (xbt_swag_size(cond->sleeping) >= 1) {
		proc = xbt_swag_extract(cond->sleeping);
		xbt_swag_insert(proc, simix_global->process_to_run);
	}

	return;
}

/**
 * \brief Waits on a condition.
 *
 * Blocks a process until the signal is called. This functions frees the mutex associated and locks it after its execution.
 * \param cond A condition
 * \param mutex A mutex
 */
void SIMIX_cond_wait(smx_cond_t cond,smx_mutex_t mutex)
{
	smx_action_t act_sleep;
	xbt_assert0((mutex != NULL), "Invalid parameters");
	
	cond->mutex = mutex;

	SIMIX_mutex_unlock(mutex);
	/* create an action null only if there are no actions already on the condition, usefull if the host crashs */
	if (xbt_fifo_size(cond->actions) ==0 ) {
		act_sleep = SIMIX_action_sleep(SIMIX_host_self(), -1);
		SIMIX_register_action_to_condition(act_sleep,cond);
		SIMIX_register_condition_to_action(act_sleep,cond);
		__SIMIX_cond_wait(cond);
		xbt_fifo_pop(act_sleep->cond_list);
		SIMIX_action_destroy(act_sleep);
	}
	else {
		__SIMIX_cond_wait(cond);
	}
	/* get the mutex again */
	SIMIX_mutex_lock(cond->mutex);

	return;
}

xbt_fifo_t SIMIX_cond_get_actions(smx_cond_t cond)
{
	xbt_assert0((cond != NULL), "Invalid parameters");
	return cond->actions;
}

void __SIMIX_cond_wait(smx_cond_t cond)
{
	smx_process_t self = SIMIX_process_self();
	xbt_assert0((cond != NULL), "Invalid parameters");
	
	/* process status */	

	self->simdata->cond = cond;
	xbt_swag_insert(self, cond->sleeping);
	xbt_context_yield();
	self->simdata->cond = NULL;
	while (self->simdata->suspended) {
		xbt_context_yield();
	}
	return;

}

/**
 * \brief Waits on a condition with timeout.
 *
 * Same behavior of #SIMIX_cond_wait, but waits a maximum time.
 * \param cond A condition
 * \param mutex A mutex
 * \param max_duration Timeout time
 */
void SIMIX_cond_wait_timeout(smx_cond_t cond,smx_mutex_t mutex, double max_duration)
{
	xbt_assert0((mutex != NULL), "Invalid parameters");
	smx_action_t act_sleep;

	cond->mutex = mutex;

	SIMIX_mutex_unlock(mutex);
	if (max_duration >=0) {
		act_sleep = SIMIX_action_sleep(SIMIX_host_self(), max_duration);
		SIMIX_register_action_to_condition(act_sleep,cond);
		SIMIX_register_condition_to_action(act_sleep,cond);
		__SIMIX_cond_wait(cond);
		xbt_fifo_remove(act_sleep->cond_list,cond);
		SIMIX_action_destroy(act_sleep);
	}
	else
		__SIMIX_cond_wait(cond);

	/* get the mutex again */
	SIMIX_mutex_lock(cond->mutex);

	return;
}

/**
 * \brief Broadcasts a condition.
 *
 * Signalizes a condition and wakes up ALL sleping process. If there are no process sleeping, no action is done.
 * \param cond A condition
 */
void SIMIX_cond_broadcast(smx_cond_t cond)
{
	xbt_assert0((cond != NULL), "Invalid parameters");
	smx_process_t proc = NULL;
	smx_process_t proc_next = NULL;

	xbt_swag_foreach_safe(proc,proc_next,cond->sleeping) {
		xbt_swag_remove(proc,cond->sleeping);
		xbt_swag_insert(proc, simix_global->process_to_run);
	}

	return;
}

/**
 * \brief Destroys a contidion.
 *
 * Destroys and frees the condition's memory. 
 * \param cond A condition
 */
void SIMIX_cond_destroy(smx_cond_t cond)
{
	if ( cond == NULL )
		return ;
	else {
		xbt_assert0( xbt_swag_size(cond->sleeping) == 0 , "Cannot destroy conditional");
		xbt_swag_free(cond->sleeping);
		xbt_fifo_free(cond->actions);
		xbt_free(cond);
		return;
	}
}

/**
 * 	\brief Set a condition to an action
 *
 * 	Creates the "link" between an action and a condition. You have to call this function when you create an action and want to wait its ending. 
 *	\param action SIMIX action
 *	\param cond SIMIX cond
 */
void SIMIX_register_condition_to_action(smx_action_t action, smx_cond_t cond)
{
	xbt_assert0( (action != NULL) && (cond != NULL), "Invalid parameters");

	xbt_fifo_push(action->cond_list,cond);
}



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
smx_mutex_t SIMIX_mutex_init()
{
	smx_mutex_t m = xbt_new0(s_smx_mutex_t,1);
	s_smx_process_t p; /* useful to initialize sleeping swag */
	/* structures initialization */
	m->using = 0;
	m->sleeping = xbt_swag_new(xbt_swag_offset(p, synchro_hookup));
	return m;
}

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
		/* verify if the process was suspended */
		while (self->simdata->suspended) {
			xbt_context_yield();
		}

		self->simdata->mutex = NULL;
		mutex->using = 1;
	}
	else {
		/* mutex free */
		mutex->using = 1;
	}
	return;
}

/* return 1 if the process got the mutex, else 0. */
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
smx_cond_t SIMIX_cond_init()
{
	smx_cond_t cond = xbt_new0(s_smx_cond_t,1);
	s_smx_process_t p;
	
	cond->sleeping = xbt_swag_new(xbt_swag_offset(p,synchro_hookup));
	cond->actions = xbt_fifo_new();
	cond->mutex = NULL;
	return cond;
}

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

void SIMIX_cond_wait(smx_cond_t cond,smx_mutex_t mutex)
{
	smx_process_t self = SIMIX_process_self();
	xbt_assert0((mutex != NULL), "Invalid parameters");
	
	cond->mutex = mutex;

	SIMIX_mutex_unlock(mutex);
	__SIMIX_cond_wait(cond);
	/* get the mutex again */
	self->simdata->mutex = cond->mutex;
	SIMIX_mutex_lock(cond->mutex);

	return;
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

void SIMIX_cond_wait_timeout(smx_cond_t cond,smx_mutex_t mutex, double max_duration)
{
	smx_process_t self = SIMIX_process_self();
	xbt_assert0((mutex != NULL), "Invalid parameters");
	smx_action_t act_sleep;

	cond->mutex = mutex;

	SIMIX_mutex_unlock(mutex);
	if (max_duration >=0) {
		act_sleep = SIMIX_action_sleep(SIMIX_host_self(), max_duration);
		SIMIX_register_action_to_condition(act_sleep,cond);
		SIMIX_register_condition_to_action(act_sleep,cond);
	}
	__SIMIX_cond_wait(cond);

	/* get the mutex again */
	self->simdata->mutex = cond->mutex;
	SIMIX_mutex_lock(cond->mutex);

	return;
}

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

void SIMIX_register_condition_to_action(smx_action_t action, smx_cond_t cond)
{
	xbt_assert0( (action != NULL) && (cond != NULL), "Invalid parameters");

	xbt_fifo_push(action->simdata->cond_list,cond);
}



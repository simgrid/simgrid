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
		__SIMIX_process_block(-1,"");
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
		__SIMIX_process_unblock(p);
	}
	else {
		/* nobody to wape up */
		mutex->using = 0;
	}
	return;
}

SIMIX_error_t SIMIX_mutex_destroy(smx_mutex_t mutex)
{
	if ( mutex == NULL )
		return SIMIX_WARNING;
	else {
		xbt_swag_free(mutex->sleeping);
		xbt_free(mutex);
		return SIMIX_OK;
	}
}

/******************************** Conditional *********************************/
smx_cond_t SIMIX_cond_init()
{
	return xbt_new0(s_smx_cond_t,1);
}

void SIMIX_cond_signal(smx_cond_t cond)
{
	return;
}

void SIMIX_cond_wait(smx_cond_t cond,smx_mutex_t mutex)
{
	return;
}

void SIMIX_cond_wait_timeout(smx_cond_t cond,smx_mutex_t mutex, double max_duration)
{
	return;
}

void SIMIX_cond_broadcast(smx_cond_t cond)
{
	return;
}

void SIMIX_cond_destroy(smx_cond_t cond)
{
	return;
}

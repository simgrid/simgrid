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
	return xbt_new0(s_smx_mutex_t,1);
}

void SIMIX_mutex_lock(smx_mutex_t mutex)
{
	return;
}

void SIMIX_mutex_trylock(smx_mutex_t mutex)
{
	return;
}

void SIMIX_mutex_unlock(smx_mutex_t mutex)
{
	return;
}

void SIMIX_mutex_destroy(smx_mutex_t mutex)
{
	return;
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

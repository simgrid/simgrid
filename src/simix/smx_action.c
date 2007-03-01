/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donnassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_action, simix,
				"Logging specific to SIMIX (action)");

/************************************* Actions *********************************/

smx_action_t SIMIX_communicate(smx_host_t sender,smx_host_t receiver, double size)
{
	return xbt_new0(s_smx_action_t,1);
}

smx_action_t SIMIX_execute(smx_host_t host,double amount)
{
	return xbt_new0(s_smx_action_t,1);
}

SIMIX_error_t SIMIX_action_cancel(smx_action_t action)
{

	return SIMIX_OK;
}

void SIMIX_action_set_priority(smx_action_t action, double priority)
{
	return;
}

SIMIX_error_t SIMIX_action_destroy(smx_action_t action)
{
	return SIMIX_OK;
}

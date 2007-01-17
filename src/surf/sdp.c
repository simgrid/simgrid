/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Pedro Velho. All rights reserved.     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "maxmin_private.h"
#include <stdlib.h>
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_sdp, surf,
				"Logging specific to SURF (sdp)");

void sdp_solve(lmm_system_t sys)
{
/*   lmm_variable_t var = NULL; */
/*   lmm_constraint_t cnst = NULL; */
/*   lmm_element_t elem = NULL; */
/*   xbt_swag_t cnst_list = NULL; */
/*   xbt_swag_t var_list = NULL; */
/*   xbt_swag_t elem_list = NULL; */
/*   double min_usage = -1; */

  if (!(sys->modified))
    return;

}

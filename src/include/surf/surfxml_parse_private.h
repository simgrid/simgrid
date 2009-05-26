/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_SURFXML_PARSE_PRIVATE_H
#define _SURF_SURFXML_PARSE_PRIVATE_H

#include <stdio.h>
#include "xbt/misc.h"
#include "xbt/function_types.h"
#include "surf/surfxml_parse.h"
#include "surf/trace_mgr.h"

void surf_parse_get_trace(tmgr_trace_t * trace, const char *string);

#endif

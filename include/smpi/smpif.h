/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPIF_H
#define SMPIF_H

#include "gras_config.h" //For HAVE_SMPI_F2C_H

#ifdef HAVE_SMPI_F2C_H
	#undef min //Need because first def in xbt.h
	#undef max //Need because first def in xbt.h
	#include <f2c.h>
#endif

#include <xbt/misc.h>

XBT_PUBLIC(int) smpi_process_argc(void);
XBT_PUBLIC(int) smpi_process_getarg(integer* index, char* dst, ftnlen len);
XBT_PUBLIC(int) smpi_global_rank(void);
XBT_PUBLIC(int) smpi_global_size(void);

#endif

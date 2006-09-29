#ifndef __XBT_WIN32_CONFIG_H__
#define __XBT_WIN32_CONFIG_H__


/* config.h - simgrid config selection for windows platforms. */

/* Copyright (c) 2003, 2004 Cherier Malek. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* 
 * config selection. 
*/
#include <compiler/select_compiler_features.h>


#if defined(_XBT_BORLAND_COMPILER)
# include <compiler/borland.h>
# else
# error "Unknown compiler - please report the problems to the main simgrid mailing list (http://gforge.inria.fr/mail/?group_id=12)"
#endif

	

#endif /* #ifndef __XBT_WIN32_CONFIG_H__ */
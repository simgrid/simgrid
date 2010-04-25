/* ex -- exception handling                                                 */
/* This file is to loaded in any location defining exception handlers       */
/* (such as context.c) or in gras transport layer, to exchange them.        */

/* Copyright (c) 2003, 2004, 2007 Martin Quinson. All rights reserved.      */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_EX_INTERFACE_H_
#define _XBT_EX_INTERFACE_H_

#include "xbt/ex.h"
/* The display made by an uncatched exception */
void xbt_ex_display(xbt_ex_t * e);

/* Change raw libc symbols to file names and line numbers */
void xbt_ex_setup_backtrace(xbt_ex_t * e);

#endif /* _XBT_EX_INTERFACE_H_ */

/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_CONTEXT_H
#define _XBT_CONTEXT_H

typedef struct s_xbt_context *xbt_context_t;
typedef int(*xbt_context_function_t)(int argc, char *argv[]);

void xbt_context_init(void);
void xbt_context_exit(void);
void xbt_context_empty_trash(void);
xbt_context_t xbt_context_new(xbt_context_function_t code, int argc, char *argv[]);
void xbt_context_start(xbt_context_t context);
void xbt_context_yield(void);
void xbt_context_schedule(xbt_context_t context);

#endif				/* _XBT_CONTEXT_H */

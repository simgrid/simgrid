/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_CONTEXT_H
#define _XBT_CONTEXT_H

typedef struct s_context *context_t;
typedef int(*context_function_t)(int argc, char *argv[]);

void context_init(void);
void context_empty_trash(void);
context_t context_create(context_function_t code, int argc, char *argv[]);
void context_start(context_t context);
void context_yield(context_t context);
int context_get_id(context_t context);

#endif				/* _XBT_CONTEXT_H */

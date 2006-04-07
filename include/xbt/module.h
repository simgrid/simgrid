/* $Id$ */

/* module - modularize the code                                             */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_MODULE_H
#define _XBT_MODULE_H

extern char *xbt_binary_name;

typedef struct xbt_module_ xbt_module_t;

typedef xbt_module_t (*xbt_module_new_fct_t)(int argc, char **argv);
typedef int (*xbt_module_finalize_fct_t)(void);

void xbt_init(int *argc,char **argv);
void xbt_exit(void);
#endif /* _XBT_MODULE_H */

/* $Id$ */

/* module - modularize the code                                             */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 the Martin Quinson.                                   */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _GRAS_MODULE_H
#define _GRAS_MODULE_H

typedef struct gras_module_ gras_module_t;

typedef gras_module_t (*gras_module_new_fct_t)(int argc, char **argv);
typedef int (*gras_module_finalize_fct_t)(void);

void gras_init(int argc,char **argv);
void gras_finalize(void);
#endif /* _GRAS_MODULE_H */

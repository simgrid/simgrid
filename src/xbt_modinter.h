/* xbt_modinter - How to init/exit the XBT modules                          */

/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_MODINTER_H
#define XBT_MODINTER_H
#include "xbt/misc.h"

SG_BEGIN_DECL

/* Modules definitions */

void xbt_log_preinit(void);
void xbt_log_postexit(void);

void xbt_dict_preinit(void);
void xbt_dict_postexit(void);

void *mmalloc_preinit(void);
void mmalloc_postexit(void);

extern int smx_cleaned;
extern int xbt_initialized;

SG_END_DECL

#endif

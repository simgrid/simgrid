/* xbt_modinter - How to init/exit the XBT modules                          */

/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_MODINTER_H
#define XBT_MODINTER_H
#include "xbt/misc.h"

/* Modules definitions */
void xbt_backtrace_init(void);
void xbt_backtrace_exit(void);

void xbt_log_init(int *argc, char **argv);
void xbt_log_exit(void);
void xbt_fifo_exit(void);
void xbt_dict_exit(void);

void xbt_os_thread_mod_init(void);
void xbt_os_thread_mod_exit(void);

#endif /* XBT_MODINTER_H */

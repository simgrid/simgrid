/* $Id$ */

/* xbt_modinter - How to init/exit the XBT modules                          */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_MODINTER_H
#define XBT_MODINTER_H

/* Modules definitions */
void xbt_log_init(int *argc,char **argv);
void xbt_log_exit(void);
void xbt_fifo_exit(void);
void xbt_dict_exit(void);

#endif /* XBT_MODINTER_H */

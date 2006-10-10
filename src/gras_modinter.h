/* $Id$ */

/* gras_modinter.h - How to init/exit the GRAS modules                      */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_MODINTER_H
#define GRAS_MODINTER_H
#include <xbt/misc.h> /* XBT_PUBLIC */

/* modules initialization functions */
XBT_PUBLIC void gras_emul_init(void);
XBT_PUBLIC void XBT_PUBLIC gras_emul_exit(void); 

XBT_PUBLIC void gras_msg_register(void);
XBT_PUBLIC void gras_msg_init(void);
XBT_PUBLIC void gras_msg_exit(void);
XBT_PUBLIC void gras_trp_register(void);
XBT_PUBLIC void gras_trp_init(void);
XBT_PUBLIC void gras_trp_exit(void);
XBT_PUBLIC void gras_datadesc_init(void);
XBT_PUBLIC void gras_datadesc_exit(void);

XBT_PUBLIC void gras_procdata_init(void);
XBT_PUBLIC void gras_procdata_exit(void);

#endif /* GRAS_MODINTER_H */

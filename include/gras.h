/* $Id$ */

/* gras.h - Public interface to the GRAS                                    */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_H
#define GRAS_H

#include <xbt.h>                /* our toolbox */
#include <xbt/ex.h>             /* There's a whole bunch of exception handling in GRAS */

#include <gras/process.h>
#include <gras/module.h>
#include <gras/virtu.h>
#include <gras/emul.h>

#include <gras/transport.h>
#include <gras/datadesc.h>
#include <gras/messages.h>
#include <gras/timer.h>

XBT_PUBLIC(void) gras_init(int *argc, char **argv);
XBT_PUBLIC(void) gras_exit(void);

#endif /* GRAS_H */

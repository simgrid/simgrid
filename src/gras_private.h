/* $Id$ */

/* gras_private.h - GRAS private definitions                                */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_PRIVATE_H
#define GRAS_PRIVATE_H

#define max(a, b) (((a) > (b))?(a):(b))
#define min(a, b) (((a) < (b))?(a):(b))

#define TRUE  1
#define FALSE 0

#define GRAS_MAX_CHANNEL 10 /* FIXME: killme */

#include "gras_config.h"

#include "xbt/sysdep.h"

#include "xbt/error.h"
#include "xbt/log.h"
#include "xbt/module.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"
#include "xbt/set.h"
#include "xbt/config.h"

#include "gras/core.h"
#include "gras/process.h"
#include "gras/virtu.h"
#include "gras/cond.h"

#include "gras/transport.h"
#include "gras/datadesc.h"
#include "gras/messages.h"

/* modules initialization functions */
void gras_msg_init(void);
void gras_msg_exit(void);
void gras_trp_init(void);
void gras_trp_exit(void);
void gras_datadesc_init(void);
void gras_datadesc_exit(void);

#endif /* GRAS_PRIVATE_H */

/* $Id$ */

/* gras_private.h - GRAS private definitions                                */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_PRIVATE_H
#define GRAS_PRIVATE_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define max(a, b) (((a) > (b))?(a):(b))
#define min(a, b) (((a) < (b))?(a):(b))

#define TRUE  1
#define FALSE 0

#define GRAS_MAX_CHANNEL 10 /* FIXME: killme */

#include "gras_config.h"

#include "gros/error.h"
#include "gros/log.h"
#include "gros/module.h"
#include "gros/dynar.h"
#include "gros/dict.h"
#include "gros/set.h"
#include "gros/config.h"

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
gras_error_t gras_trp_init(void); /* FIXME */
void         gras_trp_exit(void);
void gras_datadesc_init(void);
void gras_datadesc_exit(void);

#endif /* GRAS_PRIVATE_H */

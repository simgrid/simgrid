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

#include "gras_config.h"

#include "gras/error.h"
#include "gras/log.h"
#include "gras/module.h"
#include "gras/dynar.h"
#include "gras/dict.h"
#include "gras/set.h"
#include "gras/config.h"

#include "gras/core.h"
#include "gras/process.h"

#include "gras/transport.h"
#include "Transport/transport_interface.h"
#include "gras/datadesc.h"
#include "DataDesc/datadesc_interface.h"
#include "gras/messages.h"
#include "Messaging/messaging_interface.h"

#include "Virtu/virtu_interface.h"

#endif /* GRAS_PRIVATE_H */

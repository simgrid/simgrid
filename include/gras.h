/* $Id$ */

/* gras.h - Public interface to the GRAS                                    */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_H
#define GRAS_H

/* Oli's macro */
#ifndef GS_FAILURE_CONTEXT
#  define GS_FAILURE_CONTEXT
#endif /* GS_FAILURE_CONTEXT */

#define GS_FAILURE(str) \
     (fprintf(stderr, "FAILURE: %s(%s:%d)" GS_FAILURE_CONTEXT "%s\n", __func__, __FILE__, __LINE__, (str)), \
      abort())

#define max(a, b) (((a) > (b))?(a):(b))
#define min(a, b) (((a) < (b))?(a):(b))

#define TRUE  1
#define FALSE 0

/* end of Oli's cruft */

#include <gras/error.h>
#include <gras/log.h>

#include <gras/module.h>

#include <gras/dynar.h>
#include <gras/dict.h>
#include <gras/set.h>

#include <gras/config.h>

#include <gras/core.h>

#include <gras/transport.h>
#include <gras/datadesc.h>
//#include <gras/datadesc_simple.h>
#include <gras/socket.h>
#include <gras/messages.h>

#include <gras/data_description.h>
#include <gras/dd_type_bag.h>

#include <gras/modules/base.h>
#include <gras/modules/bandwidth.h>

#endif /* GRAS_H */

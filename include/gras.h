/* $Id$ */

/* gras.h - Public interface to the GRAS                                    */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_H
#define GRAS_H

#define max(a, b) (((a) > (b))?(a):(b))
#define min(a, b) (((a) < (b))?(a):(b))

#define TRUE  1
#define FALSE 0

#include <gras/error.h>
#include <gras/log.h>

#include <gras/module.h>

#include <gras/dynar.h>
#include <gras/dict.h>
#include <gras/set.h>

#include <gras/config.h>

#include <gras/core.h>
#include <gras/process.h>


#include <gras/transport.h>
#include <gras/datadesc.h>
#include <gras/messages.h>

#include <gras/modules/base.h>
#include <gras/modules/bandwidth.h>

#endif /* GRAS_H */

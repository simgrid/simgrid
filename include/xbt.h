/* $Id$ */

/* gros.h - Public interface to the GROS (gras's toolbox)                   */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GROS_H
#define GROS_H

#define max(a, b) (((a) > (b))?(a):(b))
#define min(a, b) (((a) < (b))?(a):(b))

#define TRUE  1
#define FALSE 0

#define GRAS_MAX_CHANNEL 10 /* FIXME: killme */

#include <gros/error.h>
#include <gros/log.h>

#include <gros/module.h>

#include <gros/dynar.h>
#include <gros/dict.h>
#include <gros/set.h>

#include <gros/config.h>

#endif /* GROS_H */

/* $Id$ */

/* xbt.h - Public interface to the xbt (gras's toolbox)                   */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef xbt_H
#define xbt_H

#define max(a, b) (((a) > (b))?(a):(b))
#define min(a, b) (((a) < (b))?(a):(b))

#define TRUE  1
#define FALSE 0

#define XBT_MAX_CHANNEL 10 /* FIXME: killme */

#include <xbt/sysdep.h>

#include <xbt/error.h>
#include <xbt/log.h>

#include <xbt/module.h>

#include <xbt/dynar.h>
#include <xbt/dict.h>
#include <xbt/set.h>

#include <xbt/config.h>

#endif /* xbt_H */

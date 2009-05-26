/* $Id$ */

/* a generic DYNamic ARray implementation.                                  */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef DYNAR_PRIVATE_H
#define DYNAR_PRIVATE_H

#include "xbt/synchro.h"
typedef struct xbt_dynar_s {
  unsigned long size;
  unsigned long used;
  unsigned long elmsize;
  void *data;
  void_f_pvoid_t free_f;
  xbt_mutex_t mutex;
} s_xbt_dynar_t;

#endif /* DYNAR_PRIVATE_H */

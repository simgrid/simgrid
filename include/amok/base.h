/* amok_base - things needed in amok, but too small to constitute a module  */

/* Copyright (c) 2004, 2005, 2006, 2007, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef AMOK_BASE_H
#define AMOK_BASE_H

#include "gras/messages.h"

/* ****************************************************************************
 * The common types used as payload in the messages and their definitions
 * ****************************************************************************/

/**
 * amok_result_t:
 *
 * how to report the result of an experiment
 */

typedef struct {
  unsigned int timestamp;
  double value;
} amok_result_t;

XBT_PUBLIC(void) amok_base_init(void);
XBT_PUBLIC(void) amok_base_exit(void);


#endif /* AMOK_BASE_H */

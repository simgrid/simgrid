/* Copyright (c) 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg_private.h"
#include "simgrid/sg_config.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"

/** \brief set a configuration variable
 *
 * Do --help on any simgrid binary to see the list of currently existing configuration variables
 *
 * Example:
 * MSG_config("workstation/model","KCCFLN05");
 */
void MSG_config(const char *name, ...)
{
  va_list pa;

  if (!msg_global) {
    fprintf(stderr,
            "ERROR: Please call MSG_init() before using MSG_config()\n");
    xbt_abort();
  }
  va_start(pa, name);
  xbt_cfg_set_vargs(_sg_cfg_set, name, pa);
  va_end(pa);
  return;
}

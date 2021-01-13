/* check_defaults -- simple program displaying its context factory          */

/* Copyright (c) 2013-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/simix.h"
#include "xbt/log.h"

int main(int argc, char* argv[])
{
  xbt_log_control_set("root.fmt:[%c/%p]%e%m%n");
  xbt_log_control_set("simix_context.threshold:verbose");
  SIMIX_global_init(&argc, argv);
  return 0;
}

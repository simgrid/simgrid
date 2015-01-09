/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <errno.h>

#include <unistd.h>

#include "mc_base.h"

int main(int argc, char** argv)
{
  if (argc < 2)
    xbt_die("Missing arguments.\n");

  setenv(MC_ENV_VARIABLE, "1", 1);
  execvp(argv[1], argv+1);

  perror("simgrid-mc");
  return 127;
}

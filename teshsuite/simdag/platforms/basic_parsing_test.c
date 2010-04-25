/* Copyright (c) 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"

int main(int argc, char **argv)
{
  /* initialisation of SD */
  SD_init(&argc, argv);

  /* creation of the environment */
  SD_create_environment(argv[1]);

  SD_exit();
  return 0;
}

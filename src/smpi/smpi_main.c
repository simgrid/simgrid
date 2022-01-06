/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>

#include <smpi/smpi.h>

int main(int argc, char **argv)
{
  if (getenv("SMPI_PRETEND_CC") != NULL) {
    /* Hack to ensure that smpicc can pretend to be a simple compiler. Particularly handy to pass it to the
     * configuration tools. This one is used with dlopen privatization. */
    return 0;
  }

  if (argc < 2) {
    fprintf(stderr, "Usage: smpimain <program to launch>\n");
    exit(1);
  }
  return smpi_main(argv[1], argc - 1, argv + 1);
}

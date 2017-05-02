/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>

#include <smpi/smpi.h>

int main(int argc, char **argv)
{
  if (argc < 2) {
    fprintf(stderr, "Usage: smpi_main <program to launch>\n");
    exit(1);
  }
  return smpi_main(argv[1], argc - 1, argv + 1);
}

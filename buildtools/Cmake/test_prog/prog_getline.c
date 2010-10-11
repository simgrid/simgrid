/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define _GNU_SOURCE
#include <stdio.h>
int main(void)
{
  FILE *fp;
  char *line = NULL;
  size_t len = 0;
  getline(&line, &len, fp);
}

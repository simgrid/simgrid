/* Copyright (c) 2010, 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>

static int iterate = 10;
static int growsdown(int *x)
{
  int y;
  y = (x > &y);
  if (--iterate > 0)
    y = growsdown(&y);
  if (y != (x > &y))
    exit(1);
  return y;
}

int main(int argc, char *argv[])
{
  int x;
  printf("%s", growsdown(&x) ? "down" : "up");
  exit(0);
  return 1;
}

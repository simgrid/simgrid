//This programme check size of datatypes 

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include <sys/types.h>
#include <stdio.h>

int main(void)
{

  int c = sizeof(char);
  int si = sizeof(short int);
  int i = sizeof(int);
  int li = sizeof(long int);
  int lli = sizeof(long long int);
  int f = sizeof(float);
  int v = sizeof(void *);
  int vv = sizeof(void (*)(void));
  int t[8] = { c, si, i, li, lli, f, v, vv };

  int max = t[0];

  for (i = 1; i < 8; i++) {
    if (t[i] > max)
      max = t[i];

  }
  printf("%d", max);
  return 1;
}

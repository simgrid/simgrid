/* Copyright (c) 2010-2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>

__thread int thread_specific_variable = 0;

int main(void) {

  thread_specific_variable++;
  printf("%d\n", thread_specific_variable);
  return 0;
}

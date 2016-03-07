/* Copyright (c) 2010, 2014, 2016. The SimGrid Team. All rights reserved.   */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifdef __APPLE__
#define _XOPEN_SOURCE 700
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

ucontext_t uc_child;
ucontext_t uc_main;

static void child(void)
{
  /* switch back to the main context */
  if (swapcontext(&uc_child, &uc_main) != 0)
    exit(2);
}

int main(int argc, char *argv[])
{
  void *stack = malloc(64 * 1024);

  /* configure a child user-space context */
  if (stack == NULL)
    exit(3);
  if (getcontext(&uc_child) != 0)
    exit(4);
  uc_child.uc_link = NULL;
  uc_child.uc_stack.ss_sp = (char *) stack + (32 * 1024);
  uc_child.uc_stack.ss_size = 32 * 1024;
  uc_child.uc_stack.ss_flags = 0;
  makecontext(&uc_child, child, 0);

  /* switch into the user context */
  if (swapcontext(&uc_main, &uc_child) != 0)
    exit(5);

  /* Fine, child came home */
  printf("yes\n");

  /* die successfully */
  return 0;
}

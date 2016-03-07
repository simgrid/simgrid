/* Copyright (c) 2010, 2014-2016. The SimGrid Team. All rights reserved.    */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifdef __APPLE__
#define _XOPEN_SOURCE 700
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

union alltypes {
  long l;
  double d;
  void *vp;
  void (*fp) (void);
  char *cp;
};
static volatile char *handler_addr = (char *) 0xDEAD;
static ucontext_t uc_handler;
static ucontext_t uc_main;
void handler(void)
{
  char garbage[1024];
  auto int dummy;
  for (int i = 0; i < 1024; i++)
    garbage[i] = 'X';
  handler_addr = (char *) &dummy;
  swapcontext(&uc_handler, &uc_main);
  return;
}

int main(int argc, char *argv[])
{
  int sksize = 32768;
  char *skbuf = (char *) malloc(sksize * 2 + 2 * sizeof(union alltypes));
  if (skbuf == NULL)
    exit(1);
  for (int i = 0; i < sksize * 2 + 2 * sizeof(union alltypes); i++)
    skbuf[i] = 'A';
  char *skaddr = skbuf + sizeof(union alltypes);

  if (getcontext(&uc_handler) != 0)
    exit(1);
  uc_handler.uc_link = NULL;
  uc_handler.uc_stack.ss_sp = (void *) (skaddr + sksize);
  uc_handler.uc_stack.ss_size = sksize;
  uc_handler.uc_stack.ss_flags = 0;
  makecontext(&uc_handler, handler, 0);
  swapcontext(&uc_main, &uc_handler);

  if (handler_addr == (char *) 0xDEAD)
    exit(1);
  if (handler_addr < skaddr + sksize) {
    /* stack was placed into lower area */
    if (*(skaddr + sksize) != 'A')
      printf("(skaddr)+(sksize)-%d;(sksize)-%d", sizeof(union alltypes), sizeof(union alltypes));
    else
      printf("(skaddr)+(sksize);(sksize)");
  } else {
    /* stack was placed into higher area */
    if (*(skaddr + sksize * 2) != 'A')
      printf("(skaddr);(sksize)-%d", sizeof(union alltypes));
    else
      printf("(skaddr);(sksize)");
  }
  return 0;
}

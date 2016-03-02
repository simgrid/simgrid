/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/mmalloc.h"
#include "xbt.h"
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(test,"this test");

#define BUFFSIZE 204800
#define TESTSIZE 100
#define size_of_block(i) (((i % 50)+1)* 100)

int main(int argc, char**argv)
{
  void *heapA;
  void *pointers[TESTSIZE];
  xbt_init(&argc,argv);

  XBT_INFO("Allocating a new heap");
  unsigned long mask = ~((unsigned long)xbt_pagesize - 1);
  void *addr = (void*)(((unsigned long)sbrk(0) + BUFFSIZE) & mask);
  heapA = xbt_mheap_new(-1, addr);
  if (heapA == NULL) {
    perror("attach 1 failed");
    fprintf(stderr, "bye\n");
    exit(1);
  }

  XBT_INFO("HeapA allocated");

  int i, size;
  for (i = 0; i < TESTSIZE; i++) {
    size = size_of_block(i);
    pointers[i] = mmalloc(heapA, size);
    XBT_INFO("%d bytes allocated with offset %tx", size, ((char*)pointers[i])-((char*)heapA));
  }
  XBT_INFO("All blocks were correctly allocated. Free every second block");
  for (i = 0; i < TESTSIZE; i+=2) {
    mfree(heapA,pointers[i]);
  }
  XBT_INFO("Memset every second block to zero (yeah, they are not currently allocated :)");
  for (i = 0; i < TESTSIZE; i+=2) {
    size = size_of_block(i);
    memset(pointers[i],0, size);
  }
  XBT_INFO("Re-allocate every second block");
  for (i = 0; i < TESTSIZE; i+=2) {
    size = size_of_block(i);
    pointers[i] = mmalloc(heapA, size);
  }

  XBT_INFO("free all blocks (each one twice, to check that double free are correctly catched)");
  for (i = 0; i < TESTSIZE; i++) {
    xbt_ex_t e;
    int gotit = 1;

    mfree(heapA, pointers[i]);
    TRY {
      mfree(heapA, pointers[i]);
      gotit = 0;
    } CATCH(e) {
      xbt_ex_free(e);
    }
    if (!gotit)
      xbt_die("FAIL: A double-free went undetected (for size:%d)",size_of_block(i));
  }

  XBT_INFO("free again all blocks (to really check that double free are correctly catched)");
  for (i = 0; i < TESTSIZE; i++) {
    xbt_ex_t e;
    int gotit = 1;

    TRY {
      mfree(heapA, pointers[i]);
      gotit = 0;
    } CATCH(e) {
      xbt_ex_free(e);
    }
    if (!gotit)
      xbt_die("FAIL: A double-free went undetected (for size:%d)",size_of_block(i));
  }

  XBT_INFO("Damnit, I cannot break mmalloc this time. That's SO disappointing.");
  return 0;
}

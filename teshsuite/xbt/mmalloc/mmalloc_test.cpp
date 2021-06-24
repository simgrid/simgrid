/* Copyright (c) 2012-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "xbt.h"
#include "xbt/mmalloc.h"

#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(test,"this test");

constexpr int BUFFSIZE = 204800;
constexpr int TESTSIZE = 100;

#define size_of_block(i) ((((i) % 50) + 1) * 100)

static void check_block(const unsigned char* p, unsigned char b, int n)
{
  for (int i = 0; i < n; i++)
    xbt_assert(p[i] == b, "value mismatch: %p[%d] = %#hhx, expected %#hhx", p, i, p[i], b);
}

int main(int argc, char**argv)
{
  xbt_mheap_t heapA = nullptr;
  std::array<void*, TESTSIZE> pointers;
  xbt_init(&argc,argv);

  XBT_INFO("Allocating a new heap");
  unsigned long mask = ~((unsigned long)xbt_pagesize - 1);
  auto* addr         = reinterpret_cast<void*>(((unsigned long)sbrk(0) + BUFFSIZE) & mask);
  heapA              = xbt_mheap_new(addr, 0);
  if (heapA == nullptr) {
    perror("attach 1 failed");
    fprintf(stderr, "bye\n");
    exit(1);
  }

  XBT_INFO("HeapA allocated");

  int i;
  int size;
  for (i = 0; i < TESTSIZE; i++) {
    size = size_of_block(i);
    pointers[i] = mmalloc(heapA, size);
    XBT_INFO("%d bytes allocated with offset %zx", size, (size_t)((char*)pointers[i] - (char*)heapA));
  }
  XBT_INFO("All blocks were correctly allocated. Free every second block");
  for (i = 0; i < TESTSIZE; i+=2) {
    mfree(heapA, pointers[i]);
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

  XBT_INFO("free all blocks (each one twice, to check that double free are correctly caught)");
  for (i = 0; i < TESTSIZE; i++) {
    bool gotit = false;
    mfree(heapA, pointers[i]);
    try {
      mfree(heapA, pointers[i]);
    } catch (const simgrid::Exception&) {
      gotit = true;
    }
    xbt_assert(gotit, "FAIL: A double-free went undetected (for size:%d)", size_of_block(i));
  }

  XBT_INFO("free again all blocks (to really check that double free are correctly caught)");
  for (i = 0; i < TESTSIZE; i++) {
    bool gotit = false;
    try {
      mfree(heapA, pointers[i]);
    } catch (const simgrid::Exception&) {
      gotit = true;
    }
    xbt_assert(gotit, "FAIL: A double-free went undetected (for size:%d)", size_of_block(i));
  }

  XBT_INFO("Let's try different codepaths for mrealloc");
  for (i = 0; i < TESTSIZE; i++) {
    const std::vector<std::pair<int, unsigned char>> requests = {
        {size_of_block(i) / 2, 0x77}, {size_of_block(i) * 2, 0xaa}, {1, 0xc0}, {0, 0}};
    pointers[i] = nullptr;
    for (unsigned k = 0; k < requests.size(); ++k) {
      size        = requests[k].first;
      pointers[i] = mrealloc(heapA, pointers[i], size);
      if (k > 0)
        check_block(static_cast<unsigned char*>(pointers[i]), requests[k - 1].second,
                    std::min(size, requests[k - 1].first));
      if (size > 0)
        memset(pointers[i], requests[k].second, size);
    }
  }

  XBT_INFO("Damnit, I cannot break mmalloc this time. That's SO disappointing.");
  return 0;
}

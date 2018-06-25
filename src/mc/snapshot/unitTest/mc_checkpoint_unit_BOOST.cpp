/* Copyright (c) 2008-2018. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* 
* TODO: 
* 1. clean:
*    Some of the '#include's should not be here
*
* 2. Add a header file for 'mc_checkpoint.cpp'
*    so that we can test the functions in 'mc_checkpoint.cpp'
*/

#include <unistd.h>

#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <link.h>

#ifndef WIN32
#include <sys/mman.h>
#endif

#include "src/internal_config.h"
#include "src/mc/mc_private.hpp"
#include "src/smpi/include/private.hpp"
#include "xbt/file.hpp"
#include "xbt/mmalloc.h"
#include "xbt/module.h"

#include "src/xbt/mmalloc/mmprivate.h"

#include "src/simix/smx_private.hpp"

#include <libunwind.h>
#include <libelf.h>

#include "src/mc/mc_private.hpp"
#include <mc/mc.h>

#include "src/mc/mc_config.hpp"
#include "src/mc/mc_hash.hpp"
#include "src/mc/mc_mmu.hpp"
#include "src/mc/mc_smx.hpp"
#include "src/mc/mc_snapshot.hpp"
#include "src/mc/mc_unw.hpp"
#include "src/mc/remote/mc_protocol.h"

#include "src/mc/RegionSnapshot.hpp"
#include "src/mc/ObjectInformation.hpp"
#include "src/mc/Frame.hpp"
#include "src/mc/Variable.hpp"

#ifdef SIMGRID_TEST

/* **BOOST** */
#define BOOST_TEST_MODULE checkpoint
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <stdio.h>    // perror()
#include <errno.h>    // perror()
#include <stdlib.h>   // exit()
#include <iostream>
#include <functional>
#include <sys/mman.h>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <cstring>

void add_content(void*, int, size_t);
size_t make_hash(char*);

int add_region_BOOST()
{
  
  RemoteClient* this_process = new RemoteClient(getpid(), -1);
  /* read /proc/getpid()/maps into "this_process->memory_map", etc. */
  this_process->init();
  simgrid::mc::Snapshot* snapshot = new simgrid::mc::Snapshot(this_process, 0); // first ckpt
  simgrid::mc::RegionType type = simgrid::mc::RegionType::Unknown;
  std::size_t PAGESIZE = 4096;
  std::size_t region_num = 10;
  std::size_t size = PAGESIZE * region_num;
  void* addr;
  int prot = PROT_READ|PROT_WRITE;
  int flags = MAP_PRIVATE|MAP_ANONYMOUS;
  if (mmap(addr, size, prot, flags, -1, 0) == (void *)-1) {
    perror("mmap()");
    exit(1);
  }
  size_t hashes[region_num];
  void* source;
  for(int i=0; i<region_num; ++i) {
    source = (char*)addr + i*PAGESIZE;
    add_content(source, i, PAGESIZE);
    add_region(i, snaphot, type, NULL, source, source, size);
  }
  
  BOOST_TEST((snapshot->snapshot_regions).size == 10, "snapshot_regions size");
  /* test retrieving regions from a snapshot*/
  void* src1;
  void* src2;
  for (int i=2; i<region_num; ++) {
  /* hashes of even regions should be equal
     and hashes of odd regions should also be equal. */
    src1 = (snapshot->snapshot_regions[i])->start_addr_;
    src2 = (snapshot->snapshot_regions[i-2])->start_addr_;
    BOOST_TEST(make_hash((char*)src1) == make_hash((char*)src2), "retrieval failed");
  }

  /* release memory before returning */
  if (munmap(addr, size) == -1) {
    perror("munmap()");
    exit(1);
  }
  return 1; // just for testing
}

// stack unwinding.

BOOST_AUTO_TEST_SUITE(checkpoint)
BOOST_AUTO_TEST_CASE(add_region_test) {
  BOOST_TEST(add_region_BOOST() == 1);
}
BOOST_AUTO_TEST_SUITE_END()

void add_content(void* addr, int index, size_t size) {
  memset(addr, index%2+1, size);
  memset(addr+size-1, 0, 1); // NULL-termination
}

st::hash<std::string> str_hash;
void make_hash(char* addr) {
  return str_hash(string(addr));
}

#endif // SIMGRID_TEST

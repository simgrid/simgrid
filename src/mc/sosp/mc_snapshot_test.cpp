/* Copyright (c) 2014-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define BOOST_TEST_MODULE snapshots
#define BOOST_TEST_DYN_LINK
bool init_unit_test(); // boost sometimes forget to give this prototype (NetBSD and other), which does not fit our paranoid flags
#include <boost/test/unit_test.hpp>

#include <cstdlib>
#include <cstring>

#include <sys/mman.h>

#include "src/mc/mc_config.hpp"
#include "src/mc/mc_mmu.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/sosp/mc_snapshot.hpp"

/**************** Class BOOST_tests *************************/
using simgrid::mc::RegionSnapshot;
class BOOST_tests {
public:
  static void init_memory(void* mem, size_t size);
  static void Init(bool sparse_ckpt);
  typedef struct {
    size_t size;
    void* src;
    void* dstn;
    RegionSnapshot region0;
    RegionSnapshot region;
  } prologue_return;
  static prologue_return prologue(int n); // common to the below 5 fxs
  static void read_whole_region();
  static void read_region_parts();
  static void compare_whole_region();
  static void compare_region_parts();
  static void read_pointer();

  static void cleanup()
  {
    delete mc_model_checker;
    mc_model_checker = nullptr;
  }

  static bool sparse_checkpoint;
  static std::unique_ptr<simgrid::mc::RemoteClient> process;
};

// static member variables init.
bool BOOST_tests::sparse_checkpoint                             = 0;
std::unique_ptr<simgrid::mc::RemoteClient> BOOST_tests::process = nullptr;

void BOOST_tests::init_memory(void* mem, size_t size)
{
  char* dest = (char*)mem;
  for (size_t i = 0; i < size; ++i) {
    dest[i] = rand() & 255;
  }
}

void BOOST_tests::Init(bool sparse_ckpt)
{
  _sg_mc_sparse_checkpoint = sparse_ckpt;
  BOOST_CHECK_EQUAL(xbt_pagesize, getpagesize());
  BOOST_CHECK_EQUAL(1 << xbt_pagebits, xbt_pagesize);

  process = std::unique_ptr<simgrid::mc::RemoteClient>(new simgrid::mc::RemoteClient(getpid(), -1));
  process->init();
  mc_model_checker = new ::simgrid::mc::ModelChecker(std::move(process));
}

BOOST_tests::prologue_return BOOST_tests::prologue(int n)
{
  // Store region page(s):
  size_t byte_size = n * xbt_pagesize;
  void* source     = mmap(nullptr, byte_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  BOOST_CHECK_MESSAGE(source != MAP_FAILED, "Could not allocate source memory");

  // Init memory and take snapshots:
  init_memory(source, byte_size);
  simgrid::mc::RegionSnapshot region0 =
      simgrid::mc::sparse_region(simgrid::mc::RegionType::Unknown, source, source, byte_size);
  for (int i = 0; i < n; i += 2) {
    init_memory((char*)source + i * xbt_pagesize, xbt_pagesize);
  }
  simgrid::mc::RegionSnapshot region =
      simgrid::mc::sparse_region(simgrid::mc::RegionType::Unknown, source, source, byte_size);

  void* destination = mmap(nullptr, byte_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  BOOST_CHECK_MESSAGE(source != MAP_FAILED, "Could not allocate destination memory");

  return {.size    = byte_size,
          .src     = source,
          .dstn    = destination,
          .region0 = std::move(region0),
          .region  = std::move(region)};
}

void BOOST_tests::read_whole_region()
{
  for (int n = 1; n != 256; ++n) {

    prologue_return ret = prologue(n);
    const void* read    = MC_region_read(&(ret.region), ret.dstn, ret.src, ret.size);
    BOOST_CHECK_MESSAGE(not memcmp(ret.src, read, ret.size), "Mismatch in MC_region_read()");

    munmap(ret.dstn, ret.size);
    munmap(ret.src, ret.size);
  }
}

void BOOST_tests::read_region_parts()
{
  for (int n = 1; n != 256; ++n) {

    prologue_return ret = prologue(n);

    for (int j = 0; j != 100; ++j) {
      size_t offset    = rand() % ret.size;
      size_t size      = rand() % (ret.size - offset);
      const void* read = MC_region_read(&(ret.region), ret.dstn, (const char*)ret.src + offset, size);
      BOOST_CHECK_MESSAGE(not memcmp((char*)ret.src + offset, read, size), "Mismatch in MC_region_read()");
    }
    munmap(ret.dstn, ret.size);
    munmap(ret.src, ret.size);
  }
}

void BOOST_tests::compare_whole_region()
{
  for (int n = 1; n != 256; ++n) {

    prologue_return ret = prologue(n);

    BOOST_CHECK_MESSAGE(MC_snapshot_region_memcmp(ret.src, &(ret.region0), ret.src, &(ret.region), ret.size),
                        "Unexpected match in MC_snapshot_region_memcmp() with previous snapshot");

    munmap(ret.dstn, ret.size);
    munmap(ret.src, ret.size);
  }
}

void BOOST_tests::compare_region_parts()
{
  for (int n = 1; n != 256; ++n) {

    prologue_return ret = prologue(n);

    for (int j = 0; j != 100; ++j) {
      size_t offset = rand() % ret.size;
      size_t size   = rand() % (ret.size - offset);
      BOOST_CHECK_MESSAGE(not MC_snapshot_region_memcmp((char*)ret.src + offset, &(ret.region), (char*)ret.src + offset,
                                                        &(ret.region), size),
                          "Mismatch in MC_snapshot_region_memcmp()");
    }
    munmap(ret.dstn, ret.size);
    munmap(ret.src, ret.size);
  }
}

void BOOST_tests::read_pointer()
{

  prologue_return ret = prologue(1);
  memcpy(ret.src, &mc_model_checker, sizeof(void*));
  simgrid::mc::RegionSnapshot region2 =
      simgrid::mc::sparse_region(simgrid::mc::RegionType::Unknown, ret.src, ret.src, ret.size);
  BOOST_CHECK_MESSAGE(MC_region_read_pointer(&region2, ret.src) == mc_model_checker,
                      "Mismtach in MC_region_read_pointer()");

  munmap(ret.dstn, ret.size);
  munmap(ret.src, ret.size);
}

/*************** End: class BOOST_tests *****************************/

namespace utf = boost::unit_test; // for test case dependence

BOOST_AUTO_TEST_SUITE(flat_snapshot)
BOOST_AUTO_TEST_CASE(Init)
{
  BOOST_tests::Init(0);
}

BOOST_AUTO_TEST_CASE(read_whole_region, *utf::depends_on("flat_snapshot/Init"))
{
  BOOST_tests::read_whole_region();
}

BOOST_AUTO_TEST_CASE(read_region_parts, *utf::depends_on("flat_snapshot/read_whole_region"))
{
  BOOST_tests::read_region_parts();
}

BOOST_AUTO_TEST_CASE(compare_whole_region, *utf::depends_on("flat_snapshot/read_region_parts"))
{
  BOOST_tests::compare_whole_region();
}

BOOST_AUTO_TEST_CASE(compare_region_parts, *utf::depends_on("flat_snapshot/compare_whole_region"))
{
  BOOST_tests::compare_region_parts();
}

BOOST_AUTO_TEST_CASE(read_pointer, *utf::depends_on("flat_snapshot/compare_region_parts"))
{
  BOOST_tests::read_pointer();
}

// not really a test, just for cleanup the resources
BOOST_AUTO_TEST_CASE(cleanup, *utf::depends_on("flat_snapshot/read_pointer"))
{
  BOOST_tests::cleanup();
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(page_snapshots)
BOOST_AUTO_TEST_CASE(Init)
{
  BOOST_tests::Init(1);
}

BOOST_AUTO_TEST_CASE(read_whole_region, *utf::depends_on("page_snapshots/Init"))
{
  BOOST_tests::read_whole_region();
}

BOOST_AUTO_TEST_CASE(read_region_parts, *utf::depends_on("page_snapshots/read_whole_region"))
{
  BOOST_tests::read_region_parts();
}

BOOST_AUTO_TEST_CASE(compare_whole_region, *utf::depends_on("page_snapshots/read_region_parts"))
{
  BOOST_tests::compare_whole_region();
}

BOOST_AUTO_TEST_CASE(compare_region_parts, *utf::depends_on("page_snapshots/compare_whole_region"))
{
  BOOST_tests::compare_region_parts();
}

BOOST_AUTO_TEST_CASE(read_pointer, *utf::depends_on("page_snapshots/compare_region_parts"))
{
  BOOST_tests::read_pointer();
}

// not really a test, just for cleanup the resources
BOOST_AUTO_TEST_CASE(cleanup, *utf::depends_on("page_snapshots/read_pointer"))
{
  BOOST_tests::cleanup();
}
BOOST_AUTO_TEST_SUITE_END()

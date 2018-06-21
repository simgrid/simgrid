/*************************************************
TODO: comment
*************************************************/
#define BOOST_TEST_MODULE snapshots
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

// /*******************************/
// /* GENERATED FILE, DO NOT EDIT */
// /*******************************/
// 
// #include <stdio.h>
// #include "xbt.h"
// /*******************************/
// /* GENERATED FILE, DO NOT EDIT */
// /*******************************/
// 
// #line 180 "mc/mc_snapshot.cpp" 

#include <cstdlib>
#include <cstring>

#include <sys/mman.h>

#include "src/mc/mc_config.hpp"
#include "src/mc/mc_mmu.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_snapshot.hpp"


static inline void init_memory(void* mem, size_t size)
{
  char* dest = (char*) mem;
  for (size_t i = 0; i < size; ++i) {
    dest[i] = rand() & 255;
  }
}

static int test_snapshot(bool sparse_checkpoint);

BOOST_AUTO_TEST_SUITE(Snapshots)
BOOST_AUTO_TEST_CASE(flat_snapshots) {
  BOOST_TEST(test_snapshot(0) == 1);
}
BOOST_AUTO_TEST_CASE(page_snapshots) {
  BOOST_TEST(test_snapshot(1) == 1);
}
BOOST_AUTO_TEST_SUITE_END()

static int test_snapshot(bool sparse_checkpoint) {

  // xbt_test_add("Initialization");
  _sg_mc_sparse_checkpoint = sparse_checkpoint;
  BOOST_TEST(xbt_pagesize == getpagesize());
  BOOST_TEST(1 << xbt_pagebits == xbt_pagesize);

  std::unique_ptr<simgrid::mc::RemoteClient> process(new simgrid::mc::RemoteClient(getpid(), -1));
  process->init();
  mc_model_checker = new ::simgrid::mc::ModelChecker(std::move(process));

  for(int n=1; n!=256; ++n) {

    // Store region page(s):
    size_t byte_size = n * xbt_pagesize;
    void* source = mmap(nullptr, byte_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    BOOST_TEST(source!=MAP_FAILED, "Could not allocate source memory");

    // Init memory and take snapshots:
    init_memory(source, byte_size);
    simgrid::mc::RegionSnapshot region0 = simgrid::mc::sparse_region(
      simgrid::mc::RegionType::Unknown, source, source, byte_size);
    for(int i=0; i<n; i+=2) {
      init_memory((char*) source + i*xbt_pagesize, xbt_pagesize);
    }
    simgrid::mc::RegionSnapshot region = simgrid::mc::sparse_region(
      simgrid::mc::RegionType::Unknown, source, source, byte_size);

    void* destination = mmap(nullptr, byte_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    BOOST_TEST(source!=MAP_FAILED, "Could not allocate destination memory");

    // xbt_test_add("Reading whole region data for %i page(s)", n);
    const void* read = MC_region_read(&region, destination, source, byte_size);
    BOOST_TEST(not memcmp(source, read, byte_size), "Mismatch in MC_region_read()");

    // xbt_test_add("Reading parts of region data for %i page(s)", n);
    for(int j=0; j!=100; ++j) {
      size_t offset = rand() % byte_size;
      size_t size = rand() % (byte_size - offset);
      const void* read = MC_region_read(&region, destination, (const char*) source+offset, size);
      BOOST_TEST(not memcmp((char*)source + offset, read, size), "Mismatch in MC_region_read()");
    }

    // xbt_test_add("Compare whole region data for %i page(s)", n);

    BOOST_TEST(MC_snapshot_region_memcmp(source, &region0, source, &region, byte_size),
      "Unexpected match in MC_snapshot_region_memcmp() with previous snapshot");

    // xbt_test_add("Compare parts of region data for %i page(s) with itself", n);
    for(int j=0; j!=100; ++j) {
      size_t offset = rand() % byte_size;
      size_t size = rand() % (byte_size - offset);
      BOOST_TEST(
          not MC_snapshot_region_memcmp((char*)source + offset, &region, (char*)source + offset, &region, size),
          "Mismatch in MC_snapshot_region_memcmp()");
    }

    if (n==1) {
      // xbt_test_add("Read pointer for %i page(s)", n);
      memcpy(source, &mc_model_checker, sizeof(void*));
      simgrid::mc::RegionSnapshot region2 = simgrid::mc::sparse_region(
        simgrid::mc::RegionType::Unknown, source, source, byte_size);
      BOOST_TEST(MC_region_read_pointer(&region2, source) == mc_model_checker,
        "Mismtach in MC_region_read_pointer()");
    }

    munmap(destination, byte_size);
    munmap(source, byte_size);
  }

  delete mc_model_checker;
  mc_model_checker = nullptr;

  return 1; // dummy value, for BOOST unit test
}

/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/


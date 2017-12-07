/* Copyright (c) 2008-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** \file compare.cpp Memory snapshooting and comparison                    */

#include <cinttypes>

#include <array>
#include <memory>
#include <set>
#include <utility>
#include <unordered_set>

#include "xbt/dynar.h"
#include "xbt/sysdep.h"
#include <xbt/mmalloc.h>

#include <mc/mc.h>
#include <mc/datatypes.h>

#include "src/internal_config.h"

#include "src/xbt/mmalloc/mmprivate.h"

#if HAVE_SMPI
#include "src/smpi/include/private.hpp"
#endif

#include "src/mc/Frame.hpp"
#include "src/mc/ObjectInformation.hpp"
#include "src/mc/Type.hpp"
#include "src/mc/Variable.hpp"
#include "src/mc/mc_dwarf.hpp"
#include "src/mc/mc_forward.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_smx.hpp"
#include "src/mc/mc_snapshot.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_compare, xbt, "Logging specific to mc_compare in mc");

namespace simgrid {
namespace mc {

struct HeapLocation;
typedef std::array<HeapLocation, 2> HeapLocationPair;
typedef std::set<HeapLocationPair> HeapLocationPairs;
struct HeapArea;
struct ProcessComparisonState;
struct StateComparator;

static int compare_heap_area(
  StateComparator& state,
  int process_index, const void *area1, const void* area2,
  Snapshot* snapshot1, Snapshot* snapshot2,
  HeapLocationPairs* previous, Type* type, int pointer_level);

}
}

using simgrid::mc::remote;

/*********************************** Heap comparison ***********************************/
/***************************************************************************************/

namespace simgrid {
namespace mc {

class HeapLocation {
public:
  int block_    = 0;
  int fragment_ = 0;

  HeapLocation() = default;
  HeapLocation(int block, int fragment = 0) : block_(block), fragment_(fragment) {}

  bool operator==(HeapLocation const& that) const
  {
    return block_ == that.block_ && fragment_ == that.fragment_;
  }
  bool operator<(HeapLocation const& that) const
  {
    return std::make_pair(block_, fragment_) < std::make_pair(that.block_, that.fragment_);
  }
};

static inline
HeapLocationPair makeHeapLocationPair(int block1, int fragment1, int block2, int fragment2)
{
  return simgrid::mc::HeapLocationPair{{
    simgrid::mc::HeapLocation(block1, fragment1),
    simgrid::mc::HeapLocation(block2, fragment2)
  }};
}

class HeapArea : public HeapLocation {
public:
  bool valid_ = false;
  HeapArea() = default;
  explicit HeapArea(int block) : valid_(true) { block_ = block; }
  HeapArea(int block, int fragment) : valid_(true)
  {
    block_    = block;
    fragment_ = fragment;
  }
};

class ProcessComparisonState {
public:
  std::vector<simgrid::mc::IgnoredHeapRegion>* to_ignore = nullptr;
  std::vector<HeapArea> equals_to;
  std::vector<simgrid::mc::Type*> types;
  std::size_t heapsize = 0;

  void initHeapInformation(xbt_mheap_t heap, std::vector<simgrid::mc::IgnoredHeapRegion>* i);
};

namespace {

/** A hash which works with more stuff
 *
 *  It can hash pairs: the standard hash currently doesn't include this.
 */
template <class X> class hash : public std::hash<X> {
};

template <class X, class Y> class hash<std::pair<X, Y>> {
public:
  std::size_t operator()(std::pair<X,Y>const& x) const
  {
    hash<X> h1;
    hash<X> h2;
    return h1(x.first) ^ h2(x.second);
  }
};

}

class StateComparator {
public:
  s_xbt_mheap_t std_heap_copy;
  std::size_t heaplimit;
  std::array<ProcessComparisonState, 2> processStates;

  std::unordered_set<std::pair<void*, void*>, hash<std::pair<void*, void*>>> compared_pointers;

  void clear()
  {
    compared_pointers.clear();
  }

  int initHeapInformation(
    xbt_mheap_t heap1, xbt_mheap_t heap2,
    std::vector<simgrid::mc::IgnoredHeapRegion>* i1,
    std::vector<simgrid::mc::IgnoredHeapRegion>* i2);

  HeapArea& equals_to1_(std::size_t i, std::size_t j)
  {
    return processStates[0].equals_to[ MAX_FRAGMENT_PER_BLOCK * i + j];
  }
  HeapArea& equals_to2_(std::size_t i, std::size_t j)
  {
    return processStates[1].equals_to[ MAX_FRAGMENT_PER_BLOCK * i + j];
  }
  Type*& types1_(std::size_t i, std::size_t j)
  {
    return processStates[0].types[ MAX_FRAGMENT_PER_BLOCK * i + j];
  }
  Type*& types2_(std::size_t i, std::size_t j)
  {
    return processStates[1].types[ MAX_FRAGMENT_PER_BLOCK * i + j];
  }

  HeapArea const& equals_to1_(std::size_t i, std::size_t j) const
  {
    return processStates[0].equals_to[ MAX_FRAGMENT_PER_BLOCK * i + j];
  }
  HeapArea const& equals_to2_(std::size_t i, std::size_t j) const
  {
    return processStates[1].equals_to[ MAX_FRAGMENT_PER_BLOCK * i + j];
  }
  Type* const& types1_(std::size_t i, std::size_t j) const
  {
    return processStates[0].types[ MAX_FRAGMENT_PER_BLOCK * i + j];
  }
  Type* const& types2_(std::size_t i, std::size_t j) const
  {
    return processStates[1].types[ MAX_FRAGMENT_PER_BLOCK * i + j];
  }

  /** Check whether two blocks are known to be matching
   *
   *  @param b1     Block of state 1
   *  @param b2     Block of state 2
   *  @return       if the blocks are known to be matching
   */
  bool blocksEqual(int b1, int b2) const
  {
    return this->equals_to1_(b1, 0).block_ == b2 && this->equals_to2_(b2, 0).block_ == b1;
  }

  /** Check whether two fragments are known to be matching
   *
   *  @param b1     Block of state 1
   *  @param f1     Fragment of state 1
   *  @param b2     Block of state 2
   *  @param f2     Fragment of state 2
   *  @return       if the fragments are known to be matching
   */
  int fragmentsEqual(int b1, int f1, int b2, int f2) const
  {
    return this->equals_to1_(b1, f1).block_ == b2 && this->equals_to1_(b1, f1).fragment_ == f2 &&
           this->equals_to2_(b2, f2).block_ == b1 && this->equals_to2_(b2, f2).fragment_ == f1;
  }

  void match_equals(HeapLocationPairs* list);
};

}
}

/************************************************************************************/

static ssize_t heap_comparison_ignore_size(
  std::vector<simgrid::mc::IgnoredHeapRegion>* ignore_list,
  const void *address)
{
  int start = 0;
  int end = ignore_list->size() - 1;

  while (start <= end) {
    unsigned int cursor = (start + end) / 2;
    simgrid::mc::IgnoredHeapRegion const& region = (*ignore_list)[cursor];
    if (region.address == address)
      return region.size;
    if (region.address < address)
      start = cursor + 1;
    if (region.address > address)
      end = cursor - 1;
  }

  return -1;
}

static bool is_stack(const void *address)
{
  for (auto const& stack : mc_model_checker->process().stack_areas())
    if (address == stack.address)
      return true;
  return false;
}

// TODO, this should depend on the snapshot?
static bool is_block_stack(int block)
{
  for (auto const& stack : mc_model_checker->process().stack_areas())
    if (block == stack.block)
      return true;
  return false;
}

namespace simgrid {
namespace mc {

void StateComparator::match_equals(HeapLocationPairs* list)
{
  for (auto const& pair : *list) {
    if (pair[0].fragment_ != -1) {
      this->equals_to1_(pair[0].block_, pair[0].fragment_) = simgrid::mc::HeapArea(pair[1].block_, pair[1].fragment_);
      this->equals_to2_(pair[1].block_, pair[1].fragment_) = simgrid::mc::HeapArea(pair[0].block_, pair[0].fragment_);
    } else {
      this->equals_to1_(pair[0].block_, 0) = simgrid::mc::HeapArea(pair[1].block_, pair[1].fragment_);
      this->equals_to2_(pair[1].block_, 0) = simgrid::mc::HeapArea(pair[0].block_, pair[0].fragment_);
    }
  }
}

void ProcessComparisonState::initHeapInformation(xbt_mheap_t heap,
                        std::vector<simgrid::mc::IgnoredHeapRegion>* i)
{
  auto heaplimit  = heap->heaplimit;
  this->heapsize  = heap->heapsize;
  this->to_ignore = i;
  this->equals_to.assign(heaplimit * MAX_FRAGMENT_PER_BLOCK, HeapArea());
  this->types.assign(heaplimit * MAX_FRAGMENT_PER_BLOCK, nullptr);
}

int StateComparator::initHeapInformation(xbt_mheap_t heap1, xbt_mheap_t heap2,
                          std::vector<simgrid::mc::IgnoredHeapRegion>* i1,
                          std::vector<simgrid::mc::IgnoredHeapRegion>* i2)
{
  if ((heap1->heaplimit != heap2->heaplimit) || (heap1->heapsize != heap2->heapsize))
    return -1;
  this->heaplimit     = heap1->heaplimit;
  this->std_heap_copy = *mc_model_checker->process().get_heap();
  this->processStates[0].initHeapInformation(heap1, i1);
  this->processStates[1].initHeapInformation(heap2, i2);
  return 0;
}

// TODO, have a robust way to find it in O(1)
static inline
mc_mem_region_t MC_get_heap_region(simgrid::mc::Snapshot* snapshot)
{
  for (auto const& region : snapshot->snapshot_regions)
    if (region->region_type() == simgrid::mc::RegionType::Heap)
      return region.get();
  xbt_die("No heap region");
}

static
int mmalloc_compare_heap(
  simgrid::mc::StateComparator& state, simgrid::mc::Snapshot* snapshot1, simgrid::mc::Snapshot* snapshot2)
{
  simgrid::mc::RemoteClient* process = &mc_model_checker->process();

  /* Start comparison */
  size_t i1;
  size_t i2;
  size_t j1;
  size_t j2;
  size_t k;
  void* addr_block1;
  void* addr_block2;
  void* addr_frag1;
  void* addr_frag2;
  int nb_diff1 = 0;
  int nb_diff2 = 0;
  int equal;

  /* Check busy blocks */
  i1 = 1;

  malloc_info heapinfo_temp1;
  malloc_info heapinfo_temp2;
  malloc_info heapinfo_temp2b;

  mc_mem_region_t heap_region1 = MC_get_heap_region(snapshot1);
  mc_mem_region_t heap_region2 = MC_get_heap_region(snapshot2);

  // This is the address of std_heap->heapinfo in the application process:
  void* heapinfo_address = &((xbt_mheap_t) process->heap_address)->heapinfo;

  // This is in snapshot do not use them directly:
  const malloc_info* heapinfos1 = snapshot1->read<malloc_info*>(
      RemotePtr<malloc_info*>((std::uint64_t)heapinfo_address), simgrid::mc::ProcessIndexMissing);
  const malloc_info* heapinfos2 = snapshot2->read<malloc_info*>(
      RemotePtr<malloc_info*>((std::uint64_t)heapinfo_address), simgrid::mc::ProcessIndexMissing);

  while (i1 < state.heaplimit) {

    const malloc_info* heapinfo1 = (const malloc_info*) MC_region_read(heap_region1, &heapinfo_temp1, &heapinfos1[i1], sizeof(malloc_info));
    const malloc_info* heapinfo2 = (const malloc_info*) MC_region_read(heap_region2, &heapinfo_temp2, &heapinfos2[i1], sizeof(malloc_info));

    if (heapinfo1->type == MMALLOC_TYPE_FREE || heapinfo1->type == MMALLOC_TYPE_HEAPINFO) {      /* Free block */
      i1 ++;
      continue;
    }

    if (heapinfo1->type < 0) {
      fprintf(stderr, "Unkown mmalloc block type.\n");
      abort();
    }

    addr_block1 = ((void*)(((ADDR2UINT(i1)) - 1) * BLOCKSIZE + (char*)state.std_heap_copy.heapbase));

    if (heapinfo1->type == MMALLOC_TYPE_UNFRAGMENTED) {       /* Large block */

      if (is_stack(addr_block1)) {
        for (k = 0; k < heapinfo1->busy_block.size; k++)
          state.equals_to1_(i1 + k, 0) = HeapArea(i1, -1);
        for (k = 0; k < heapinfo2->busy_block.size; k++)
          state.equals_to2_(i1 + k, 0) = HeapArea(i1, -1);
        i1 += heapinfo1->busy_block.size;
        continue;
      }

      if (state.equals_to1_(i1, 0).valid_) {
        i1++;
        continue;
      }

      i2          = 1;
      equal       = 0;

      /* Try first to associate to same block in the other heap */
      if (heapinfo2->type == heapinfo1->type && state.equals_to2_(i1, 0).valid_ == 0) {
        addr_block2 = (ADDR2UINT(i1) - 1) * BLOCKSIZE + (char*)state.std_heap_copy.heapbase;
        int res_compare = compare_heap_area(state, simgrid::mc::ProcessIndexMissing, addr_block1, addr_block2,
                                            snapshot1, snapshot2, nullptr, nullptr, 0);
        if (res_compare != 1) {
          for (k = 1; k < heapinfo2->busy_block.size; k++)
            state.equals_to2_(i1 + k, 0) = HeapArea(i1, -1);
          for (k = 1; k < heapinfo1->busy_block.size; k++)
            state.equals_to1_(i1 + k, 0) = HeapArea(i1, -1);
          equal = 1;
          i1 += heapinfo1->busy_block.size;
        }
      }

      while (i2 < state.heaplimit && not equal) {

        addr_block2 = (ADDR2UINT(i2) - 1) * BLOCKSIZE + (char*)state.std_heap_copy.heapbase;

        if (i2 == i1) {
          i2++;
          continue;
        }

        const malloc_info* heapinfo2b = (const malloc_info*) MC_region_read(heap_region2, &heapinfo_temp2b, &heapinfos2[i2], sizeof(malloc_info));

        if (heapinfo2b->type != MMALLOC_TYPE_UNFRAGMENTED) {
          i2++;
          continue;
        }

        if (state.equals_to2_(i2, 0).valid_) {
          i2++;
          continue;
        }

        int res_compare = compare_heap_area(state, simgrid::mc::ProcessIndexMissing, addr_block1, addr_block2,
                                            snapshot1, snapshot2, nullptr, nullptr, 0);

        if (res_compare != 1) {
          for (k = 1; k < heapinfo2b->busy_block.size; k++)
            state.equals_to2_(i2 + k, 0) = HeapArea(i1, -1);
          for (k = 1; k < heapinfo1->busy_block.size; k++)
            state.equals_to1_(i1 + k, 0) = HeapArea(i2, -1);
          equal = 1;
          i1 += heapinfo1->busy_block.size;
        }

        i2++;
      }

      if (not equal) {
        XBT_DEBUG("Block %zu not found (size_used = %zu, addr = %p)", i1, heapinfo1->busy_block.busy_size, addr_block1);
        i1 = state.heaplimit + 1;
        nb_diff1++;
      }

    } else {                    /* Fragmented block */

      for (j1 = 0; j1 < (size_t) (BLOCKSIZE >> heapinfo1->type); j1++) {

        if (heapinfo1->busy_frag.frag_size[j1] == -1) /* Free fragment_ */
          continue;

        if (state.equals_to1_(i1, j1).valid_)
          continue;

        addr_frag1 = (void*)((char*)addr_block1 + (j1 << heapinfo1->type));

        i2 = 1;
        equal = 0;

        /* Try first to associate to same fragment_ in the other heap */
        if (heapinfo2->type == heapinfo1->type && not state.equals_to2_(i1, j1).valid_) {
          addr_block2 = (ADDR2UINT(i1) - 1) * BLOCKSIZE +
                         (char *) state.std_heap_copy.heapbase;
          addr_frag2 =
              (void *) ((char *) addr_block2 +
                        (j1 << heapinfo2->type));
          int res_compare = compare_heap_area(state, simgrid::mc::ProcessIndexMissing, addr_frag1, addr_frag2,
                                              snapshot1, snapshot2, nullptr, nullptr, 0);
          if (res_compare != 1)
            equal = 1;
        }

        while (i2 < state.heaplimit && not equal) {

          const malloc_info* heapinfo2b = (const malloc_info*) MC_region_read(
            heap_region2, &heapinfo_temp2b, &heapinfos2[i2],
            sizeof(malloc_info));

          if (heapinfo2b->type == MMALLOC_TYPE_FREE || heapinfo2b->type == MMALLOC_TYPE_HEAPINFO) {
            i2 ++;
            continue;
          }

          // We currently do not match fragments with unfragmented blocks (maybe we should).
          if (heapinfo2b->type == MMALLOC_TYPE_UNFRAGMENTED) {
            i2++;
            continue;
          }

          if (heapinfo2b->type < 0) {
            fprintf(stderr, "Unknown mmalloc block type.\n");
            abort();
          }

          for (j2 = 0; j2 < (size_t) (BLOCKSIZE >> heapinfo2b->type);
               j2++) {

            if (i2 == i1 && j2 == j1)
              continue;

            if (state.equals_to2_(i2, j2).valid_)
              continue;

            addr_block2 = (ADDR2UINT(i2) - 1) * BLOCKSIZE + (char*)state.std_heap_copy.heapbase;
            addr_frag2  = (void*)((char*)addr_block2 + (j2 << heapinfo2b->type));

            int res_compare = compare_heap_area(state, simgrid::mc::ProcessIndexMissing, addr_frag1, addr_frag2,
                                                snapshot2, snapshot2, nullptr, nullptr, 0);
            if (res_compare != 1) {
              equal = 1;
              break;
            }
          }

          i2++;
        }

        if (not equal) {
          XBT_DEBUG("Block %zu, fragment_ %zu not found (size_used = %zd, address = %p)\n", i1, j1,
                    heapinfo1->busy_frag.frag_size[j1], addr_frag1);
          i1 = state.heaplimit + 1;
          nb_diff1++;
          break;
        }
      }

      i1++;
    }
  }

  /* All blocks/fragments are equal to another block/fragment_ ? */
  size_t i = 1;
  size_t j = 0;

  for(i = 1; i < state.heaplimit; i++) {
    const malloc_info* heapinfo1 = (const malloc_info*) MC_region_read(
      heap_region1, &heapinfo_temp1, &heapinfos1[i], sizeof(malloc_info));

    if (heapinfo1->type == MMALLOC_TYPE_UNFRAGMENTED && i1 == state.heaplimit && heapinfo1->busy_block.busy_size > 0 &&
        not state.equals_to1_(i, 0).valid_) {
      XBT_DEBUG("Block %zu not found (size used = %zu)", i, heapinfo1->busy_block.busy_size);
      nb_diff1++;
    }

    if (heapinfo1->type <= 0)
      continue;
    for (j = 0; j < (size_t) (BLOCKSIZE >> heapinfo1->type); j++)
      if (i1 == state.heaplimit && heapinfo1->busy_frag.frag_size[j] > 0 && not state.equals_to1_(i, j).valid_) {
        XBT_DEBUG("Block %zu, Fragment %zu not found (size used = %zd)", i, j, heapinfo1->busy_frag.frag_size[j]);
        nb_diff1++;
      }
  }

  if (i1 == state.heaplimit)
    XBT_DEBUG("Number of blocks/fragments not found in heap1: %d", nb_diff1);

  for (i=1; i < state.heaplimit; i++) {
    const malloc_info* heapinfo2 = (const malloc_info*) MC_region_read(
      heap_region2, &heapinfo_temp2, &heapinfos2[i], sizeof(malloc_info));
    if (heapinfo2->type == MMALLOC_TYPE_UNFRAGMENTED && i1 == state.heaplimit && heapinfo2->busy_block.busy_size > 0 &&
        not state.equals_to2_(i, 0).valid_) {
      XBT_DEBUG("Block %zu not found (size used = %zu)", i,
                heapinfo2->busy_block.busy_size);
      nb_diff2++;
    }

    if (heapinfo2->type <= 0)
      continue;

    for (j = 0; j < (size_t) (BLOCKSIZE >> heapinfo2->type); j++)
      if (i1 == state.heaplimit && heapinfo2->busy_frag.frag_size[j] > 0 && not state.equals_to2_(i, j).valid_) {
        XBT_DEBUG("Block %zu, Fragment %zu not found (size used = %zd)",
          i, j, heapinfo2->busy_frag.frag_size[j]);
        nb_diff2++;
      }

  }

  if (i1 == state.heaplimit)
    XBT_DEBUG("Number of blocks/fragments not found in heap2: %d", nb_diff2);

  return nb_diff1 > 0 || nb_diff2 > 0;
}

/**
 *
 * @param state
 * @param real_area1     Process address for state 1
 * @param real_area2     Process address for state 2
 * @param snapshot1      Snapshot of state 1
 * @param snapshot2      Snapshot of state 2
 * @param previous
 * @param size
 * @param check_ignore
 */
static int compare_heap_area_without_type(
  simgrid::mc::StateComparator& state, int process_index,
  const void *real_area1, const void *real_area2,
  simgrid::mc::Snapshot* snapshot1,
  simgrid::mc::Snapshot* snapshot2,
  HeapLocationPairs* previous, int size,
  int check_ignore)
{
  simgrid::mc::RemoteClient* process = &mc_model_checker->process();
  mc_mem_region_t heap_region1 = MC_get_heap_region(snapshot1);
  mc_mem_region_t heap_region2 = MC_get_heap_region(snapshot2);

  for (int i = 0; i < size; ) {

    if (check_ignore > 0) {
      ssize_t ignore1 = heap_comparison_ignore_size(
        state.processStates[0].to_ignore, (char *) real_area1 + i);
      if (ignore1 != -1) {
        ssize_t ignore2 = heap_comparison_ignore_size(
          state.processStates[1].to_ignore, (char *) real_area2 + i);
        if (ignore2 == ignore1) {
          if (ignore1 == 0) {
            check_ignore--;
            return 0;
          } else {
            i = i + ignore2;
            check_ignore--;
            continue;
          }
        }
      }
    }

    if (MC_snapshot_region_memcmp(((char *) real_area1) + i, heap_region1, ((char *) real_area2) + i, heap_region2, 1) != 0) {

      int pointer_align = (i / sizeof(void *)) * sizeof(void *);
      const void* addr_pointed1 = snapshot1->read(
        remote((void**)((char *) real_area1 + pointer_align)), process_index);
      const void* addr_pointed2 = snapshot2->read(
        remote((void**)((char *) real_area2 + pointer_align)), process_index);

      if (process->in_maestro_stack(remote(addr_pointed1))
        && process->in_maestro_stack(remote(addr_pointed2))) {
        i = pointer_align + sizeof(void *);
        continue;
      }

      if (addr_pointed1 > state.std_heap_copy.heapbase
           && addr_pointed1 < mc_snapshot_get_heap_end(snapshot1)
           && addr_pointed2 > state.std_heap_copy.heapbase
           && addr_pointed2 < mc_snapshot_get_heap_end(snapshot2)) {
        // Both addreses are in the heap:
        int res_compare = compare_heap_area(state ,process_index,
          addr_pointed1, addr_pointed2,
          snapshot1, snapshot2, previous, nullptr, 0);
        if (res_compare == 1)
          return res_compare;
        i = pointer_align + sizeof(void *);
        continue;
      }

      return 1;
    }

    i++;
  }

  return 0;
}

/**
 *
 * @param state
 * @param real_area1     Process address for state 1
 * @param real_area2     Process address for state 2
 * @param snapshot1      Snapshot of state 1
 * @param snapshot2      Snapshot of state 2
 * @param previous
 * @param type
 * @param area_size      either a byte_size or an elements_count (?)
 * @param check_ignore
 * @param pointer_level
 * @return               0 (same), 1 (different), -1 (unknown)
 */
static int compare_heap_area_with_type(
  simgrid::mc::StateComparator& state, int process_index,
  const void *real_area1, const void *real_area2,
  simgrid::mc::Snapshot* snapshot1,
  simgrid::mc::Snapshot* snapshot2,
  HeapLocationPairs* previous, simgrid::mc::Type* type,
  int area_size, int check_ignore,
  int pointer_level)
{
  do {

    // HACK: This should not happen but in pratice, there are some
    // DW_TAG_typedef without an associated DW_AT_type:
    //<1><538832>: Abbrev Number: 111 (DW_TAG_typedef)
    //    <538833>   DW_AT_name        : (indirect string, offset: 0x2292f3): gregset_t
    //    <538837>   DW_AT_decl_file   : 98
    //    <538838>   DW_AT_decl_line   : 37
    if (type == nullptr)
      return 0;

    if (is_stack(real_area1) && is_stack(real_area2))
      return 0;

    if (check_ignore > 0) {
      ssize_t ignore1 = heap_comparison_ignore_size(state.processStates[0].to_ignore, real_area1);
      if (ignore1 > 0 && heap_comparison_ignore_size(state.processStates[1].to_ignore, real_area2) == ignore1)
        return 0;
    }

    simgrid::mc::Type* subtype;
    simgrid::mc::Type* subsubtype;
    int res;
    int elm_size;
    const void* addr_pointed1;
    const void* addr_pointed2;

    mc_mem_region_t heap_region1 = MC_get_heap_region(snapshot1);
    mc_mem_region_t heap_region2 = MC_get_heap_region(snapshot2);

    switch (type->type) {
      case DW_TAG_unspecified_type:
        return 1;

      case DW_TAG_base_type:
        if (not type->name.empty() && type->name == "char") { /* String, hence random (arbitrary ?) size */
          if (real_area1 == real_area2)
            return -1;
          else
            return MC_snapshot_region_memcmp(real_area1, heap_region1, real_area2, heap_region2, area_size) != 0;
        } else {
          if (area_size != -1 && type->byte_size != area_size)
            return -1;
          else
            return MC_snapshot_region_memcmp(real_area1, heap_region1, real_area2, heap_region2, type->byte_size) != 0;
        }
        break;

      case DW_TAG_enumeration_type:
        if (area_size != -1 && type->byte_size != area_size)
          return -1;
        return MC_snapshot_region_memcmp(real_area1, heap_region1, real_area2, heap_region2, type->byte_size) != 0;

      case DW_TAG_typedef:
      case DW_TAG_const_type:
      case DW_TAG_volatile_type:
        // Poor man's TCO:
        type = type->subtype;
        continue; // restart

      case DW_TAG_array_type:
        subtype = type->subtype;
        switch (subtype->type) {
          case DW_TAG_unspecified_type:
            return 1;

          case DW_TAG_base_type:
          case DW_TAG_enumeration_type:
          case DW_TAG_pointer_type:
          case DW_TAG_reference_type:
          case DW_TAG_rvalue_reference_type:
          case DW_TAG_structure_type:
          case DW_TAG_class_type:
          case DW_TAG_union_type:
            if (subtype->full_type)
              subtype = subtype->full_type;
            elm_size  = subtype->byte_size;
            break;
          // TODO, just remove the type indirection?
          case DW_TAG_const_type:
          case DW_TAG_typedef:
          case DW_TAG_volatile_type:
            subsubtype = subtype->subtype;
            if (subsubtype->full_type)
              subsubtype = subsubtype->full_type;
            elm_size     = subsubtype->byte_size;
            break;
          default:
            return 0;
            break;
        }
        for (int i = 0; i < type->element_count; i++) {
          // TODO, add support for variable stride (DW_AT_byte_stride)
          res = compare_heap_area_with_type(state, process_index, (char*)real_area1 + (i * elm_size),
                                            (char*)real_area2 + (i * elm_size), snapshot1, snapshot2, previous,
                                            type->subtype, subtype->byte_size, check_ignore, pointer_level);
          if (res == 1)
            return res;
        }
        return 0;

      case DW_TAG_reference_type:
      case DW_TAG_rvalue_reference_type:
      case DW_TAG_pointer_type:
        if (type->subtype && type->subtype->type == DW_TAG_subroutine_type) {
          addr_pointed1 = snapshot1->read(remote((void**)real_area1), process_index);
          addr_pointed2 = snapshot2->read(remote((void**)real_area2), process_index);
          return (addr_pointed1 != addr_pointed2);
        }
        pointer_level++;
        if (pointer_level <= 1) {
          addr_pointed1 = snapshot1->read(remote((void**)real_area1), process_index);
          addr_pointed2 = snapshot2->read(remote((void**)real_area2), process_index);
          if (addr_pointed1 > state.std_heap_copy.heapbase && addr_pointed1 < mc_snapshot_get_heap_end(snapshot1) &&
              addr_pointed2 > state.std_heap_copy.heapbase && addr_pointed2 < mc_snapshot_get_heap_end(snapshot2))
            return compare_heap_area(state, process_index, addr_pointed1, addr_pointed2, snapshot1, snapshot2, previous,
                                     type->subtype, pointer_level);
          else
            return (addr_pointed1 != addr_pointed2);
        }
        for (size_t i = 0; i < (area_size / sizeof(void*)); i++) {
          addr_pointed1 = snapshot1->read(remote((void**)((char*)real_area1 + i * sizeof(void*))), process_index);
          addr_pointed2 = snapshot2->read(remote((void**)((char*)real_area2 + i * sizeof(void*))), process_index);
          if (addr_pointed1 > state.std_heap_copy.heapbase && addr_pointed1 < mc_snapshot_get_heap_end(snapshot1) &&
              addr_pointed2 > state.std_heap_copy.heapbase && addr_pointed2 < mc_snapshot_get_heap_end(snapshot2))
            res = compare_heap_area(state, process_index, addr_pointed1, addr_pointed2, snapshot1, snapshot2, previous,
                                    type->subtype, pointer_level);
          else
            res = (addr_pointed1 != addr_pointed2);
          if (res == 1)
            return res;
        }
        return 0;

      case DW_TAG_structure_type:
      case DW_TAG_class_type:
        if (type->full_type)
          type = type->full_type;
        if (area_size != -1 && type->byte_size != area_size) {
          if (area_size <= type->byte_size || area_size % type->byte_size != 0)
            return -1;
          for (size_t i = 0; i < (size_t)(area_size / type->byte_size); i++) {
            int res = compare_heap_area_with_type(state, process_index, (char*)real_area1 + i * type->byte_size,
                                                  (char*)real_area2 + i * type->byte_size, snapshot1, snapshot2,
                                                  previous, type, -1, check_ignore, 0);
            if (res == 1)
              return res;
          }
        } else {
          for (simgrid::mc::Member& member : type->members) {
            // TODO, optimize this? (for the offset case)
            void* real_member1 = simgrid::dwarf::resolve_member(real_area1, type, &member,
                                                                (simgrid::mc::AddressSpace*)snapshot1, process_index);
            void* real_member2 = simgrid::dwarf::resolve_member(real_area2, type, &member,
                                                                (simgrid::mc::AddressSpace*)snapshot2, process_index);
            int res = compare_heap_area_with_type(state, process_index, real_member1, real_member2, snapshot1,
                                                  snapshot2, previous, member.type, -1, check_ignore, 0);
            if (res == 1)
              return res;
          }
        }
        return 0;

      case DW_TAG_union_type:
        return compare_heap_area_without_type(state, process_index, real_area1, real_area2, snapshot1, snapshot2,
                                              previous, type->byte_size, check_ignore);

      default:
        return 0;
    }

    xbt_die("Unreachable");
  } while (true);
}

/** Infer the type of a part of the block from the type of the block
 *
 * TODO, handle DW_TAG_array_type as well as arrays of the object ((*p)[5], p[5])
 *
 * TODO, handle subfields ((*p).bar.foo, (*p)[5].bar…)
 *
 * @param  type               DWARF type ID of the root address
 * @param  area_size
 * @return                    DWARF type ID for given offset
 */
static simgrid::mc::Type* get_offset_type(void *real_base_address, simgrid::mc::Type* type,
                                 int offset, int area_size,
                                 simgrid::mc::Snapshot* snapshot, int process_index)
{

  // Beginning of the block, the infered variable type if the type of the block:
  if (offset == 0)
    return type;

  switch (type->type) {

  case DW_TAG_structure_type:
  case DW_TAG_class_type:
    if (type->full_type)
      type = type->full_type;
    if (area_size != -1 && type->byte_size != area_size) {
      if (area_size > type->byte_size && area_size % type->byte_size == 0)
        return type;
      else
        return nullptr;
    }

    for (simgrid::mc::Member& member : type->members) {
      if (member.has_offset_location()) {
        // We have the offset, use it directly (shortcut):
        if (member.offset() == offset)
          return member.type;
      } else {
        void* real_member = simgrid::dwarf::resolve_member(real_base_address, type, &member, snapshot, process_index);
        if ((char*)real_member - (char*)real_base_address == offset)
          return member.type;
      }
    }
    return nullptr;

  default:
    /* FIXME: other cases ? */
    return nullptr;

  }
}

/**
 *
 * @param area1          Process address for state 1
 * @param area2          Process address for state 2
 * @param snapshot1      Snapshot of state 1
 * @param snapshot2      Snapshot of state 2
 * @param previous       Pairs of blocks already compared on the current path (or nullptr)
 * @param type_id        Type of variable
 * @param pointer_level
 * @return 0 (same), 1 (different), -1
 */
static
int compare_heap_area(simgrid::mc::StateComparator& state, int process_index,
                      const void *area1, const void *area2,
                      simgrid::mc::Snapshot* snapshot1,
                      simgrid::mc::Snapshot* snapshot2,
                      HeapLocationPairs* previous,
                      simgrid::mc::Type* type, int pointer_level)
{
  simgrid::mc::RemoteClient* process = &mc_model_checker->process();

  ssize_t block1;
  ssize_t block2;
  ssize_t size;
  int check_ignore = 0;

  int type_size = -1;
  int offset1   = 0;
  int offset2   = 0;
  int new_size1 = -1;
  int new_size2 = -1;

  simgrid::mc::Type* new_type1 = nullptr;
  simgrid::mc::Type* new_type2 = nullptr;

  bool match_pairs = false;

  // This is the address of std_heap->heapinfo in the application process:
  void* heapinfo_address = &((xbt_mheap_t) process->heap_address)->heapinfo;

  const malloc_info* heapinfos1 = snapshot1->read(remote((const malloc_info**)heapinfo_address), process_index);
  const malloc_info* heapinfos2 = snapshot2->read(remote((const malloc_info**)heapinfo_address), process_index);

  malloc_info heapinfo_temp1;
  malloc_info heapinfo_temp2;

  simgrid::mc::HeapLocationPairs current;
  if (previous == nullptr) {
    previous = &current;
    match_pairs = true;
  }

  // Get block number:
  block1 = ((char*)area1 - (char*)state.std_heap_copy.heapbase) / BLOCKSIZE + 1;
  block2 = ((char*)area2 - (char*)state.std_heap_copy.heapbase) / BLOCKSIZE + 1;

  // If either block is a stack block:
  if (is_block_stack((int) block1) && is_block_stack((int) block2)) {
    previous->insert(simgrid::mc::makeHeapLocationPair(block1, -1, block2, -1));
    if (match_pairs)
      state.match_equals(previous);
    return 0;
  }

  // If either block is not in the expected area of memory:
  if (((char*)area1 < (char*)state.std_heap_copy.heapbase) || (block1 > (ssize_t)state.processStates[0].heapsize) ||
      (block1 < 1) || ((char*)area2 < (char*)state.std_heap_copy.heapbase) ||
      (block2 > (ssize_t)state.processStates[1].heapsize) || (block2 < 1)) {
    return 1;
  }

  // Process address of the block:
  void* real_addr_block1 = (ADDR2UINT(block1) - 1) * BLOCKSIZE + (char*)state.std_heap_copy.heapbase;
  void* real_addr_block2 = (ADDR2UINT(block2) - 1) * BLOCKSIZE + (char*)state.std_heap_copy.heapbase;

  if (type) {
    if (type->full_type)
      type = type->full_type;

    // This assume that for "boring" types (volatile ...) byte_size is absent:
    while (type->byte_size == 0 && type->subtype != nullptr)
      type = type->subtype;

    // Find type_size:
    if (type->type == DW_TAG_pointer_type ||
        (type->type == DW_TAG_base_type && not type->name.empty() && type->name == "char"))
      type_size = -1;
    else
      type_size = type->byte_size;

  }

  mc_mem_region_t heap_region1 = MC_get_heap_region(snapshot1);
  mc_mem_region_t heap_region2 = MC_get_heap_region(snapshot2);

  const malloc_info* heapinfo1 = (const malloc_info*) MC_region_read(
    heap_region1, &heapinfo_temp1, &heapinfos1[block1], sizeof(malloc_info));
  const malloc_info* heapinfo2 = (const malloc_info*) MC_region_read(
    heap_region2, &heapinfo_temp2, &heapinfos2[block2], sizeof(malloc_info));

  if ((heapinfo1->type == MMALLOC_TYPE_FREE || heapinfo1->type==MMALLOC_TYPE_HEAPINFO)
    && (heapinfo2->type == MMALLOC_TYPE_FREE || heapinfo2->type ==MMALLOC_TYPE_HEAPINFO)) {
    /* Free block */
    if (match_pairs)
      state.match_equals(previous);
    return 0;
  }

  if (heapinfo1->type == MMALLOC_TYPE_UNFRAGMENTED && heapinfo2->type == MMALLOC_TYPE_UNFRAGMENTED) {
    /* Complete block */

    // TODO, lookup variable type from block type as done for fragmented blocks

    if (state.equals_to1_(block1, 0).valid_ && state.equals_to2_(block2, 0).valid_ &&
        state.blocksEqual(block1, block2)) {
      if (match_pairs)
        state.match_equals(previous);
      return 0;
    }

    if (type_size != -1 && type_size != (ssize_t)heapinfo1->busy_block.busy_size &&
        type_size != (ssize_t)heapinfo2->busy_block.busy_size &&
        (type->name.empty() || type->name == "struct s_smx_context")) {
      if (match_pairs)
        state.match_equals(previous);
      return -1;
    }

    if (heapinfo1->busy_block.size != heapinfo2->busy_block.size)
      return 1;
    if (heapinfo1->busy_block.busy_size != heapinfo2->busy_block.busy_size)
      return 1;

    if (not previous->insert(simgrid::mc::makeHeapLocationPair(block1, -1, block2, -1)).second) {
      if (match_pairs)
        state.match_equals(previous);
      return 0;
    }

    size = heapinfo1->busy_block.busy_size;

    // Remember (basic) type inference.
    // The current data structure only allows us to do this for the whole block.
    if (type != nullptr && area1 == real_addr_block1)
      state.types1_(block1, 0) = type;
    if (type != nullptr && area2 == real_addr_block2)
      state.types2_(block2, 0) = type;

    if (size <= 0) {
      if (match_pairs)
        state.match_equals(previous);
      return 0;
    }

    if (heapinfo1->busy_block.ignore > 0
        && heapinfo2->busy_block.ignore == heapinfo1->busy_block.ignore)
      check_ignore = heapinfo1->busy_block.ignore;

  } else if ((heapinfo1->type > 0) && (heapinfo2->type > 0)) {      /* Fragmented block */

    // Fragment number:
    ssize_t frag1 = ((uintptr_t)(ADDR2UINT(area1) % (BLOCKSIZE))) >> heapinfo1->type;
    ssize_t frag2 = ((uintptr_t)(ADDR2UINT(area2) % (BLOCKSIZE))) >> heapinfo2->type;

    // Process address of the fragment_:
    void* real_addr_frag1 = (void*)((char*)real_addr_block1 + (frag1 << heapinfo1->type));
    void* real_addr_frag2 = (void*)((char*)real_addr_block2 + (frag2 << heapinfo2->type));

    // Check the size of the fragments against the size of the type:
    if (type_size != -1) {
      if (heapinfo1->busy_frag.frag_size[frag1] == -1 || heapinfo2->busy_frag.frag_size[frag2] == -1) {
        if (match_pairs)
          state.match_equals(previous);
        return -1;
      }
      // ?
      if (type_size != heapinfo1->busy_frag.frag_size[frag1]
          || type_size != heapinfo2->busy_frag.frag_size[frag2]) {
        if (match_pairs)
          state.match_equals(previous);
        return -1;
      }
    }

    // Check if the blocks are already matched together:
    if (state.equals_to1_(block1, frag1).valid_ && state.equals_to2_(block2, frag2).valid_ && offset1 == offset2 &&
        state.fragmentsEqual(block1, frag1, block2, frag2)) {
      if (match_pairs)
        state.match_equals(previous);
      return 0;
    }
    // Compare the size of both fragments:
    if (heapinfo1->busy_frag.frag_size[frag1] != heapinfo2->busy_frag.frag_size[frag2]) {
      if (type_size == -1) {
        if (match_pairs)
          state.match_equals(previous);
        return -1;
      } else
        return 1;
    }

    // Size of the fragment_:
    size = heapinfo1->busy_frag.frag_size[frag1];

    // Remember (basic) type inference.
    // The current data structure only allows us to do this for the whole fragment_.
    if (type != nullptr && area1 == real_addr_frag1)
      state.types1_(block1, frag1) = type;
    if (type != nullptr && area2 == real_addr_frag2)
      state.types2_(block2, frag2) = type;

    // The type of the variable is already known:
    if (type) {
      new_type1 = new_type2 = type;
    }
    // Type inference from the block type.
    else if (state.types1_(block1, frag1) != nullptr || state.types2_(block2, frag2) != nullptr) {

      offset1 = (char*)area1 - (char*)real_addr_frag1;
      offset2 = (char*)area2 - (char*)real_addr_frag2;

      if (state.types1_(block1, frag1) != nullptr && state.types2_(block2, frag2) != nullptr) {
        new_type1 =
            get_offset_type(real_addr_frag1, state.types1_(block1, frag1), offset1, size, snapshot1, process_index);
        new_type2 =
            get_offset_type(real_addr_frag2, state.types2_(block2, frag2), offset1, size, snapshot2, process_index);
      } else if (state.types1_(block1, frag1) != nullptr) {
        new_type1 =
            get_offset_type(real_addr_frag1, state.types1_(block1, frag1), offset1, size, snapshot1, process_index);
        new_type2 =
            get_offset_type(real_addr_frag2, state.types1_(block1, frag1), offset2, size, snapshot2, process_index);
      } else if (state.types2_(block2, frag2) != nullptr) {
        new_type1 =
            get_offset_type(real_addr_frag1, state.types2_(block2, frag2), offset1, size, snapshot1, process_index);
        new_type2 =
            get_offset_type(real_addr_frag2, state.types2_(block2, frag2), offset2, size, snapshot2, process_index);
      } else {
        if (match_pairs)
          state.match_equals(previous);
        return -1;
      }

      if (new_type1 != nullptr && new_type2 != nullptr && new_type1 != new_type2) {

        type = new_type1;
        while (type->byte_size == 0 && type->subtype != nullptr)
          type = type->subtype;
        new_size1 = type->byte_size;

        type = new_type2;
        while (type->byte_size == 0 && type->subtype != nullptr)
          type = type->subtype;
        new_size2 = type->byte_size;

      } else {
        if (match_pairs)
          state.match_equals(previous);
        return -1;
      }
    }

    if (new_size1 > 0 && new_size1 == new_size2) {
      type = new_type1;
      size = new_size1;
    }

    if (offset1 == 0 && offset2 == 0 &&
        not previous->insert(simgrid::mc::makeHeapLocationPair(block1, frag1, block2, frag2)).second) {
      if (match_pairs)
        state.match_equals(previous);
      return 0;
    }

    if (size <= 0) {
      if (match_pairs)
        state.match_equals(previous);
      return 0;
    }

    if ((heapinfo1->busy_frag.ignore[frag1] > 0) &&
        (heapinfo2->busy_frag.ignore[frag2] == heapinfo1->busy_frag.ignore[frag1]))
      check_ignore = heapinfo1->busy_frag.ignore[frag1];

  } else
    return 1;


  /* Start comparison */
  int res_compare;
  if (type)
    res_compare = compare_heap_area_with_type(state, process_index, area1, area2, snapshot1, snapshot2, previous, type,
                                              size, check_ignore, pointer_level);
  else
    res_compare = compare_heap_area_without_type(state, process_index, area1, area2, snapshot1, snapshot2, previous,
                                                 size, check_ignore);

  if (res_compare == 1)
    return res_compare;

  if (match_pairs)
    state.match_equals(previous);
  return 0;
}

}
}

/************************** Snapshot comparison *******************************/
/******************************************************************************/

static int compare_areas_with_type(simgrid::mc::StateComparator& state,
                                   int process_index,
                                   void* real_area1, simgrid::mc::Snapshot* snapshot1, mc_mem_region_t region1,
                                   void* real_area2, simgrid::mc::Snapshot* snapshot2, mc_mem_region_t region2,
                                   simgrid::mc::Type* type, int pointer_level)
{
  simgrid::mc::RemoteClient* process = &mc_model_checker->process();

  simgrid::mc::Type* subtype;
  simgrid::mc::Type* subsubtype;
  int elm_size;
  int i;
  int res;

  do {
    switch (type->type) {
      case DW_TAG_unspecified_type:
        return 1;

      case DW_TAG_base_type:
      case DW_TAG_enumeration_type:
      case DW_TAG_union_type:
        return MC_snapshot_region_memcmp(real_area1, region1, real_area2, region2, type->byte_size) != 0;
      case DW_TAG_typedef:
      case DW_TAG_volatile_type:
      case DW_TAG_const_type:
        // Poor man's TCO:
        type = type->subtype;
        continue; // restart
      case DW_TAG_array_type:
        subtype = type->subtype;
        switch (subtype->type) {
          case DW_TAG_unspecified_type:
            return 1;

          case DW_TAG_base_type:
          case DW_TAG_enumeration_type:
          case DW_TAG_pointer_type:
          case DW_TAG_reference_type:
          case DW_TAG_rvalue_reference_type:
          case DW_TAG_structure_type:
          case DW_TAG_class_type:
          case DW_TAG_union_type:
            if (subtype->full_type)
              subtype = subtype->full_type;
            elm_size  = subtype->byte_size;
            break;
          case DW_TAG_const_type:
          case DW_TAG_typedef:
          case DW_TAG_volatile_type:
            subsubtype = subtype->subtype;
            if (subsubtype->full_type)
              subsubtype = subsubtype->full_type;
            elm_size     = subsubtype->byte_size;
            break;
          default:
            return 0;
            break;
        }
        for (i = 0; i < type->element_count; i++) {
          size_t off = i * elm_size;
          res        = compare_areas_with_type(state, process_index, (char*)real_area1 + off, snapshot1, region1,
                                        (char*)real_area2 + off, snapshot2, region2, type->subtype, pointer_level);
          if (res == 1)
            return res;
        }
        break;
      case DW_TAG_pointer_type:
      case DW_TAG_reference_type:
      case DW_TAG_rvalue_reference_type: {
        void* addr_pointed1 = MC_region_read_pointer(region1, real_area1);
        void* addr_pointed2 = MC_region_read_pointer(region2, real_area2);

        if (type->subtype && type->subtype->type == DW_TAG_subroutine_type)
          return (addr_pointed1 != addr_pointed2);
        if (addr_pointed1 == nullptr && addr_pointed2 == nullptr)
          return 0;
        if (addr_pointed1 == nullptr || addr_pointed2 == nullptr)
          return 1;
        if (not state.compared_pointers.insert(std::make_pair(addr_pointed1, addr_pointed2)).second)
          return 0;

        pointer_level++;

        // Some cases are not handled here:
        // * the pointers lead to different areas (one to the heap, the other to the RW segment ...)
        // * a pointer leads to the read-only segment of the current object
        // * a pointer lead to a different ELF object

        if (addr_pointed1 > process->heap_address && addr_pointed1 < mc_snapshot_get_heap_end(snapshot1)) {
          if (not(addr_pointed2 > process->heap_address && addr_pointed2 < mc_snapshot_get_heap_end(snapshot2)))
            return 1;
          // The pointers are both in the heap:
          return simgrid::mc::compare_heap_area(state, process_index, addr_pointed1, addr_pointed2, snapshot1,
                                                snapshot2, nullptr, type->subtype, pointer_level);

        } else if (region1->contain(simgrid::mc::remote(addr_pointed1))) {
          // The pointers are both in the current object R/W segment:
          if (not region2->contain(simgrid::mc::remote(addr_pointed2)))
            return 1;
          if (not type->type_id)
            return (addr_pointed1 != addr_pointed2);
          else
            return compare_areas_with_type(state, process_index, addr_pointed1, snapshot1, region1, addr_pointed2,
                                           snapshot2, region2, type->subtype, pointer_level);
        } else {

          // TODO, We do not handle very well the case where
          // it belongs to a different (non-heap) region from the current one.

          return (addr_pointed1 != addr_pointed2);
        }
        break;
      }
      case DW_TAG_structure_type:
      case DW_TAG_class_type:
        for (simgrid::mc::Member& member : type->members) {
          void* member1 = simgrid::dwarf::resolve_member(real_area1, type, &member, snapshot1, process_index);
          void* member2 = simgrid::dwarf::resolve_member(real_area2, type, &member, snapshot2, process_index);
          mc_mem_region_t subregion1 = mc_get_region_hinted(member1, snapshot1, process_index, region1);
          mc_mem_region_t subregion2 = mc_get_region_hinted(member2, snapshot2, process_index, region2);
          res = compare_areas_with_type(state, process_index, member1, snapshot1, subregion1, member2, snapshot2,
                                        subregion2, member.type, pointer_level);
          if (res == 1)
            return res;
        }
        break;
      case DW_TAG_subroutine_type:
        return -1;
        break;
      default:
        XBT_VERB("Unknown case: %d", type->type);
        break;
    }

    return 0;
  } while (true);
}

static int compare_global_variables(
  simgrid::mc::StateComparator& state,
  simgrid::mc::ObjectInformation* object_info,
  int process_index,
  mc_mem_region_t r1, mc_mem_region_t r2,
  simgrid::mc::Snapshot* snapshot1, simgrid::mc::Snapshot* snapshot2)
{
  xbt_assert(r1 && r2, "Missing region.");

#if HAVE_SMPI
  if (r1->storage_type() == simgrid::mc::StorageType::Privatized) {
    xbt_assert(process_index >= 0);
    if (r2->storage_type() != simgrid::mc::StorageType::Privatized)
      return 1;

    size_t process_count = MC_smpi_process_count();
    xbt_assert(process_count == r1->privatized_data().size()
      && process_count == r2->privatized_data().size());

    // Compare the global variables separately for each simulates process:
    for (size_t process_index = 0; process_index < process_count; process_index++) {
      if (compare_global_variables(state,
          object_info, process_index,
          &r1->privatized_data()[process_index],
          &r2->privatized_data()[process_index],
          snapshot1, snapshot2))
        return 1;
    }
    return 0;
  }
#else
  xbt_assert(r1->storage_type() != simgrid::mc::StorageType::Privatized);
#endif
  xbt_assert(r2->storage_type() != simgrid::mc::StorageType::Privatized);

  std::vector<simgrid::mc::Variable>& variables = object_info->global_variables;

  for (simgrid::mc::Variable const& current_var : variables) {

    // If the variable is not in this object, skip it:
    // We do not expect to find a pointer to something which is not reachable
    // by the global variables.
    if ((char *) current_var.address < (char *) object_info->start_rw
        || (char *) current_var.address > (char *) object_info->end_rw)
      continue;

    simgrid::mc::Type* bvariable_type = current_var.type;
    int res = compare_areas_with_type(state, process_index,
                                (char *) current_var.address, snapshot1, r1,
                                (char *) current_var.address, snapshot2, r2,
                                bvariable_type, 0);
    if (res == 1) {
      XBT_VERB("Global variable %s (%p) is different between snapshots",
               current_var.name.c_str(),
               (char *) current_var.address);
      return 1;
    }
  }

  return 0;
}

static int compare_local_variables(simgrid::mc::StateComparator& state,
                                   int process_index,
                                   simgrid::mc::Snapshot* snapshot1,
                                   simgrid::mc::Snapshot* snapshot2,
                                   mc_snapshot_stack_t stack1,
                                   mc_snapshot_stack_t stack2)
{
  if (stack1->local_variables.size() != stack2->local_variables.size()) {
    XBT_VERB("Different number of local variables");
    return 1;
  }

    unsigned int cursor = 0;
    local_variable_t current_var1;
    local_variable_t current_var2;
    while (cursor < stack1->local_variables.size()) {
      current_var1 = &stack1->local_variables[cursor];
      current_var2 = &stack1->local_variables[cursor];
      if (current_var1->name != current_var2->name
          || current_var1->subprogram != current_var2->subprogram
          || current_var1->ip != current_var2->ip) {
        // TODO, fix current_varX->subprogram->name to include name if DW_TAG_inlined_subprogram
        XBT_VERB
            ("Different name of variable (%s - %s) "
             "or frame (%s - %s) or ip (%lu - %lu)",
             current_var1->name.c_str(),
             current_var2->name.c_str(),
             current_var1->subprogram->name.c_str(),
             current_var2->subprogram->name.c_str(),
             current_var1->ip, current_var2->ip);
        return 1;
      }
      // TODO, fix current_varX->subprogram->name to include name if DW_TAG_inlined_subprogram

        simgrid::mc::Type* subtype = current_var1->type;
        int res                    = compare_areas_with_type(
            state, process_index, current_var1->address, snapshot1,
            mc_get_snapshot_region(current_var1->address, snapshot1, process_index), current_var2->address, snapshot2,
            mc_get_snapshot_region(current_var2->address, snapshot2, process_index), subtype, 0);

        if (res == 1) {
          // TODO, fix current_varX->subprogram->name to include name if DW_TAG_inlined_subprogram
          XBT_VERB("Local variable %s (%p - %p) in frame %s "
                   "is different between snapshots",
                   current_var1->name.c_str(), current_var1->address, current_var2->address,
                   current_var1->subprogram->name.c_str());
          return res;
      }
      cursor++;
    }
    return 0;
}

namespace simgrid {
namespace mc {

static std::unique_ptr<simgrid::mc::StateComparator> state_comparator;

int snapshot_compare(int num1, simgrid::mc::Snapshot* s1, int num2, simgrid::mc::Snapshot* s2)
{
  // TODO, make this a field of ModelChecker or something similar

  if (state_comparator == nullptr)
    state_comparator = std::unique_ptr<StateComparator>(new StateComparator());
  else
    state_comparator->clear();

  simgrid::mc::RemoteClient* process = &mc_model_checker->process();

  int errors = 0;

  int hash_result = 0;
  if (_sg_mc_hash) {
    hash_result = (s1->hash != s2->hash);
    if (hash_result) {
      XBT_VERB("(%d - %d) Different hash: 0x%" PRIx64 "--0x%" PRIx64, num1, num2, s1->hash, s2->hash);
#ifndef MC_DEBUG
      return 1;
#endif
    } else
      XBT_VERB("(%d - %d) Same hash: 0x%" PRIx64, num1, num2, s1->hash);
  }

  /* Compare enabled processes */
  if (s1->enabled_processes != s2->enabled_processes) {
    XBT_VERB("(%d - %d) Different amount of enabled processes", num1, num2);
    return 1;
  }

  /* Compare size of stacks */
  int is_diff = 0;
  for (unsigned long i = 0; i < s1->stacks.size(); i++) {
    size_t size_used1 = s1->stack_sizes[i];
    size_t size_used2 = s2->stack_sizes[i];
    if (size_used1 != size_used2) {
#ifdef MC_DEBUG
      XBT_DEBUG("(%d - %d) Different size used in stacks: %zu - %zu", num1, num2, size_used1, size_used2);
      errors++;
      is_diff = 1;
#else
#ifdef MC_VERBOSE
      XBT_VERB("(%d - %d) Different size used in stacks: %zu - %zu", num1, num2, size_used1, size_used2);
#endif
      return 1;
#endif
    }
  }
  if (is_diff) // do not proceed if there is any stacks that don't match
    return 1;

  /* Init heap information used in heap comparison algorithm */
  xbt_mheap_t heap1 = (xbt_mheap_t)s1->read_bytes(
    alloca(sizeof(struct mdesc)), sizeof(struct mdesc),
    remote(process->heap_address),
    simgrid::mc::ProcessIndexMissing, simgrid::mc::ReadOptions::lazy());
  xbt_mheap_t heap2 = (xbt_mheap_t)s2->read_bytes(
    alloca(sizeof(struct mdesc)), sizeof(struct mdesc),
    remote(process->heap_address),
    simgrid::mc::ProcessIndexMissing, simgrid::mc::ReadOptions::lazy());
  int res_init = state_comparator->initHeapInformation(heap1, heap2, &s1->to_ignore, &s2->to_ignore);

  if (res_init == -1) {
#ifdef MC_DEBUG
    XBT_DEBUG("(%d - %d) Different heap information", num1, num2);
    errors++;
#else
#ifdef MC_VERBOSE
    XBT_VERB("(%d - %d) Different heap information", num1, num2);
#endif

    return 1;
#endif
  }

  /* Stacks comparison */
  int diff_local = 0;
  for (unsigned int cursor = 0; cursor < s1->stacks.size(); cursor++) {
    mc_snapshot_stack_t stack1 = &s1->stacks[cursor];
    mc_snapshot_stack_t stack2 = &s2->stacks[cursor];

    if (stack1->process_index != stack2->process_index) {
      diff_local = 1;
      XBT_DEBUG("(%d - %d) Stacks with different process index (%i vs %i)", num1, num2,
        stack1->process_index, stack2->process_index);
    }
    else diff_local = compare_local_variables(*state_comparator,
      stack1->process_index, s1, s2, stack1, stack2);
    if (diff_local > 0) {
#ifdef MC_DEBUG
      XBT_DEBUG("(%d - %d) Different local variables between stacks %d", num1,
                num2, cursor + 1);
      errors++;
#else

#ifdef MC_VERBOSE
      XBT_VERB("(%d - %d) Different local variables between stacks %u", num1, num2, cursor + 1);
#endif

      return 1;
#endif
    }
  }

  size_t regions_count = s1->snapshot_regions.size();
  // TODO, raise a difference instead?
  xbt_assert(regions_count == s2->snapshot_regions.size());

  for (size_t k = 0; k != regions_count; ++k) {
    mc_mem_region_t region1 = s1->snapshot_regions[k].get();
    mc_mem_region_t region2 = s2->snapshot_regions[k].get();

    // Preconditions:
    if (region1->region_type() != simgrid::mc::RegionType::Data)
      continue;

    xbt_assert(region1->region_type() == region2->region_type());
    xbt_assert(region1->object_info() == region2->object_info());
    xbt_assert(region1->object_info());

    std::string const& name = region1->object_info()->file_name;

    /* Compare global variables */
    if (compare_global_variables(*state_comparator, region1->object_info(), simgrid::mc::ProcessIndexDisabled, region1,
                                 region2, s1, s2)) {

#ifdef MC_DEBUG
      XBT_DEBUG("(%d - %d) Different global variables in %s",
        num1, num2, name.c_str());
      errors++;
#else
#ifdef MC_VERBOSE
      XBT_VERB("(%d - %d) Different global variables in %s",
        num1, num2, name.c_str());
#endif

      return 1;
#endif
    }
  }

  /* Compare heap */
  if (simgrid::mc::mmalloc_compare_heap(*state_comparator, s1, s2) > 0) {

#ifdef MC_DEBUG
    XBT_DEBUG("(%d - %d) Different heap (mmalloc_compare)", num1, num2);
    errors++;
#else

#ifdef MC_VERBOSE
    XBT_VERB("(%d - %d) Different heap (mmalloc_compare)", num1, num2);
#endif
    return 1;
#endif
  }

#ifdef MC_VERBOSE
  if (errors || hash_result)
    XBT_VERB("(%d - %d) Difference found", num1, num2);
  else
    XBT_VERB("(%d - %d) No difference found", num1, num2);
#endif

#if defined(MC_DEBUG) && defined(MC_VERBOSE)
  if (_sg_mc_hash) {
    // * false positive SHOULD be avoided.
    // * There MUST not be any false negative.

    XBT_VERB("(%d - %d) State equality hash test is %s %s", num1, num2,
             (hash_result != 0) == (errors != 0) ? "true" : "false", not hash_result ? "positive" : "negative");
  }
#endif

  return errors > 0 || hash_result;
}

}
}

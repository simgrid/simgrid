/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* mc_diff - Memory snapshooting and comparison                             */

#include "src/xbt/ex_interface.h"   /* internals of backtrace setup */
#include "mc/mc.h"
#include "xbt/mmalloc.h"
#include "mc/datatypes.h"
#include "src/mc/malloc.hpp"
#include "src/mc/mc_private.h"
#include "src/mc/mc_snapshot.h"
#include "src/mc/mc_dwarf.hpp"
#include "src/mc/Type.hpp"

using simgrid::mc::remote;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_diff, xbt,
                                "Logging specific to mc_diff in mc");

/*********************************** Heap comparison ***********************************/
/***************************************************************************************/

struct XBT_PRIVATE s_mc_diff {
  s_xbt_mheap_t std_heap_copy;
  std::size_t heaplimit;
  // Number of blocks in the heaps:
  std::size_t heapsize1, heapsize2;
  std::vector<simgrid::mc::IgnoredHeapRegion>* to_ignore1;
  std::vector<simgrid::mc::IgnoredHeapRegion>* to_ignore2;
  s_heap_area_t *equals_to1, *equals_to2;
  simgrid::mc::Type **types1;
  simgrid::mc::Type **types2;
  std::size_t available;
};

#define equals_to1_(i,j) equals_to1[ MAX_FRAGMENT_PER_BLOCK*(i) + (j)]
#define equals_to2_(i,j) equals_to2[ MAX_FRAGMENT_PER_BLOCK*(i) + (j)]
#define types1_(i,j) types1[ MAX_FRAGMENT_PER_BLOCK*(i) + (j)]
#define types2_(i,j) types2[ MAX_FRAGMENT_PER_BLOCK*(i) + (j)]

static __thread struct s_mc_diff *mc_diff_info = nullptr;

/*********************************** Free functions ************************************/

static void heap_area_pair_free(heap_area_pair_t pair)
{
  xbt_free(pair);
  pair = nullptr;
}

static void heap_area_pair_free_voidp(void *d)
{
  heap_area_pair_free((heap_area_pair_t) * (void **) d);
}

static void heap_area_free(heap_area_t area)
{
  xbt_free(area);
  area = nullptr;
}

/************************************************************************************/

static s_heap_area_t make_heap_area(int block, int fragment)
{
  s_heap_area_t area;
  area.valid = 1;
  area.block = block;
  area.fragment = fragment;
  return area;
}


static int is_new_heap_area_pair(xbt_dynar_t list, int block1, int fragment1,
                                 int block2, int fragment2)
{

  unsigned int cursor = 0;
  heap_area_pair_t current_pair;

  xbt_dynar_foreach(list, cursor, current_pair)
    if (current_pair->block1 == block1 && current_pair->block2 == block2
        && current_pair->fragment1 == fragment1
        && current_pair->fragment2 == fragment2)
      return 0;

  return 1;
}

static int add_heap_area_pair(xbt_dynar_t list, int block1, int fragment1,
                              int block2, int fragment2)
{

  if (is_new_heap_area_pair(list, block1, fragment1, block2, fragment2)) {
    heap_area_pair_t pair = nullptr;
    pair = xbt_new0(s_heap_area_pair_t, 1);
    pair->block1 = block1;
    pair->fragment1 = fragment1;
    pair->block2 = block2;
    pair->fragment2 = fragment2;

    xbt_dynar_push(list, &pair);

    return 1;
  }

  return 0;
}

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

static void match_equals(struct s_mc_diff *state, xbt_dynar_t list)
{

  unsigned int cursor = 0;
  heap_area_pair_t current_pair;

  xbt_dynar_foreach(list, cursor, current_pair)

    if (current_pair->fragment1 != -1) {

      state->equals_to1_(current_pair->block1, current_pair->fragment1) =
          make_heap_area(current_pair->block2, current_pair->fragment2);
      state->equals_to2_(current_pair->block2, current_pair->fragment2) =
          make_heap_area(current_pair->block1, current_pair->fragment1);

    } else {

      state->equals_to1_(current_pair->block1, 0) =
          make_heap_area(current_pair->block2, current_pair->fragment2);
      state->equals_to2_(current_pair->block2, 0) =
          make_heap_area(current_pair->block1, current_pair->fragment1);

    }

}

/** Check whether two blocks are known to be matching
 *
 *  @param state  State used
 *  @param b1     Block of state 1
 *  @param b2     Block of state 2
 *  @return       if the blocks are known to be matching
 */
static int equal_blocks(struct s_mc_diff *state, int b1, int b2)
{

  if (state->equals_to1_(b1, 0).block == b2
      && state->equals_to2_(b2, 0).block == b1)
    return 1;

  return 0;
}

/** Check whether two fragments are known to be matching
 *
 *  @param state  State used
 *  @param b1     Block of state 1
 *  @param f1     Fragment of state 1
 *  @param b2     Block of state 2
 *  @param f2     Fragment of state 2
 *  @return       if the fragments are known to be matching
 */
static int equal_fragments(struct s_mc_diff *state, int b1, int f1, int b2,
                           int f2)
{

  if (state->equals_to1_(b1, f1).block == b2
      && state->equals_to1_(b1, f1).fragment == f2
      && state->equals_to2_(b2, f2).block == b1
      && state->equals_to2_(b2, f2).fragment == f1)
    return 1;

  return 0;
}

namespace simgrid {
namespace mc {

int init_heap_information(xbt_mheap_t heap1, xbt_mheap_t heap2,
                          std::vector<simgrid::mc::IgnoredHeapRegion>* i1,
                          std::vector<simgrid::mc::IgnoredHeapRegion>* i2)
{
  if (mc_diff_info == nullptr) {
    mc_diff_info = xbt_new0(struct s_mc_diff, 1);
    mc_diff_info->equals_to1 = nullptr;
    mc_diff_info->equals_to2 = nullptr;
    mc_diff_info->types1 = nullptr;
    mc_diff_info->types2 = nullptr;
  }
  struct s_mc_diff *state = mc_diff_info;

  if ((((struct mdesc *) heap1)->heaplimit !=
       ((struct mdesc *) heap2)->heaplimit)
      ||
      ((((struct mdesc *) heap1)->heapsize !=
        ((struct mdesc *) heap2)->heapsize)))
    return -1;

  state->heaplimit = ((struct mdesc *) heap1)->heaplimit;
  
  state->std_heap_copy = *mc_model_checker->process().get_heap();

  state->heapsize1 = heap1->heapsize;
  state->heapsize2 = heap2->heapsize;

  state->to_ignore1 = i1;
  state->to_ignore2 = i2;

  if (state->heaplimit > state->available) {
    state->equals_to1 = (s_heap_area_t*)
        realloc(state->equals_to1,
                state->heaplimit * MAX_FRAGMENT_PER_BLOCK *
                sizeof(s_heap_area_t));
    state->types1 = (simgrid::mc::Type**)
        realloc(state->types1,
                state->heaplimit * MAX_FRAGMENT_PER_BLOCK *
                sizeof(simgrid::mc::Type*));
    state->equals_to2 = (s_heap_area_t*)
        realloc(state->equals_to2,
                state->heaplimit * MAX_FRAGMENT_PER_BLOCK *
                sizeof(s_heap_area_t));
    state->types2 = (simgrid::mc::Type**)
        realloc(state->types2,
                state->heaplimit * MAX_FRAGMENT_PER_BLOCK *
                sizeof(simgrid::mc::Type*));
    state->available = state->heaplimit;
  }

  memset(state->equals_to1, 0,
         state->heaplimit * MAX_FRAGMENT_PER_BLOCK * sizeof(s_heap_area_t));
  memset(state->equals_to2, 0,
         state->heaplimit * MAX_FRAGMENT_PER_BLOCK * sizeof(s_heap_area_t));
  memset(state->types1, 0,
         state->heaplimit * MAX_FRAGMENT_PER_BLOCK * sizeof(char**));
  memset(state->types2, 0,
         state->heaplimit * MAX_FRAGMENT_PER_BLOCK * sizeof(char**));

  return 0;

}

void reset_heap_information()
{

}

// TODO, have a robust way to find it in O(1)
static inline
mc_mem_region_t MC_get_heap_region(simgrid::mc::Snapshot* snapshot)
{
  size_t n = snapshot->snapshot_regions.size();
  for (size_t i=0; i!=n; ++i) {
    mc_mem_region_t region = snapshot->snapshot_regions[i].get();
    if (region->region_type() == simgrid::mc::RegionType::Heap)
      return region;
  }
  xbt_die("No heap region");
}

int mmalloc_compare_heap(simgrid::mc::Snapshot* snapshot1, simgrid::mc::Snapshot* snapshot2)
{
  simgrid::mc::Process* process = &mc_model_checker->process();
  struct s_mc_diff *state = mc_diff_info;

  /* Start comparison */
  size_t i1, i2, j1, j2, k;
  void *addr_block1, *addr_block2, *addr_frag1, *addr_frag2;
  int nb_diff1 = 0, nb_diff2 = 0;

  int equal, res_compare = 0;

  /* Check busy blocks */

  i1 = 1;

  malloc_info heapinfo_temp1, heapinfo_temp2;
  malloc_info heapinfo_temp2b;

  mc_mem_region_t heap_region1 = MC_get_heap_region(snapshot1);
  mc_mem_region_t heap_region2 = MC_get_heap_region(snapshot2);

  // This is the address of std_heap->heapinfo in the application process:
  void* heapinfo_address = &((xbt_mheap_t) process->heap_address)->heapinfo;

  // This is in snapshot do not use them directly:
  const malloc_info* heapinfos1 = snapshot1->read<malloc_info*>(
    (std::uint64_t)heapinfo_address, simgrid::mc::ProcessIndexMissing);
  const malloc_info* heapinfos2 = snapshot2->read<malloc_info*>(
    (std::uint64_t)heapinfo_address, simgrid::mc::ProcessIndexMissing);

  while (i1 <= state->heaplimit) {

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

    addr_block1 =
        ((void *) (((ADDR2UINT(i1)) - 1) * BLOCKSIZE +
                   (char *) state->std_heap_copy.heapbase));

    if (heapinfo1->type == MMALLOC_TYPE_UNFRAGMENTED) {       /* Large block */

      if (is_stack(addr_block1)) {
        for (k = 0; k < heapinfo1->busy_block.size; k++)
          state->equals_to1_(i1 + k, 0) = make_heap_area(i1, -1);
        for (k = 0; k < heapinfo2->busy_block.size; k++)
          state->equals_to2_(i1 + k, 0) = make_heap_area(i1, -1);
        i1 += heapinfo1->busy_block.size;
        continue;
      }

      if (state->equals_to1_(i1, 0).valid) {
        i1++;
        continue;
      }

      i2 = 1;
      equal = 0;
      res_compare = 0;

      /* Try first to associate to same block in the other heap */
      if (heapinfo2->type == heapinfo1->type) {

        if (state->equals_to2_(i1, 0).valid == 0) {

          addr_block2 = (ADDR2UINT(i1) - 1) * BLOCKSIZE +
                         (char *) state->std_heap_copy.heapbase;

          res_compare =
              compare_heap_area(simgrid::mc::ProcessIndexMissing, addr_block1, addr_block2, snapshot1, snapshot2,
                                nullptr, NULL, 0);

          if (res_compare != 1) {
            for (k = 1; k < heapinfo2->busy_block.size; k++)
              state->equals_to2_(i1 + k, 0) = make_heap_area(i1, -1);
            for (k = 1; k < heapinfo1->busy_block.size; k++)
              state->equals_to1_(i1 + k, 0) = make_heap_area(i1, -1);
            equal = 1;
            i1 += heapinfo1->busy_block.size;
          }

        }

      }

      while (i2 <= state->heaplimit && !equal) {

        addr_block2 = (ADDR2UINT(i2) - 1) * BLOCKSIZE +
                       (char *) state->std_heap_copy.heapbase;

        if (i2 == i1) {
          i2++;
          continue;
        }

        const malloc_info* heapinfo2b = (const malloc_info*) MC_region_read(heap_region2, &heapinfo_temp2b, &heapinfos2[i2], sizeof(malloc_info));

        if (heapinfo2b->type != MMALLOC_TYPE_UNFRAGMENTED) {
          i2++;
          continue;
        }

        if (state->equals_to2_(i2, 0).valid) {
          i2++;
          continue;
        }

        res_compare =
            compare_heap_area(simgrid::mc::ProcessIndexMissing, addr_block1, addr_block2, snapshot1, snapshot2,
                              nullptr, NULL, 0);

        if (res_compare != 1) {
          for (k = 1; k < heapinfo2b->busy_block.size; k++)
            state->equals_to2_(i2 + k, 0) = make_heap_area(i1, -1);
          for (k = 1; k < heapinfo1->busy_block.size; k++)
            state->equals_to1_(i1 + k, 0) = make_heap_area(i2, -1);
          equal = 1;
          i1 += heapinfo1->busy_block.size;
        }

        i2++;

      }

      if (!equal) {
        XBT_DEBUG("Block %zu not found (size_used = %zu, addr = %p)", i1,
                  heapinfo1->busy_block.busy_size, addr_block1);
        i1 = state->heaplimit + 1;
        nb_diff1++;
        //i1++;
      }

    } else {                    /* Fragmented block */

      for (j1 = 0; j1 < (size_t) (BLOCKSIZE >> heapinfo1->type); j1++) {

        if (heapinfo1->busy_frag.frag_size[j1] == -1) /* Free fragment */
          continue;

        if (state->equals_to1_(i1, j1).valid)
          continue;

        addr_frag1 =
            (void *) ((char *) addr_block1 + (j1 << heapinfo1->type));

        i2 = 1;
        equal = 0;

        /* Try first to associate to same fragment in the other heap */
        if (heapinfo2->type == heapinfo1->type) {

          if (state->equals_to2_(i1, j1).valid == 0) {

            addr_block2 = (ADDR2UINT(i1) - 1) * BLOCKSIZE +
                           (char *) state->std_heap_copy.heapbase;
            addr_frag2 =
                (void *) ((char *) addr_block2 +
                          (j1 << heapinfo2->type));

            res_compare =
                compare_heap_area(simgrid::mc::ProcessIndexMissing, addr_frag1, addr_frag2, snapshot1, snapshot2,
                                  nullptr, NULL, 0);

            if (res_compare != 1)
              equal = 1;

          }

        }

        while (i2 <= state->heaplimit && !equal) {

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
            fprintf(stderr, "Unkown mmalloc block type.\n");
            abort();
          }

          for (j2 = 0; j2 < (size_t) (BLOCKSIZE >> heapinfo2b->type);
               j2++) {

            if (i2 == i1 && j2 == j1)
              continue;

            if (state->equals_to2_(i2, j2).valid)
              continue;

            addr_block2 = (ADDR2UINT(i2) - 1) * BLOCKSIZE +
                           (char *) state->std_heap_copy.heapbase;
            addr_frag2 =
                (void *) ((char *) addr_block2 +
                          (j2 << heapinfo2b->type));

            res_compare =
                compare_heap_area(simgrid::mc::ProcessIndexMissing, addr_frag1, addr_frag2, snapshot2, snapshot2,
                                  nullptr, NULL, 0);

            if (res_compare != 1) {
              equal = 1;
              break;
            }

          }

          i2++;

        }

        if (!equal) {
          XBT_DEBUG
              ("Block %zu, fragment %zu not found (size_used = %zd, address = %p)\n",
               i1, j1, heapinfo1->busy_frag.frag_size[j1],
               addr_frag1);
          i2 = state->heaplimit + 1;
          i1 = state->heaplimit + 1;
          nb_diff1++;
          break;
        }

      }

      i1++;

    }

  }

  /* All blocks/fragments are equal to another block/fragment ? */
  size_t i = 1, j = 0;

  for(i = 1; i <= state->heaplimit; i++) {
    const malloc_info* heapinfo1 = (const malloc_info*) MC_region_read(
      heap_region1, &heapinfo_temp1, &heapinfos1[i], sizeof(malloc_info));
    if (heapinfo1->type == MMALLOC_TYPE_UNFRAGMENTED) {
      if (i1 == state->heaplimit) {
        if (heapinfo1->busy_block.busy_size > 0) {
          if (state->equals_to1_(i, 0).valid == 0) {
            if (XBT_LOG_ISENABLED(mc_diff, xbt_log_priority_debug)) {
              // TODO, add address
              XBT_DEBUG("Block %zu not found (size used = %zu)", i,
                        heapinfo1->busy_block.busy_size);
              //mmalloc_backtrace_block_display((void*)heapinfo1, i);
            }
            nb_diff1++;
          }
        }
      }
    }
    if (heapinfo1->type > 0) {
      for (j = 0; j < (size_t) (BLOCKSIZE >> heapinfo1->type); j++) {
        if (i1 == state->heaplimit) {
          if (heapinfo1->busy_frag.frag_size[j] > 0) {
            if (state->equals_to1_(i, j).valid == 0) {
              if (XBT_LOG_ISENABLED(mc_diff, xbt_log_priority_debug)) {
                // TODO, print fragment address
                XBT_DEBUG
                    ("Block %zu, Fragment %zu not found (size used = %zd)",
                     i, j,
                     heapinfo1->busy_frag.frag_size[j]);
                //mmalloc_backtrace_fragment_display((void*)heapinfo1, i, j);
              }
              nb_diff1++;
            }
          }
        }
      }
    }
  }

  if (i1 == state->heaplimit)
    XBT_DEBUG("Number of blocks/fragments not found in heap1 : %d", nb_diff1);

  for (i=1; i <= state->heaplimit; i++) {
    const malloc_info* heapinfo2 = (const malloc_info*) MC_region_read(
      heap_region2, &heapinfo_temp2, &heapinfos2[i], sizeof(malloc_info));
    if (heapinfo2->type == MMALLOC_TYPE_UNFRAGMENTED) {
      if (i1 == state->heaplimit) {
        if (heapinfo2->busy_block.busy_size > 0) {
          if (state->equals_to2_(i, 0).valid == 0) {
            if (XBT_LOG_ISENABLED(mc_diff, xbt_log_priority_debug)) {
              // TODO, print address of the block
              XBT_DEBUG("Block %zu not found (size used = %zu)", i,
                        heapinfo2->busy_block.busy_size);
              //mmalloc_backtrace_block_display((void*)heapinfo2, i);
            }
            nb_diff2++;
          }
        }
      }
    }
    if (heapinfo2->type > 0) {
      for (j = 0; j < (size_t) (BLOCKSIZE >> heapinfo2->type); j++) {
        if (i1 == state->heaplimit) {
          if (heapinfo2->busy_frag.frag_size[j] > 0) {
            if (state->equals_to2_(i, j).valid == 0) {
              if (XBT_LOG_ISENABLED(mc_diff, xbt_log_priority_debug)) {
                // TODO, print address of the block
                XBT_DEBUG
                    ("Block %zu, Fragment %zu not found (size used = %zd)",
                     i, j,
                     heapinfo2->busy_frag.frag_size[j]);
                //mmalloc_backtrace_fragment_display((void*)heapinfo2, i, j);
              }
              nb_diff2++;
            }
          }
        }
      }
    }
  }

  if (i1 == state->heaplimit)
    XBT_DEBUG("Number of blocks/fragments not found in heap2 : %d", nb_diff2);

  return ((nb_diff1 > 0) || (nb_diff2 > 0));
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
static int compare_heap_area_without_type(struct s_mc_diff *state, int process_index,
                                          const void *real_area1, const void *real_area2,
                                          simgrid::mc::Snapshot* snapshot1,
                                          simgrid::mc::Snapshot* snapshot2,
                                          xbt_dynar_t previous, int size,
                                          int check_ignore)
{
  simgrid::mc::Process* process = &mc_model_checker->process();

  int i = 0;
  const void *addr_pointed1, *addr_pointed2;
  int pointer_align, res_compare;
  ssize_t ignore1, ignore2;

  mc_mem_region_t heap_region1 = MC_get_heap_region(snapshot1);
  mc_mem_region_t heap_region2 = MC_get_heap_region(snapshot2);

  while (i < size) {

    if (check_ignore > 0) {
      if ((ignore1 =
           heap_comparison_ignore_size(state->to_ignore1,
                                       (char *) real_area1 + i)) != -1) {
        if ((ignore2 =
             heap_comparison_ignore_size(state->to_ignore2,
                                         (char *) real_area2 + i)) == ignore1) {
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

      pointer_align = (i / sizeof(void *)) * sizeof(void *);
      addr_pointed1 = snapshot1->read(
        remote((void**)((char *) real_area1 + pointer_align)), process_index);
      addr_pointed2 = snapshot2->read(
        remote((void**)((char *) real_area2 + pointer_align)), process_index);

      if (process->in_maestro_stack(remote(addr_pointed1))
        && process->in_maestro_stack(remote(addr_pointed2))) {
        i = pointer_align + sizeof(void *);
        continue;
      } else if (addr_pointed1 > state->std_heap_copy.heapbase
                 && addr_pointed1 < mc_snapshot_get_heap_end(snapshot1)
                 && addr_pointed2 > state->std_heap_copy.heapbase
                 && addr_pointed2 < mc_snapshot_get_heap_end(snapshot2)) {
        // Both addreses are in the heap:
        res_compare =
            compare_heap_area(process_index, addr_pointed1, addr_pointed2, snapshot1,
                              snapshot2, previous, nullptr, 0);
        if (res_compare == 1)
          return res_compare;
        i = pointer_align + sizeof(void *);
        continue;
      } else
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
 * @param type_id
 * @param area_size      either a byte_size or an elements_count (?)
 * @param check_ignore
 * @param pointer_level
 * @return               0 (same), 1 (different), -1 (unknown)
 */
static int compare_heap_area_with_type(struct s_mc_diff *state, int process_index,
                                       const void *real_area1, const void *real_area2,
                                       simgrid::mc::Snapshot* snapshot1,
                                       simgrid::mc::Snapshot* snapshot2,
                                       xbt_dynar_t previous, simgrid::mc::Type* type,
                                       int area_size, int check_ignore,
                                       int pointer_level)
{
top:
  // HACK: This should not happen but in pratice, there is some
  // DW_TAG_typedef without DW_AT_type. We should fix this somehow.
  if (type == nullptr)
    return 0;

  if (is_stack(real_area1) && is_stack(real_area2))
    return 0;
  ssize_t ignore1, ignore2;

  if ((check_ignore > 0)
      && ((ignore1 = heap_comparison_ignore_size(state->to_ignore1, real_area1))
          > 0)
      && ((ignore2 = heap_comparison_ignore_size(state->to_ignore2, real_area2))
          == ignore1))
    return 0;

  simgrid::mc::Type *subtype, *subsubtype;
  int res, elm_size;
  const void *addr_pointed1, *addr_pointed2;

  mc_mem_region_t heap_region1 = MC_get_heap_region(snapshot1);
  mc_mem_region_t heap_region2 = MC_get_heap_region(snapshot2);

  switch (type->type) {
  case DW_TAG_unspecified_type:
    return 1;

  case DW_TAG_base_type:
    if (!type->name.empty() && type->name == "char") {        /* String, hence random (arbitrary ?) size */
      if (real_area1 == real_area2)
        return -1;
      else
        return (MC_snapshot_region_memcmp(real_area1, heap_region1, real_area2, heap_region2, area_size) != 0);
    } else {
      if (area_size != -1 && type->byte_size != area_size)
        return -1;
      else
        return (MC_snapshot_region_memcmp(real_area1, heap_region1, real_area2, heap_region2, type->byte_size) != 0);
    }
    break;
  case DW_TAG_enumeration_type:
    if (area_size != -1 && type->byte_size != area_size)
      return -1;
    else
      return (MC_snapshot_region_memcmp(real_area1, heap_region1, real_area2, heap_region2, type->byte_size) != 0);
    break;
  case DW_TAG_typedef:
  case DW_TAG_const_type:
  case DW_TAG_volatile_type:
    // Poor man's TCO:
    type = type->subtype;
    goto top;
    break;
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
      elm_size = subtype->byte_size;
      break;
      // TODO, just remove the type indirection?
    case DW_TAG_const_type:
    case DW_TAG_typedef:
    case DW_TAG_volatile_type:
      subsubtype = subtype->subtype;
      if (subsubtype->full_type)
        subsubtype = subsubtype->full_type;
      elm_size = subsubtype->byte_size;
      break;
    default:
      return 0;
      break;
    }
    for (int i = 0; i < type->element_count; i++) {
      // TODO, add support for variable stride (DW_AT_byte_stride)
      res =
          compare_heap_area_with_type(state, process_index,
                                      (char *) real_area1 + (i * elm_size),
                                      (char *) real_area2 + (i * elm_size),
                                      snapshot1, snapshot2, previous,
                                      type->subtype, subtype->byte_size,
                                      check_ignore, pointer_level);
      if (res == 1)
        return res;
    }
    break;
  case DW_TAG_reference_type:
  case DW_TAG_rvalue_reference_type:
  case DW_TAG_pointer_type:
    if (type->subtype && type->subtype->type == DW_TAG_subroutine_type) {
      addr_pointed1 = snapshot1->read(remote((void**)real_area1), process_index);
      addr_pointed2 = snapshot2->read(remote((void**)real_area2), process_index);
      return (addr_pointed1 != addr_pointed2);;
    } else {
      pointer_level++;
      if (pointer_level > 1) {  /* Array of pointers */
        for (size_t i = 0; i < (area_size / sizeof(void *)); i++) {
          addr_pointed1 = snapshot1->read(
            remote((void**)((char*) real_area1 + i * sizeof(void *))),
            process_index);
          addr_pointed2 = snapshot2->read(
            remote((void**)((char*) real_area2 + i * sizeof(void *))),
            process_index);
          if (addr_pointed1 > state->std_heap_copy.heapbase
              && addr_pointed1 < mc_snapshot_get_heap_end(snapshot1)
              && addr_pointed2 > state->std_heap_copy.heapbase
              && addr_pointed2 < mc_snapshot_get_heap_end(snapshot2))
            res =
                compare_heap_area(process_index, addr_pointed1, addr_pointed2, snapshot1,
                                  snapshot2, previous, type->subtype,
                                  pointer_level);
          else
            res = (addr_pointed1 != addr_pointed2);
          if (res == 1)
            return res;
        }
      } else {
        addr_pointed1 = snapshot1->read(remote((void**)real_area1), process_index);
        addr_pointed2 = snapshot2->read(remote((void**)real_area2), process_index);
        if (addr_pointed1 > state->std_heap_copy.heapbase
            && addr_pointed1 < mc_snapshot_get_heap_end(snapshot1)
            && addr_pointed2 > state->std_heap_copy.heapbase
            && addr_pointed2 < mc_snapshot_get_heap_end(snapshot2))
          return compare_heap_area(process_index, addr_pointed1, addr_pointed2, snapshot1,
                                   snapshot2, previous, type->subtype,
                                   pointer_level);
        else
          return (addr_pointed1 != addr_pointed2);
      }
    }
    break;
  case DW_TAG_structure_type:
  case DW_TAG_class_type:
    if (type->full_type)
      type = type->full_type;
    if (area_size != -1 && type->byte_size != area_size) {
      if (area_size > type->byte_size && area_size % type->byte_size == 0) {
        for (size_t i = 0; i < (size_t)(area_size / type->byte_size); i++) {
          res =
              compare_heap_area_with_type(state, process_index,
                                          (char *) real_area1 + i * type->byte_size,
                                          (char *) real_area2 + i * type->byte_size,
                                          snapshot1, snapshot2, previous, type, -1,
                                          check_ignore, 0);
          if (res == 1)
            return res;
        }
      } else
        return -1;
    } else {
      for(simgrid::mc::Member& member : type->members) {
        // TODO, optimize this? (for the offset case)
        void *real_member1 = simgrid::dwarf::resolve_member(
          real_area1, type, &member, (simgrid::mc::AddressSpace*) snapshot1, process_index);
        void *real_member2 = simgrid::dwarf::resolve_member(
            real_area2, type, &member, (simgrid::mc::AddressSpace*) snapshot2, process_index);
        res =
          compare_heap_area_with_type(state, process_index, real_member1, real_member2,
                                        snapshot1, snapshot2,
                                        previous, member.type, -1,
                                        check_ignore, 0);
        if (res == 1)
          return res;
      }
    }
    break;
  case DW_TAG_union_type:
    return compare_heap_area_without_type(state, process_index, real_area1, real_area2,
                                          snapshot1, snapshot2, previous,
                                          type->byte_size, check_ignore);
    break;
  default:
    break;
  }

  return 0;

}

/** Infer the type of a part of the block from the type of the block
 *
 * TODO, handle DW_TAG_array_type as well as arrays of the object ((*p)[5], p[5])
 *
 * TODO, handle subfields ((*p).bar.foo, (*p)[5].bar…)
 *
 * @param  type_id            DWARF type ID of the root address
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
    } else {
      for(simgrid::mc::Member& member : type->members) {

        if (member.has_offset_location()) {
          // We have the offset, use it directly (shortcut):
          if (member.offset() == offset)
            return member.type;
        } else {
          void *real_member = simgrid::dwarf::resolve_member(
            real_base_address, type, &member, snapshot, process_index);
          if ((char*) real_member - (char *) real_base_address == offset)
            return member.type;
        }

      }
      return nullptr;
    }
    break;
  default:
    /* FIXME : other cases ? */
    return nullptr;
    break;
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
int compare_heap_area(int process_index, const void *area1, const void *area2, simgrid::mc::Snapshot* snapshot1,
                      simgrid::mc::Snapshot* snapshot2, xbt_dynar_t previous,
                      simgrid::mc::Type* type, int pointer_level)
{
  simgrid::mc::Process* process = &mc_model_checker->process();

  struct s_mc_diff *state = mc_diff_info;

  int res_compare;
  ssize_t block1, frag1, block2, frag2;
  ssize_t size;
  int check_ignore = 0;

  void *real_addr_block1, *real_addr_block2, *real_addr_frag1, *real_addr_frag2;
  int type_size = -1;
  int offset1 = 0, offset2 = 0;
  int new_size1 = -1, new_size2 = -1;
  simgrid::mc::Type *new_type1 = nullptr, *new_type2 = NULL;

  int match_pairs = 0;

  // This is the address of std_heap->heapinfo in the application process:
  void* heapinfo_address = &((xbt_mheap_t) process->heap_address)->heapinfo;

  const malloc_info* heapinfos1 = snapshot1->read(
    remote((const malloc_info**)heapinfo_address), process_index);
  const malloc_info* heapinfos2 = snapshot2->read(
    remote((const malloc_info**)heapinfo_address), process_index);

  malloc_info heapinfo_temp1, heapinfo_temp2;

  if (previous == nullptr) {
    previous =
        xbt_dynar_new(sizeof(heap_area_pair_t), heap_area_pair_free_voidp);
    match_pairs = 1;
  }
  // Get block number:
  block1 =
      ((char *) area1 -
       (char *) state->std_heap_copy.heapbase) / BLOCKSIZE + 1;
  block2 =
      ((char *) area2 -
       (char *) state->std_heap_copy.heapbase) / BLOCKSIZE + 1;

  // If either block is a stack block:
  if (is_block_stack((int) block1) && is_block_stack((int) block2)) {
    add_heap_area_pair(previous, block1, -1, block2, -1);
    if (match_pairs) {
      match_equals(state, previous);
      xbt_dynar_free(&previous);
    }
    return 0;
  }
  // If either block is not in the expected area of memory:
  if (((char *) area1 < (char *) state->std_heap_copy.heapbase)
      || (block1 > (ssize_t) state->heapsize1) || (block1 < 1)
      || ((char *) area2 < (char *) state->std_heap_copy.heapbase)
      || (block2 > (ssize_t) state->heapsize2) || (block2 < 1)) {
    if (match_pairs)
      xbt_dynar_free(&previous);
    return 1;
  }

  // Process address of the block:
  real_addr_block1 = (ADDR2UINT(block1) - 1) * BLOCKSIZE +
                 (char *) state->std_heap_copy.heapbase;
  real_addr_block2 = (ADDR2UINT(block2) - 1) * BLOCKSIZE +
                 (char *) state->std_heap_copy.heapbase;

  if (type) {

    if (type->full_type)
      type = type->full_type;

    // This assume that for "boring" types (volatile ...) byte_size is absent:
    while (type->byte_size == 0 && type->subtype != nullptr)
      type = type->subtype;

    // Find type_size:
    if ((type->type == DW_TAG_pointer_type)
        || ((type->type == DW_TAG_base_type) && !type->name.empty()
            && type->name == "char"))
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
    if (match_pairs) {
      match_equals(state, previous);
      xbt_dynar_free(&previous);
    }
    return 0;

  } else if (heapinfo1->type == MMALLOC_TYPE_UNFRAGMENTED
    && heapinfo2->type == MMALLOC_TYPE_UNFRAGMENTED) {
    /* Complete block */

    // TODO, lookup variable type from block type as done for fragmented blocks

    offset1 = (char *) area1 - (char *) real_addr_block1;
    offset2 = (char *) area2 - (char *) real_addr_block2;

    if (state->equals_to1_(block1, 0).valid
        && state->equals_to2_(block2, 0).valid) {
      if (equal_blocks(state, block1, block2)) {
        if (match_pairs) {
          match_equals(state, previous);
          xbt_dynar_free(&previous);
        }
        return 0;
      }
    }

    if (type_size != -1) {
      if (type_size != (ssize_t) heapinfo1->busy_block.busy_size
          && type_size != (ssize_t)   heapinfo2->busy_block.busy_size
          && (type->name.empty() || type->name == "struct s_smx_context")) {
        if (match_pairs) {
          match_equals(state, previous);
          xbt_dynar_free(&previous);
        }
        return -1;
      }
    }

    if (heapinfo1->busy_block.size !=
        heapinfo2->busy_block.size) {
      if (match_pairs)
        xbt_dynar_free(&previous);
      return 1;
    }

    if (heapinfo1->busy_block.busy_size !=
        heapinfo2->busy_block.busy_size) {
      if (match_pairs)
        xbt_dynar_free(&previous);
      return 1;
    }

    if (!add_heap_area_pair(previous, block1, -1, block2, -1)) {
      if (match_pairs) {
        match_equals(state, previous);
        xbt_dynar_free(&previous);
      }
      return 0;
    }

    size = heapinfo1->busy_block.busy_size;

    // Remember (basic) type inference.
    // The current data structure only allows us to do this for the whole block.
    if (type != nullptr && area1 == real_addr_block1)
      state->types1_(block1, 0) = type;
    if (type != nullptr && area2 == real_addr_block2)
      state->types2_(block2, 0) = type;

    if (size <= 0) {
      if (match_pairs) {
        match_equals(state, previous);
        xbt_dynar_free(&previous);
      }
      return 0;
    }

    frag1 = -1;
    frag2 = -1;

    if ((heapinfo1->busy_block.ignore > 0)
        && (heapinfo2->busy_block.ignore ==
            heapinfo1->busy_block.ignore))
      check_ignore = heapinfo1->busy_block.ignore;

  } else if ((heapinfo1->type > 0) && (heapinfo2->type > 0)) {      /* Fragmented block */

    // Fragment number:
    frag1 =
        ((uintptr_t) (ADDR2UINT(area1) % (BLOCKSIZE))) >> heapinfo1->type;
    frag2 =
        ((uintptr_t) (ADDR2UINT(area2) % (BLOCKSIZE))) >> heapinfo2->type;

    // Process address of the fragment:
    real_addr_frag1 =
        (void *) ((char *) real_addr_block1 +
                  (frag1 << heapinfo1->type));
    real_addr_frag2 =
        (void *) ((char *) real_addr_block2 +
                  (frag2 << heapinfo2->type));

    // Check the size of the fragments against the size of the type:
    if (type_size != -1) {
      if (heapinfo1->busy_frag.frag_size[frag1] == -1
          || heapinfo2->busy_frag.frag_size[frag2] == -1) {
        if (match_pairs) {
          match_equals(state, previous);
          xbt_dynar_free(&previous);
        }
        return -1;
      }
      // ?
      if (type_size != heapinfo1->busy_frag.frag_size[frag1]
          || type_size != heapinfo2->busy_frag.frag_size[frag2]) {
        if (match_pairs) {
          match_equals(state, previous);
          xbt_dynar_free(&previous);
        }
        return -1;
      }
    }

    // Check if the blocks are already matched together:
    if (state->equals_to1_(block1, frag1).valid
        && state->equals_to2_(block2, frag2).valid) {
      if (offset1==offset2 && equal_fragments(state, block1, frag1, block2, frag2)) {
        if (match_pairs) {
          match_equals(state, previous);
          xbt_dynar_free(&previous);
        }
        return 0;
      }
    }
    // Compare the size of both fragments:
    if (heapinfo1->busy_frag.frag_size[frag1] !=
        heapinfo2->busy_frag.frag_size[frag2]) {
      if (type_size == -1) {
        if (match_pairs) {
          match_equals(state, previous);
          xbt_dynar_free(&previous);
        }
        return -1;
      } else {
        if (match_pairs)
          xbt_dynar_free(&previous);
        return 1;
      }
    }

    // Size of the fragment:
    size = heapinfo1->busy_frag.frag_size[frag1];

    // Remember (basic) type inference.
    // The current data structure only allows us to do this for the whole fragment.
    if (type != nullptr && area1 == real_addr_frag1)
      state->types1_(block1, frag1) = type;
    if (type != nullptr && area2 == real_addr_frag2)
      state->types2_(block2, frag2) = type;

    // The type of the variable is already known:
    if (type) {
      new_type1 = type;
      new_type2 = type;
    }
    // Type inference from the block type.
    else if (state->types1_(block1, frag1) != nullptr
             || state->types2_(block2, frag2) != nullptr) {

      offset1 = (char *) area1 - (char *) real_addr_frag1;
      offset2 = (char *) area2 - (char *) real_addr_frag2;

      if (state->types1_(block1, frag1) != nullptr
          && state->types2_(block2, frag2) != nullptr) {
        new_type1 =
            get_offset_type(real_addr_frag1, state->types1_(block1, frag1),
                            offset1, size, snapshot1, process_index);
        new_type2 =
            get_offset_type(real_addr_frag2, state->types2_(block2, frag2),
                            offset1, size, snapshot2, process_index);
      } else if (state->types1_(block1, frag1) != nullptr) {
        new_type1 =
            get_offset_type(real_addr_frag1, state->types1_(block1, frag1),
                            offset1, size, snapshot1, process_index);
        new_type2 =
            get_offset_type(real_addr_frag2, state->types1_(block1, frag1),
                            offset2, size, snapshot2, process_index);
      } else if (state->types2_(block2, frag2) != nullptr) {
        new_type1 =
            get_offset_type(real_addr_frag1, state->types2_(block2, frag2),
                            offset1, size, snapshot1, process_index);
        new_type2 =
            get_offset_type(real_addr_frag2, state->types2_(block2, frag2),
                            offset2, size, snapshot2, process_index);
      } else {
        if (match_pairs) {
          match_equals(state, previous);
          xbt_dynar_free(&previous);
        }
        return -1;
      }

      if (new_type1 != nullptr && new_type2 != NULL && new_type1 != new_type2) {

        type = new_type1;
        while (type->byte_size == 0 && type->subtype != nullptr)
          type = type->subtype;
        new_size1 = type->byte_size;

        type = new_type2;
        while (type->byte_size == 0 && type->subtype != nullptr)
          type = type->subtype;
        new_size2 = type->byte_size;

      } else {
        if (match_pairs) {
          match_equals(state, previous);
          xbt_dynar_free(&previous);
        }
        return -1;
      }
    }

    if (new_size1 > 0 && new_size1 == new_size2) {
      type = new_type1;
      size = new_size1;
    }

    if (offset1 == 0 && offset2 == 0
      && !add_heap_area_pair(previous, block1, frag1, block2, frag2)) {
        if (match_pairs) {
          match_equals(state, previous);
          xbt_dynar_free(&previous);
        }
        return 0;
      }

    if (size <= 0) {
      if (match_pairs) {
        match_equals(state, previous);
        xbt_dynar_free(&previous);
      }
      return 0;
    }

    if ((heapinfo1->busy_frag.ignore[frag1] > 0)
        && (heapinfo2->busy_frag.ignore[frag2] ==
            heapinfo1->busy_frag.ignore[frag1]))
      check_ignore = heapinfo1->busy_frag.ignore[frag1];

  } else {

    if (match_pairs)
      xbt_dynar_free(&previous);
    return 1;

  }


  /* Start comparison */
  if (type)
    res_compare =
        compare_heap_area_with_type(state, process_index, area1, area2, snapshot1, snapshot2,
                                    previous, type, size, check_ignore,
                                    pointer_level);
  else
    res_compare =
        compare_heap_area_without_type(state, process_index, area1, area2, snapshot1, snapshot2,
                                       previous, size, check_ignore);

  if (res_compare == 1) {
    if (match_pairs)
      xbt_dynar_free(&previous);
    return res_compare;
  }

  if (match_pairs) {
    match_equals(state, previous);
    xbt_dynar_free(&previous);
  }

  return 0;
}

}
}

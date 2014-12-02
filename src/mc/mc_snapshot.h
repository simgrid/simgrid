/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_SNAPSHOT_H
#define MC_SNAPSHOT_H

#include <sys/types.h> // off_t
#include <stdint.h> // size_t

#include <simgrid_config.h>
#include "../xbt/mmalloc/mmprivate.h"
#include <xbt/asserts.h>
#include <xbt/dynar.h>

#include "mc_forward.h"
#include "mc_model_checker.h"
#include "mc_page_store.h"
#include "mc_mmalloc.h"

SG_BEGIN_DECL()

void mc_softdirty_reset();

// ***** Snapshot region

#define NB_REGIONS 3 /* binary data (data + BSS) (type = 2), libsimgrid data (data + BSS) (type = 1), std_heap (type = 0)*/

/** @brief Copy/snapshot of a given memory region
 *
 *  Two types of region snapshots exist:
 *  <ul>
 *    <li>flat/dense snapshots are a simple copy of the region;</li>
 *    <li>sparse/per-page snapshots are snaapshots which shared
 *    identical pages.</li>
 *  </ul>
 */
typedef struct s_mc_mem_region{
  /** @brief  Virtual address of the region in the simulated process */
  void *start_addr;

  /** @brief Permanent virtual address of the region
   *
   * This is usually the same address as the simuilated process address.
   * However, when using SMPI privatization of global variables,
   * each SMPI process has its own set of global variables stored
   * at a different virtual address. The scheduler maps those region
   * on the region of the global variables.
   *
   * */
  void *permanent_addr;

  /** @brief Copy of the snapshot for flat snapshots regions (NULL otherwise) */
  void *data;

  /** @brief Size of the data region in bytes */
  size_t size;

  /** @brief Pages indices in the page store for per-page snapshots (NULL otherwise) */
  size_t* page_numbers;

} s_mc_mem_region_t, *mc_mem_region_t;

mc_mem_region_t mc_region_new_sparse(int type, void *start_addr, void* data_addr, size_t size, mc_mem_region_t ref_reg);
void MC_region_destroy(mc_mem_region_t reg);
void mc_region_restore_sparse(mc_mem_region_t reg, mc_mem_region_t ref_reg);

static inline  __attribute__ ((always_inline))
bool mc_region_contain(mc_mem_region_t region, void* p)
{
  return p >= region->start_addr &&
    p < (void*)((char*) region->start_addr + region->size);
}

static inline __attribute__((always_inline))
void* mc_translate_address_region(uintptr_t addr, mc_mem_region_t region)
{
    size_t pageno = mc_page_number(region->start_addr, (void*) addr);
    size_t snapshot_pageno = region->page_numbers[pageno];
    const void* snapshot_page = mc_page_store_get_page(mc_model_checker->pages, snapshot_pageno);
    return (char*) snapshot_page + mc_page_offset((void*) addr);
}

mc_mem_region_t mc_get_snapshot_region(void* addr, mc_snapshot_t snapshot, int process_index);

/** \brief Translate a pointer from process address space to snapshot address space
 *
 *  The address space contains snapshot of the main/application memory:
 *  this function finds the address in a given snaphot for a given
 *  real/application address.
 *
 *  For read only memory regions and other regions which are not int the
 *  snapshot, the address is not changed.
 *
 *  \param addr     Application address
 *  \param snapshot The snapshot of interest (if NULL no translation is done)
 *  \return         Translated address in the snapshot address space
 * */
static inline __attribute__((always_inline))
void* mc_translate_address(uintptr_t addr, mc_snapshot_t snapshot, int process_index)
{

  // If not in a process state/clone:
  if (!snapshot) {
    return (uintptr_t *) addr;
  }

  mc_mem_region_t region = mc_get_snapshot_region((void*) addr, snapshot, process_index);

  xbt_assert(mc_region_contain(region, (void*) addr), "Trying to read out of the region boundary.");

  if (!region) {
    return (void *) addr;
  }

  // Flat snapshot:
  else if (region->data) {
    uintptr_t offset = addr - (uintptr_t) region->start_addr;
    return (void *) ((uintptr_t) region->data + offset);
  }

  // Per-page snapshot:
  else if (region->page_numbers) {
    return mc_translate_address_region(addr, region);
  }

  else {
    xbt_die("No data for this memory region");
  }
}

// ***** MC Snapshot

/** Ignored data
 *
 *  Some parts of the snapshot are ignored by zeroing them out: the real
 *  values is stored here.
 * */
typedef struct s_mc_snapshot_ignored_data {
  void* start;
  size_t size;
  void* data;
} s_mc_snapshot_ignored_data_t, *mc_snapshot_ignored_data_t;

typedef struct s_fd_infos{
  char *filename;
  int number;
  off_t current_position;
  int flags;
}s_fd_infos_t, *fd_infos_t;

struct s_mc_snapshot{
  size_t heap_bytes_used;
  mc_mem_region_t regions[NB_REGIONS];
  xbt_dynar_t enabled_processes;
  mc_mem_region_t* privatization_regions;
  int privatization_index;
  size_t *stack_sizes;
  xbt_dynar_t stacks;
  xbt_dynar_t to_ignore;
  uint64_t hash;
  xbt_dynar_t ignored_data;
  int total_fd;
  fd_infos_t *current_fd;
};

/** @brief Process index used when no process is available
 *
 *  The expected behaviour is that if a process index is needed it will fail.
 * */
#define MC_NO_PROCESS_INDEX -1

/** @brief Process index when any process is suitable
 *
 * We could use a special negative value in the future.
 */
#define MC_ANY_PROCESS_INDEX 0

static inline __attribute__ ((always_inline))
mc_mem_region_t mc_get_region_hinted(void* addr, mc_snapshot_t snapshot, int process_index, mc_mem_region_t region)
{
  if (mc_region_contain(region, addr))
    return region;
  else
    return mc_get_snapshot_region(addr, snapshot, process_index);
}

/** Information about a given stack frame
 *
 */
typedef struct s_mc_stack_frame {
  /** Instruction pointer */
  unw_word_t ip;
  /** Stack pointer */
  unw_word_t sp;
  unw_word_t frame_base;
  dw_frame_t frame;
  char* frame_name;
  unw_cursor_t unw_cursor;
} s_mc_stack_frame_t, *mc_stack_frame_t;

typedef struct s_mc_snapshot_stack{
  xbt_dynar_t local_variables;
  xbt_dynar_t stack_frames; // mc_stack_frame_t
  int process_index;
}s_mc_snapshot_stack_t, *mc_snapshot_stack_t;

typedef struct s_mc_global_t{
  mc_snapshot_t snapshot;
  int raw_mem_set;
  int prev_pair;
  char *prev_req;
  int initial_communications_pattern_done;
  int comm_deterministic;
  int send_deterministic;
}s_mc_global_t, *mc_global_t;

typedef struct s_mc_checkpoint_ignore_region{
  void *addr;
  size_t size;
}s_mc_checkpoint_ignore_region_t, *mc_checkpoint_ignore_region_t;

static void* mc_snapshot_get_heap_end(mc_snapshot_t snapshot);

mc_snapshot_t MC_take_snapshot(int num_state);
void MC_restore_snapshot(mc_snapshot_t);
void MC_free_snapshot(mc_snapshot_t);

int mc_important_snapshot(mc_snapshot_t snapshot);

size_t* mc_take_page_snapshot_region(void* data, size_t page_count, uint64_t* pagemap, size_t* reference_pages);
void mc_free_page_snapshot_region(size_t* pagenos, size_t page_count);
void mc_restore_page_snapshot_region(void* start_addr, size_t page_count, size_t* pagenos, uint64_t* pagemap, size_t* reference_pagenos);

static inline __attribute__((always_inline))
bool mc_snapshot_region_linear(mc_mem_region_t region) {
  return !region || !region->data;
}

void* mc_snapshot_read_fragmented(void* addr, mc_mem_region_t region, void* target, size_t size);

void* mc_snapshot_read(void* addr, mc_snapshot_t snapshot, int process_index, void* target, size_t size);
int mc_snapshot_region_memcmp(
  void* addr1, mc_mem_region_t region1,
  void* addr2, mc_mem_region_t region2, size_t size);
int mc_snapshot_memcmp(
  void* addr1, mc_snapshot_t snapshot1,
  void* addr2, mc_snapshot_t snapshot2, int process_index, size_t size);

static void* mc_snapshot_read_pointer(void* addr, mc_snapshot_t snapshot, int process_index);

static inline __attribute__ ((always_inline))
void* mc_snapshot_read_pointer(void* addr, mc_snapshot_t snapshot, int process_index)
{
  void* res;
  return *(void**) mc_snapshot_read(addr, snapshot, process_index, &res, sizeof(void*));
}

static inline __attribute__ ((always_inline))
  void* mc_snapshot_get_heap_end(mc_snapshot_t snapshot) {
  if(snapshot==NULL)
      xbt_die("snapshot is NULL");
  void** addr = &(std_heap->breakval);
  return mc_snapshot_read_pointer(addr, snapshot, MC_ANY_PROCESS_INDEX);
}

/** @brief Read memory from a snapshot region
 *
 *  @param addr    Process (non-snapshot) address of the data
 *  @param region  Snapshot memory region where the data is located
 *  @param target  Buffer to store the value
 *  @param size    Size of the data to read in bytes
 *  @return Pointer where the data is located (target buffer of original location)
 */
static inline __attribute__((always_inline))
void* mc_snapshot_read_region(void* addr, mc_mem_region_t region, void* target, size_t size)
{
  if (region==NULL)
    return addr;

  uintptr_t offset = (char*) addr - (char*) region->start_addr;

  xbt_assert(mc_region_contain(region, addr),
    "Trying to read out of the region boundary.");

  // Linear memory region:
  if (region->data) {
    return (char*) region->data + offset;
  }

  // Fragmented memory region:
  else if (region->page_numbers) {
    // Last byte of the region:
    void* end = (char*) addr + size - 1;
    if( mc_same_page(addr, end) ) {
      // The memory is contained in a single page:
      return mc_translate_address_region((uintptr_t) addr, region);
    } else {
      // The memory spans several pages:
      return mc_snapshot_read_fragmented(addr, region, target, size);
    }
  }

  else {
    xbt_die("No data available for this region");
  }
}

static inline __attribute__ ((always_inline))
void* mc_snapshot_read_pointer_region(void* addr, mc_mem_region_t region)
{
  void* res;
  return *(void**) mc_snapshot_read_region(addr, region, &res, sizeof(void*));
}

SG_END_DECL()

#endif

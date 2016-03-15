/* Copyright (c) 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define __STDC_FORMAT_MACROS
#include <cinttypes>

#include <utility>
#include <unordered_set>

#include <xbt/sysdep.h>

#include "src/internal_config.h"
#include "src/mc/mc_forward.hpp"
#include "src/mc/mc_safety.h"
#include "src/mc/mc_liveness.h"
#include "src/mc/mc_private.h"
#include "src/mc/mc_smx.h"
#include "src/mc/mc_dwarf.hpp"
#include "src/mc/malloc.hpp"
#include "src/mc/Frame.hpp"
#include "src/mc/ObjectInformation.hpp"
#include "src/mc/Variable.hpp"

#if HAVE_SMPI
#include "src/smpi/private.h"
#endif

#include "xbt/mmalloc.h"
#include "src/xbt/mmalloc/mmprivate.h"

using simgrid::mc::remote;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_compare, xbt,
                                "Logging specific to mc_compare in mc");

namespace simgrid {
namespace mc {

/** A hash which works with more stuff
 *
 *  It can hash pairs: the standard hash currently doesn't include this.
 */
template<class X> struct hash : public std::hash<X> {};

template<class X, class Y>
struct hash<std::pair<X,Y>> {
  std::size_t operator()(std::pair<X,Y>const& x) const
  {
    struct hash<X> h1;
    struct hash<X> h2;
    return h1(x.first) ^ h2(x.second);
  }
};

struct ComparisonState {
  std::unordered_set<std::pair<void*, void*>, hash<std::pair<void*, void*>>> compared_pointers;
};

}
}

using simgrid::mc::ComparisonState;

extern "C" {

/************************** Snapshot comparison *******************************/
/******************************************************************************/

static int compare_areas_with_type(ComparisonState& state,
                                   int process_index,
                                   void* real_area1, simgrid::mc::Snapshot* snapshot1, mc_mem_region_t region1,
                                   void* real_area2, simgrid::mc::Snapshot* snapshot2, mc_mem_region_t region2,
                                   simgrid::mc::Type* type, int pointer_level)
{
  simgrid::mc::Process* process = &mc_model_checker->process();

  simgrid::mc::Type* subtype;
  simgrid::mc::Type* subsubtype;
  int elm_size, i, res;

  top:
  switch (type->type) {
  case DW_TAG_unspecified_type:
    return 1;

  case DW_TAG_base_type:
  case DW_TAG_enumeration_type:
  case DW_TAG_union_type:
  {
    return MC_snapshot_region_memcmp(
      real_area1, region1, real_area2, region2,
      type->byte_size) != 0;
  }
  case DW_TAG_typedef:
  case DW_TAG_volatile_type:
  case DW_TAG_const_type:
    // Poor man's TCO:
    type = type->subtype;
    goto top;
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
    for (i = 0; i < type->element_count; i++) {
      size_t off = i * elm_size;
      res = compare_areas_with_type(state, process_index,
            (char*) real_area1 + off, snapshot1, region1,
            (char*) real_area2 + off, snapshot2, region2,
            type->subtype, pointer_level);
      if (res == 1)
        return res;
    }
    break;
  case DW_TAG_pointer_type:
  case DW_TAG_reference_type:
  case DW_TAG_rvalue_reference_type:
  {
    void* addr_pointed1 = MC_region_read_pointer(region1, real_area1);
    void* addr_pointed2 = MC_region_read_pointer(region2, real_area2);

    if (type->subtype && type->subtype->type == DW_TAG_subroutine_type)
      return (addr_pointed1 != addr_pointed2);
    if (addr_pointed1 == nullptr && addr_pointed2 == NULL)
      return 0;
    if (addr_pointed1 == nullptr || addr_pointed2 == NULL)
      return 1;
    if (!state.compared_pointers.insert(
        std::make_pair(addr_pointed1, addr_pointed2)).second)
      return 0;

    pointer_level++;

      // Some cases are not handled here:
      // * the pointers lead to different areas (one to the heap, the other to the RW segment ...);
      // * a pointer leads to the read-only segment of the current object;
      // * a pointer lead to a different ELF object.

      if (addr_pointed1 > process->heap_address
          && addr_pointed1 < mc_snapshot_get_heap_end(snapshot1)) {
        if (!
            (addr_pointed2 > process->heap_address
             && addr_pointed2 < mc_snapshot_get_heap_end(snapshot2)))
          return 1;
        // The pointers are both in the heap:
        return simgrid::mc::compare_heap_area(process_index, addr_pointed1, addr_pointed2, snapshot1,
                                 snapshot2, nullptr, type->subtype, pointer_level);
      }

      // The pointers are both in the current object R/W segment:
      else if (region1->contain(simgrid::mc::remote(addr_pointed1))) {
        if (!region2->contain(simgrid::mc::remote(addr_pointed2)))
          return 1;
        if (!type->type_id)
          return (addr_pointed1 != addr_pointed2);
        else
          return compare_areas_with_type(state, process_index,
                                         addr_pointed1, snapshot1, region1,
                                         addr_pointed2, snapshot2, region2,
                                         type->subtype, pointer_level);
      }

      // TODO, We do not handle very well the case where
      // it belongs to a different (non-heap) region from the current one.

      else
        return (addr_pointed1 != addr_pointed2);

    break;
  }
  case DW_TAG_structure_type:
  case DW_TAG_class_type:
    for(simgrid::mc::Member& member : type->members) {
      void *member1 = simgrid::dwarf::resolve_member(
        real_area1, type, &member, snapshot1, process_index);
      void *member2 = simgrid::dwarf::resolve_member(
        real_area2, type, &member, snapshot2, process_index);
      mc_mem_region_t subregion1 = mc_get_region_hinted(member1, snapshot1, process_index, region1);
      mc_mem_region_t subregion2 = mc_get_region_hinted(member2, snapshot2, process_index, region2);
      res =
          compare_areas_with_type(state, process_index,
                                  member1, snapshot1, subregion1,
                                  member2, snapshot2, subregion2,
                                  member.type, pointer_level);
      if (res == 1)
        return res;
    }
    break;
  case DW_TAG_subroutine_type:
    return -1;
    break;
  default:
    XBT_VERB("Unknown case : %d", type->type);
    break;
  }

  return 0;
}

static int compare_global_variables(simgrid::mc::ObjectInformation* object_info,
                                    int process_index,
                                    mc_mem_region_t r1,
                                    mc_mem_region_t r2, simgrid::mc::Snapshot* snapshot1,
                                    simgrid::mc::Snapshot* snapshot2)
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
      int is_diff = compare_global_variables(object_info, process_index,
        &r1->privatized_data()[process_index],
        &r2->privatized_data()[process_index],
        snapshot1, snapshot2);
      if (is_diff) return 1;
    }
    return 0;
  }
#else
  xbt_assert(r1->storage_type() != simgrid::mc::StorageType::Privatized);
#endif
  xbt_assert(r2->storage_type() != simgrid::mc::StorageType::Privatized);

  ComparisonState state;

  std::vector<simgrid::mc::Variable>& variables = object_info->global_variables;

  for (simgrid::mc::Variable& current_var : variables) {

    // If the variable is not in this object, skip it:
    // We do not expect to find a pointer to something which is not reachable
    // by the global variables.
    if ((char *) current_var.address < (char *) object_info->start_rw
        || (char *) current_var.address > (char *) object_info->end_rw)
      continue;

    simgrid::mc::Type* bvariable_type = current_var.type;
    int res =
        compare_areas_with_type(state, process_index,
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

static int compare_local_variables(int process_index,
                                   simgrid::mc::Snapshot* snapshot1,
                                   simgrid::mc::Snapshot* snapshot2,
                                   mc_snapshot_stack_t stack1,
                                   mc_snapshot_stack_t stack2)
{
  ComparisonState state;

  if (stack1->local_variables.size() != stack2->local_variables.size()) {
    XBT_VERB("Different number of local variables");
    return 1;
  }

    unsigned int cursor = 0;
    local_variable_t current_var1, current_var2;
    int res;
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
        res =
            compare_areas_with_type(state, process_index,
                                    current_var1->address, snapshot1, mc_get_snapshot_region(current_var1->address, snapshot1, process_index),
                                    current_var2->address, snapshot2, mc_get_snapshot_region(current_var2->address, snapshot2, process_index),
                                    subtype, 0);

      if (res == 1) {
        // TODO, fix current_varX->subprogram->name to include name if DW_TAG_inlined_subprogram
        XBT_VERB
            ("Local variable %s (%p - %p) in frame %s "
             "is different between snapshots",
             current_var1->name.c_str(),
             current_var1->address,
             current_var2->address,
             current_var1->subprogram->name.c_str());
        return res;
      }
      cursor++;
    }
    return 0;
}

int snapshot_compare(void *state1, void *state2)
{
  simgrid::mc::Process* process = &mc_model_checker->process();

  simgrid::mc::Snapshot* s1;
  simgrid::mc::Snapshot* s2;
  int num1, num2;

  if (_sg_mc_liveness) {        /* Liveness MC */
    s1 = ((simgrid::mc::VisitedPair*) state1)->graph_state->system_state;
    s2 = ((simgrid::mc::VisitedPair*) state2)->graph_state->system_state;
    num1 = ((simgrid::mc::VisitedPair*) state1)->num;
    num2 = ((simgrid::mc::VisitedPair*) state2)->num;
  }else if (_sg_mc_termination) { /* Non-progressive cycle MC */
    s1 = ((mc_state_t) state1)->system_state;
    s2 = ((mc_state_t) state2)->system_state;
    num1 = ((mc_state_t) state1)->num;
    num2 = ((mc_state_t) state2)->num;
  } else {                      /* Safety or comm determinism MC */
    s1 = ((simgrid::mc::VisitedState*) state1)->system_state;
    s2 = ((simgrid::mc::VisitedState*) state2)->system_state;
    num1 = ((simgrid::mc::VisitedState*) state1)->num;
    num2 = ((simgrid::mc::VisitedState*) state2)->num;
  }

  int errors = 0;
  int res_init;

  int hash_result = 0;
  if (_sg_mc_hash) {
    hash_result = (s1->hash != s2->hash);
    if (hash_result) {
      XBT_VERB("(%d - %d) Different hash : 0x%" PRIx64 "--0x%" PRIx64, num1,
               num2, s1->hash, s2->hash);
#ifndef MC_DEBUG
      return 1;
#endif
    } else
      XBT_VERB("(%d - %d) Same hash : 0x%" PRIx64, num1, num2, s1->hash);
  }

  /* Compare enabled processes */
  if (s1->enabled_processes != s2->enabled_processes) {
      XBT_VERB("(%d - %d) Different enabled processes", num1, num2);
      // return 1; ??
  }

  unsigned long i = 0;
  size_t size_used1, size_used2;
  int is_diff = 0;

  /* Compare size of stacks */
  while (i < s1->stacks.size()) {
    size_used1 = s1->stack_sizes[i];
    size_used2 = s2->stack_sizes[i];
    if (size_used1 != size_used2) {
#ifdef MC_DEBUG
      XBT_DEBUG("(%d - %d) Different size used in stacks : %zu - %zu", num1,
                num2, size_used1, size_used2);
      errors++;
      is_diff = 1;
#else
#ifdef MC_VERBOSE
      XBT_VERB("(%d - %d) Different size used in stacks : %zu - %zu", num1,
               num2, size_used1, size_used2);
#endif
      return 1;
#endif
    }
    i++;
  }

  /* Init heap information used in heap comparison algorithm */
  xbt_mheap_t heap1 = (xbt_mheap_t)s1->read_bytes(
    alloca(sizeof(struct mdesc)), sizeof(struct mdesc),
    remote(process->heap_address),
    simgrid::mc::ProcessIndexMissing, simgrid::mc::ReadOptions::lazy());
  xbt_mheap_t heap2 = (xbt_mheap_t)s2->read_bytes(
    alloca(sizeof(struct mdesc)), sizeof(struct mdesc),
    remote(process->heap_address),
    simgrid::mc::ProcessIndexMissing, simgrid::mc::ReadOptions::lazy());
  res_init = simgrid::mc::init_heap_information(heap1, heap2, &s1->to_ignore, &s2->to_ignore);
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
  unsigned cursor = 0;
  int diff_local = 0;
#ifdef MC_DEBUG
  is_diff = 0;
#endif
  mc_snapshot_stack_t stack1, stack2;
  while (cursor < s1->stacks.size()) {
    stack1 = &s1->stacks[cursor];
    stack2 = &s2->stacks[cursor];

    if (stack1->process_index != stack2->process_index) {
      diff_local = 1;
      XBT_DEBUG("(%d - %d) Stacks with different process index (%i vs %i)", num1, num2,
        stack1->process_index, stack2->process_index);
    }
    else diff_local =
        compare_local_variables(stack1->process_index, s1, s2, stack1, stack2);
    if (diff_local > 0) {
#ifdef MC_DEBUG
      XBT_DEBUG("(%d - %d) Different local variables between stacks %d", num1,
                num2, cursor + 1);
      errors++;
      is_diff = 1;
#else

#ifdef MC_VERBOSE
      XBT_VERB("(%d - %d) Different local variables between stacks %d", num1,
               num2, cursor + 1);
#endif

      simgrid::mc::reset_heap_information();

      return 1;
#endif
    }
    cursor++;
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
    is_diff =
      compare_global_variables(region1->object_info(),
        simgrid::mc::ProcessIndexDisabled,
        region1, region2,
        s1, s2);

    if (is_diff != 0) {
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
  if (simgrid::mc::mmalloc_compare_heap(s1, s2) > 0) {

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

  simgrid::mc::reset_heap_information();

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
             (hash_result != 0) == (errors != 0) ? "true" : "false",
             !hash_result ? "positive" : "negative");
  }
#endif

  return errors > 0 || hash_result;

}

}

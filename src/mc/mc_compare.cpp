/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <inttypes.h>
#include <boost/unordered_set.hpp>

#include "mc_private.h"

#include "xbt/mmalloc.h"
#include "xbt/mmalloc/mmprivate.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_compare, mc,
                                "Logging specific to mc_compare");

typedef struct s_pointers_pair {
  void *p1;
  void *p2;
  bool operator==(s_pointers_pair const& x) const {
    return this->p1 == x.p1 && this->p2 == x.p2;
  }
  bool operator<(s_pointers_pair const& x) const {
    return this->p1 < x.p1 || (this->p1 == x.p1 && this->p2 < x.p2);
  }
} s_pointers_pair_t, *pointers_pair_t;

namespace boost {
  template<>
  struct hash<s_pointers_pair> {
    typedef uintptr_t result_type;
    result_type operator()(s_pointers_pair const& x) const {
      return (result_type) x.p1 ^
        ((result_type) x.p2 << 8 | (result_type) x.p2 >> (8*sizeof(uintptr_t) - 8));
    }
  };
}

struct mc_compare_state {
  boost::unordered_set<s_pointers_pair> compared_pointers;
};

extern "C" {

/************************** Free functions ****************************/
/********************************************************************/

static void stack_region_free(stack_region_t s)
{
  if (s) {
    xbt_free(s->process_name);
    xbt_free(s);
  }
}

static void stack_region_free_voidp(void *s)
{
  stack_region_free((stack_region_t) * (void **) s);
}

static void pointers_pair_free(pointers_pair_t p)
{
  xbt_free(p);
}

static void pointers_pair_free_voidp(void *p)
{
  pointers_pair_free((pointers_pair_t) * (void **) p);
}

/************************** Snapshot comparison *******************************/
/******************************************************************************/

/** \brief Try to add a pair a compared pointers to the set of compared pointers
 *
 *  \result !=0 if the pointers were added (they were not in the set),
 *      0 otherwise (they were already in the set)
 */
static int add_compared_pointers(mc_compare_state& state, void *p1, void *p2)
{
  s_pointers_pair_t new_pair;
  new_pair.p1 = p1;
  new_pair.p2 = p2;
  return state.compared_pointers.insert(new_pair).second ? 1 : 0;
}

static int compare_areas_with_type(struct mc_compare_state& state,
                                   void* real_area1, mc_snapshot_t snapshot1, mc_mem_region_t region1,
                                   void* real_area2, mc_snapshot_t snapshot2, mc_mem_region_t region2,
                                   dw_type_t type, int pointer_level)
{
  unsigned int cursor = 0;
  dw_type_t member, subtype, subsubtype;
  int elm_size, i, res;

  top:
  switch (type->type) {
  case DW_TAG_unspecified_type:
    return 1;

  case DW_TAG_base_type:
  case DW_TAG_enumeration_type:
  case DW_TAG_union_type:
  {
    return mc_snapshot_region_memcmp(
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
      res = compare_areas_with_type(state,
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
    void* addr_pointed1 = mc_snapshot_read_pointer_region(real_area1, region1);
    void* addr_pointed2 = mc_snapshot_read_pointer_region(real_area2, region2);

    if (type->subtype && type->subtype->type == DW_TAG_subroutine_type) {
      return (addr_pointed1 != addr_pointed2);
    } else {

      if (addr_pointed1 == NULL && addr_pointed2 == NULL)
        return 0;
      if (!add_compared_pointers(state, addr_pointed1, addr_pointed2))
        return 0;

      pointer_level++;

      // Some cases are not handled here:
      // * the pointers lead to different areas (one to the heap, the other to the RW segment ...);
      // * a pointer leads to the read-only segment of the current object;
      // * a pointer lead to a different ELF object.

      if (addr_pointed1 > std_heap
          && addr_pointed1 < mc_snapshot_get_heap_end(snapshot1)) {
        if (!
            (addr_pointed2 > std_heap
             && addr_pointed2 < mc_snapshot_get_heap_end(snapshot2)))
          return 1;
        // The pointers are both in the heap:
        return compare_heap_area(addr_pointed1, addr_pointed2, snapshot1,
                                 snapshot2, NULL, type->subtype, pointer_level);
      }

      // The pointers are both in the current object R/W segment:
      else if (mc_region_contain(region1, addr_pointed1)) {
        if (!mc_region_contain(region2, addr_pointed2))
          return 1;
        if (type->dw_type_id == NULL)
          return (addr_pointed1 != addr_pointed2);
        else {
          return compare_areas_with_type(state,
                                         addr_pointed1, snapshot1, region1,
                                         addr_pointed2, snapshot2, region2,
                                         type->subtype, pointer_level);
        }
      }

      // TODO, We do not handle very well the case where
      // it belongs to a different (non-heap) region from the current one.

      else {
        return (addr_pointed1 != addr_pointed2);
      }
    }
    break;
  }
  case DW_TAG_structure_type:
  case DW_TAG_class_type:
    xbt_dynar_foreach(type->members, cursor, member) {
      void *member1 =
        mc_member_resolve(real_area1, type, member, snapshot1);
      void *member2 =
        mc_member_resolve(real_area2, type, member, snapshot2);
      mc_mem_region_t subregion1 = mc_get_region_hinted(member1, snapshot1, region1);
      mc_mem_region_t subregion2 = mc_get_region_hinted(member2, snapshot2, region2);
      res =
          compare_areas_with_type(state,
                                  member1, snapshot1, subregion1,
                                  member2, snapshot2, subregion2,
                                  member->subtype, pointer_level);
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

static int compare_global_variables(int region_type, mc_mem_region_t r1,
                                    mc_mem_region_t r2, mc_snapshot_t snapshot1,
                                    mc_snapshot_t snapshot2)
{
  xbt_assert(r1 && r2,
    "Missing region. Did you enable SMPI privatisation? It is not compatible with state comparison.");
  struct mc_compare_state state;

  xbt_dynar_t variables;
  int res;
  unsigned int cursor = 0;
  dw_variable_t current_var;

  mc_object_info_t object_info = NULL;
  if (region_type == 2) {
    object_info = mc_binary_info;
  } else {
    object_info = mc_libsimgrid_info;
  }
  variables = object_info->global_variables;

  xbt_dynar_foreach(variables, cursor, current_var) {

    // If the variable is not in this object, skip it:
    // We do not expect to find a pointer to something which is not reachable
    // by the global variables.
    if ((char *) current_var->address < (char *) object_info->start_rw
        || (char *) current_var->address > (char *) object_info->end_rw)
      continue;

    dw_type_t bvariable_type = current_var->type;
    res =
        compare_areas_with_type(state,
                                (char *) current_var->address, snapshot1, r1,
                                (char *) current_var->address, snapshot2, r2,
                                bvariable_type, 0);
    if (res == 1) {
      XBT_VERB("Global variable %s (%p) is different between snapshots",
               current_var->name, (char *) current_var->address);
      return 1;
    }

  }

  return 0;

}

static int compare_local_variables(mc_snapshot_t snapshot1,
                                   mc_snapshot_t snapshot2,
                                   mc_snapshot_stack_t stack1,
                                   mc_snapshot_stack_t stack2)
{
  struct mc_compare_state state;

  if (xbt_dynar_length(stack1->local_variables) !=
      xbt_dynar_length(stack2->local_variables)) {
    XBT_VERB("Different number of local variables");
    return 1;
  } else {
    unsigned int cursor = 0;
    local_variable_t current_var1, current_var2;
    int res;
    while (cursor < xbt_dynar_length(stack1->local_variables)) {
      current_var1 =
          (local_variable_t) xbt_dynar_get_as(stack1->local_variables, cursor,
                                              local_variable_t);
      current_var2 =
          (local_variable_t) xbt_dynar_get_as(stack2->local_variables, cursor,
                                              local_variable_t);
      if (strcmp(current_var1->name, current_var2->name) != 0
          || current_var1->subprogram != current_var1->subprogram
          || current_var1->ip != current_var2->ip) {
        // TODO, fix current_varX->subprogram->name to include name if DW_TAG_inlined_subprogram
        XBT_VERB
            ("Different name of variable (%s - %s) or frame (%s - %s) or ip (%lu - %lu)",
             current_var1->name, current_var2->name,
             current_var1->subprogram->name, current_var2->subprogram->name,
             current_var1->ip, current_var2->ip);
        return 1;
      }
      // TODO, fix current_varX->subprogram->name to include name if DW_TAG_inlined_subprogram

        dw_type_t subtype = current_var1->type;
        res =
            compare_areas_with_type(state,
                                    current_var1->address, snapshot1, mc_get_snapshot_region(current_var1->address, snapshot1),
                                    current_var2->address, snapshot2, mc_get_snapshot_region(current_var2->address, snapshot2),
                                    subtype, 0);

      if (res == 1) {
        // TODO, fix current_varX->subprogram->name to include name if DW_TAG_inlined_subprogram
        XBT_VERB
            ("Local variable %s (%p - %p) in frame %s  is different between snapshots",
             current_var1->name, current_var1->address, current_var2->address,
             current_var1->subprogram->name);
        return res;
      }
      cursor++;
    }
    return 0;
  }
}

int snapshot_compare(void *state1, void *state2)
{

  mc_snapshot_t s1, s2;
  int num1, num2;

  if (_sg_mc_liveness) {        /* Liveness MC */
    s1 = ((mc_visited_pair_t) state1)->graph_state->system_state;
    s2 = ((mc_visited_pair_t) state2)->graph_state->system_state;
    num1 = ((mc_visited_pair_t) state1)->num;
    num2 = ((mc_visited_pair_t) state2)->num;
  } else {                      /* Safety or comm determinism MC */
    s1 = ((mc_visited_state_t) state1)->system_state;
    s2 = ((mc_visited_state_t) state2)->system_state;
    num1 = ((mc_visited_state_t) state1)->num;
    num2 = ((mc_visited_state_t) state2)->num;
  }

  int errors = 0;
  int res_init;

  xbt_os_timer_t global_timer = xbt_os_timer_new();
  xbt_os_timer_t timer = xbt_os_timer_new();

  xbt_os_walltimer_start(global_timer);

#ifdef MC_DEBUG
  xbt_os_walltimer_start(timer);
#endif

  int hash_result = 0;
  if (_sg_mc_hash) {
    hash_result = (s1->hash != s2->hash);
    if (hash_result) {
      XBT_VERB("(%d - %d) Different hash : 0x%" PRIx64 "--0x%" PRIx64, num1,
               num2, s1->hash, s2->hash);
#ifndef MC_DEBUG
      return 1;
#endif
    } else {
      XBT_VERB("(%d - %d) Same hash : 0x%" PRIx64, num1, num2, s1->hash);
    }
  }

  /* Compare enabled processes */
  unsigned int cursor;
  int pid;
  xbt_dynar_foreach(s1->enabled_processes, cursor, pid){
    if(!xbt_dynar_member(s2->enabled_processes, &pid))
      XBT_VERB("(%d - %d) Different enabled processes", num1, num2);
  }

  unsigned long i = 0;
  size_t size_used1, size_used2;
  int is_diff = 0;

  /* Compare size of stacks */
  while (i < xbt_dynar_length(s1->stacks)) {
    size_used1 = s1->stack_sizes[i];
    size_used2 = s2->stack_sizes[i];
    if (size_used1 != size_used2) {
#ifdef MC_DEBUG
      if (is_diff == 0) {
        xbt_os_walltimer_stop(timer);
        mc_comp_times->stacks_sizes_comparison_time =
            xbt_os_timer_elapsed(timer);
      }
      XBT_DEBUG("(%d - %d) Different size used in stacks : %zu - %zu", num1,
                num2, size_used1, size_used2);
      errors++;
      is_diff = 1;
#else
#ifdef MC_VERBOSE
      XBT_VERB("(%d - %d) Different size used in stacks : %zu - %zu", num1,
               num2, size_used1, size_used2);
#endif

      xbt_os_walltimer_stop(timer);
      xbt_os_timer_free(timer);
      xbt_os_walltimer_stop(global_timer);
      mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
      xbt_os_timer_free(global_timer);

      return 1;
#endif
    }
    i++;
  }

#ifdef MC_DEBUG
  if (is_diff == 0)
    xbt_os_walltimer_stop(timer);
  xbt_os_walltimer_start(timer);
#endif

  /* Init heap information used in heap comparison algorithm */
  xbt_mheap_t heap1 = (xbt_mheap_t) mc_snapshot_read(std_heap, s1,
    alloca(sizeof(struct mdesc)), sizeof(struct mdesc));
  xbt_mheap_t heap2 = (xbt_mheap_t) mc_snapshot_read(std_heap, s2,
    alloca(sizeof(struct mdesc)), sizeof(struct mdesc));
  res_init = init_heap_information(heap1, heap2, s1->to_ignore, s2->to_ignore);
  if (res_init == -1) {
#ifdef MC_DEBUG
    XBT_DEBUG("(%d - %d) Different heap information", num1, num2);
    errors++;
#else
#ifdef MC_VERBOSE
    XBT_VERB("(%d - %d) Different heap information", num1, num2);
#endif

    xbt_os_walltimer_stop(global_timer);
    mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
    xbt_os_timer_free(global_timer);

    return 1;
#endif
  }
#ifdef MC_DEBUG
  xbt_os_walltimer_start(timer);
#endif

  /* Stacks comparison */
  cursor = 0;
  int diff_local = 0;
#ifdef MC_DEBUG
  is_diff = 0;
#endif
  mc_snapshot_stack_t stack1, stack2;

  while (cursor < xbt_dynar_length(s1->stacks)) {
    stack1 =
        (mc_snapshot_stack_t) xbt_dynar_get_as(s1->stacks, cursor,
                                               mc_snapshot_stack_t);
    stack2 =
        (mc_snapshot_stack_t) xbt_dynar_get_as(s2->stacks, cursor,
                                               mc_snapshot_stack_t);
    diff_local =
        compare_local_variables(s1, s2, stack1, stack2);
    if (diff_local > 0) {
#ifdef MC_DEBUG
      if (is_diff == 0) {
        xbt_os_walltimer_stop(timer);
        mc_comp_times->stacks_comparison_time = xbt_os_timer_elapsed(timer);
      }
      XBT_DEBUG("(%d - %d) Different local variables between stacks %d", num1,
                num2, cursor + 1);
      errors++;
      is_diff = 1;
#else

#ifdef MC_VERBOSE
      XBT_VERB("(%d - %d) Different local variables between stacks %d", num1,
               num2, cursor + 1);
#endif

      reset_heap_information();
      xbt_os_walltimer_stop(timer);
      xbt_os_timer_free(timer);
      xbt_os_walltimer_stop(global_timer);
      mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
      xbt_os_timer_free(global_timer);

      return 1;
#endif
    }
    cursor++;
  }



  const char *names[3] = { "?", "libsimgrid", "binary" };
#ifdef MC_DEBUG
  double *times[3] = {
    NULL,
    &mc_comp_times->libsimgrid_global_variables_comparison_time,
    &mc_comp_times->binary_global_variables_comparison_time
  };
#endif

  int k = 0;
  for (k = 2; k != 0; --k) {
#ifdef MC_DEBUG
    if (is_diff == 0)
      xbt_os_walltimer_stop(timer);
    xbt_os_walltimer_start(timer);
#endif

    /* Compare global variables */
    is_diff =
        compare_global_variables(k, s1->regions[k], s2->regions[k], s1, s2);
    if (is_diff != 0) {
#ifdef MC_DEBUG
      xbt_os_walltimer_stop(timer);
      *times[k] = xbt_os_timer_elapsed(timer);
      XBT_DEBUG("(%d - %d) Different global variables in %s", num1, num2,
                names[k]);
      errors++;
#else
#ifdef MC_VERBOSE
      XBT_VERB("(%d - %d) Different global variables in %s", num1, num2,
               names[k]);
#endif

      reset_heap_information();
      xbt_os_walltimer_stop(timer);
      xbt_os_timer_free(timer);
      xbt_os_walltimer_stop(global_timer);
      mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
      xbt_os_timer_free(global_timer);

      return 1;
#endif
    }
  }

#ifdef MC_DEBUG
  xbt_os_walltimer_start(timer);
#endif

  /* Compare heap */
  if (mmalloc_compare_heap(s1, s2) > 0) {

#ifdef MC_DEBUG
    xbt_os_walltimer_stop(timer);
    mc_comp_times->heap_comparison_time = xbt_os_timer_elapsed(timer);
    XBT_DEBUG("(%d - %d) Different heap (mmalloc_compare)", num1, num2);
    errors++;
#else

#ifdef MC_VERBOSE
    XBT_VERB("(%d - %d) Different heap (mmalloc_compare)", num1, num2);
#endif

    reset_heap_information();
    xbt_os_walltimer_stop(timer);
    xbt_os_timer_free(timer);
    xbt_os_walltimer_stop(global_timer);
    mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
    xbt_os_timer_free(global_timer);

    return 1;
#endif
  } else {
#ifdef MC_DEBUG
    xbt_os_walltimer_stop(timer);
#endif
  }

  reset_heap_information();

  xbt_os_walltimer_stop(timer);
  xbt_os_timer_free(timer);

#ifdef MC_VERBOSE
  xbt_os_walltimer_stop(global_timer);
  mc_snapshot_comparison_time = xbt_os_timer_elapsed(global_timer);
#endif

  xbt_os_timer_free(global_timer);

#ifdef MC_DEBUG
  print_comparison_times();
#endif

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

/***************************** Statistics *****************************/
/*******************************************************************/

void print_comparison_times()
{
  XBT_DEBUG("*** Comparison times ***");
  XBT_DEBUG("- Nb processes : %f", mc_comp_times->nb_processes_comparison_time);
  XBT_DEBUG("- Nb bytes used : %f", mc_comp_times->bytes_used_comparison_time);
  XBT_DEBUG("- Stacks sizes : %f", mc_comp_times->stacks_sizes_comparison_time);
  XBT_DEBUG("- Binary global variables : %f",
            mc_comp_times->binary_global_variables_comparison_time);
  XBT_DEBUG("- Libsimgrid global variables : %f",
            mc_comp_times->libsimgrid_global_variables_comparison_time);
  XBT_DEBUG("- Heap : %f", mc_comp_times->heap_comparison_time);
  XBT_DEBUG("- Stacks : %f", mc_comp_times->stacks_comparison_time);
}

/**************************** MC snapshot compare simcall **************************/
/***********************************************************************************/

int SIMIX_pre_mc_compare_snapshots(smx_simcall_t simcall,
                                   mc_snapshot_t s1, mc_snapshot_t s2)
{
  return snapshot_compare(s1, s2);
}

int MC_compare_snapshots(void *s1, void *s2)
{

  return simcall_mc_compare_snapshots(s1, s2);

}

}

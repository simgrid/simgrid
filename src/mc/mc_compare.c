/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <inttypes.h>

#include "mc_private.h"

#include "xbt/mmalloc.h"
#include "xbt/mmalloc/mmprivate.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_compare, mc,
                                "Logging specific to mc_compare");

typedef struct s_pointers_pair {
  void *p1;
  void *p2;
} s_pointers_pair_t, *pointers_pair_t;

__thread xbt_dynar_t compared_pointers;

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
static int add_compared_pointers(void *p1, void *p2)
{

  pointers_pair_t new_pair = xbt_new0(s_pointers_pair_t, 1);
  new_pair->p1 = p1;
  new_pair->p2 = p2;

  if (xbt_dynar_is_empty(compared_pointers)) {
    xbt_dynar_push(compared_pointers, &new_pair);
    return 1;
  }

  unsigned int cursor = 0;
  int start = 0;
  int end = xbt_dynar_length(compared_pointers) - 1;
  pointers_pair_t pair = NULL;

  pointers_pair_t *p =
      (pointers_pair_t *) xbt_dynar_get_ptr(compared_pointers, 0);

  while (start <= end) {
    cursor = (start + end) / 2;
    pair = p[cursor];
    if (pair->p1 < p1) {
      start = cursor + 1;
    } else if (pair->p1 > p1) {
      end = cursor - 1;
    } else if (pair->p2 < p2) {
      start = cursor + 1;
    } else if (pair->p2 > p2) {
      end = cursor - 1;
    } else {
      pointers_pair_free(new_pair);
      return 0;
    }
  }

  if (pair->p1 < p1)
    xbt_dynar_insert_at(compared_pointers, cursor + 1, &new_pair);
  else if (pair->p1 > p1)
    xbt_dynar_insert_at(compared_pointers, cursor, &new_pair);
  else if (pair->p2 < p2)
    xbt_dynar_insert_at(compared_pointers, cursor + 1, &new_pair);
  else if (pair->p2 > p2)
    xbt_dynar_insert_at(compared_pointers, cursor, &new_pair);
  else
    xbt_die("Unrecheable");

  return 1;
}

static int compare_areas_with_type(void *area1, void *area2,
                                   mc_snapshot_t snapshot1,
                                   mc_snapshot_t snapshot2, dw_type_t type,
                                   int region_size, int region_type,
                                   void *start_data, int pointer_level)
{

  unsigned int cursor = 0;
  dw_type_t member, subtype, subsubtype;
  int elm_size, i, res;
  void *addr_pointed1, *addr_pointed2;

  switch (type->type) {
  case DW_TAG_unspecified_type:
    return 1;

  case DW_TAG_base_type:
  case DW_TAG_enumeration_type:
  case DW_TAG_union_type:
    return (memcmp(area1, area2, type->byte_size) != 0);
    break;
  case DW_TAG_typedef:
  case DW_TAG_volatile_type:
  case DW_TAG_const_type:
    return compare_areas_with_type(area1, area2, snapshot1, snapshot2,
                                   type->subtype, region_size, region_type,
                                   start_data, pointer_level);
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
      res =
          compare_areas_with_type((char *) area1 + (i * elm_size),
                                  (char *) area2 + (i * elm_size), snapshot1,
                                  snapshot2, type->subtype, region_size,
                                  region_type, start_data, pointer_level);
      if (res == 1)
        return res;
    }
    break;
  case DW_TAG_pointer_type:
  case DW_TAG_reference_type:
  case DW_TAG_rvalue_reference_type:

    addr_pointed1 = *((void **) (area1));
    addr_pointed2 = *((void **) (area2));

    if (type->subtype && type->subtype->type == DW_TAG_subroutine_type) {
      return (addr_pointed1 != addr_pointed2);
    } else {

      if (addr_pointed1 == NULL && addr_pointed2 == NULL)
        return 0;
      if (!add_compared_pointers(addr_pointed1, addr_pointed2))
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
      else if (addr_pointed1 > start_data
               && (char *) addr_pointed1 <= (char *) start_data + region_size) {
        if (!
            (addr_pointed2 > start_data
             && (char *) addr_pointed2 <= (char *) start_data + region_size))
          return 1;
        if (type->dw_type_id == NULL)
          return (addr_pointed1 != addr_pointed2);
        else {
          void *translated_addr_pointer1 =
              mc_translate_address((uintptr_t) addr_pointed1, snapshot1);
          void *translated_addr_pointer2 =
              mc_translate_address((uintptr_t) addr_pointed2, snapshot2);
          return compare_areas_with_type(translated_addr_pointer1,
                                         translated_addr_pointer2, snapshot1,
                                         snapshot2, type->subtype, region_size,
                                         region_type, start_data,
                                         pointer_level);
        }
      }

      else {
        return (addr_pointed1 != addr_pointed2);
      }
    }
    break;
  case DW_TAG_structure_type:
  case DW_TAG_class_type:
    xbt_dynar_foreach(type->members, cursor, member) {
      void *member1 =
          mc_member_snapshot_resolve(area1, type, member, snapshot1);
      void *member2 =
          mc_member_snapshot_resolve(area2, type, member, snapshot2);
      res =
          compare_areas_with_type(member1, member2, snapshot1, snapshot2,
                                  member->subtype, region_size, region_type,
                                  start_data, pointer_level);
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

  if (!compared_pointers) {
    compared_pointers =
        xbt_dynar_new(sizeof(pointers_pair_t), pointers_pair_free_voidp);
  } else {
    xbt_dynar_reset(compared_pointers);
  }

  xbt_dynar_t variables;
  int res;
  unsigned int cursor = 0;
  dw_variable_t current_var;
  size_t offset;
  void *start_data;
  void *start_data_binary = mc_binary_info->start_rw;
  void *start_data_libsimgrid = mc_libsimgrid_info->start_rw;

  mc_object_info_t object_info = NULL;
  if (region_type == 2) {
    object_info = mc_binary_info;
    start_data = start_data_binary;
  } else {
    object_info = mc_libsimgrid_info;
    start_data = start_data_libsimgrid;
  }
  variables = object_info->global_variables;

  xbt_dynar_foreach(variables, cursor, current_var) {

    // If the variable is not in this object, skip it:
    // We do not expect to find a pointer to something which is not reachable
    // by the global variables.
    if ((char *) current_var->address < (char *) object_info->start_rw
        || (char *) current_var->address > (char *) object_info->end_rw)
      continue;

    offset = (char *) current_var->address - (char *) object_info->start_rw;

    dw_type_t bvariable_type = current_var->type;
    res =
        compare_areas_with_type((char *) r1->data + offset,
                                (char *) r2->data + offset, snapshot1,
                                snapshot2, bvariable_type, r1->size,
                                region_type, start_data, 0);
    if (res == 1) {
      XBT_VERB("Global variable %s (%p - %p) is different between snapshots",
               current_var->name, (char *) r1->data + offset,
               (char *) r2->data + offset);
      xbt_dynar_free(&compared_pointers);
      compared_pointers = NULL;
      return 1;
    }

  }

  xbt_dynar_free(&compared_pointers);
  compared_pointers = NULL;

  return 0;

}

static int compare_local_variables(mc_snapshot_t snapshot1,
                                   mc_snapshot_t snapshot2,
                                   mc_snapshot_stack_t stack1,
                                   mc_snapshot_stack_t stack2, void *heap1,
                                   void *heap2)
{
  void *start_data_binary = mc_binary_info->start_rw;
  void *start_data_libsimgrid = mc_libsimgrid_info->start_rw;

  if (!compared_pointers) {
    compared_pointers =
        xbt_dynar_new(sizeof(pointers_pair_t), pointers_pair_free_voidp);
  } else {
    xbt_dynar_reset(compared_pointers);
  }

  if (xbt_dynar_length(stack1->local_variables) !=
      xbt_dynar_length(stack2->local_variables)) {
    XBT_VERB("Different number of local variables");
    xbt_dynar_free(&compared_pointers);
    compared_pointers = NULL;
    return 1;
  } else {
    unsigned int cursor = 0;
    local_variable_t current_var1, current_var2;
    int offset1, offset2, res;
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
        xbt_dynar_free(&compared_pointers);
        // TODO, fix current_varX->subprogram->name to include name if DW_TAG_inlined_subprogram
        XBT_VERB
            ("Different name of variable (%s - %s) or frame (%s - %s) or ip (%lu - %lu)",
             current_var1->name, current_var2->name,
             current_var1->subprogram->name, current_var2->subprogram->name,
             current_var1->ip, current_var2->ip);
        return 1;
      }
      offset1 = (char *) current_var1->address - (char *) std_heap;
      offset2 = (char *) current_var2->address - (char *) std_heap;
      // TODO, fix current_varX->subprogram->name to include name if DW_TAG_inlined_subprogram

      if (current_var1->region == 1) {
        dw_type_t subtype = current_var1->type;
        res =
            compare_areas_with_type((char *) heap1 + offset1,
                                    (char *) heap2 + offset2, snapshot1,
                                    snapshot2, subtype, 0, 1,
                                    start_data_libsimgrid, 0);
      } else {
        dw_type_t subtype = current_var2->type;
        res =
            compare_areas_with_type((char *) heap1 + offset1,
                                    (char *) heap2 + offset2, snapshot1,
                                    snapshot2, subtype, 0, 2, start_data_binary,
                                    0);
      }
      if (res == 1) {
        // TODO, fix current_varX->subprogram->name to include name if DW_TAG_inlined_subprogram
        XBT_VERB
            ("Local variable %s (%p - %p) in frame %s  is different between snapshots",
             current_var1->name, (char *) heap1 + offset1,
             (char *) heap2 + offset2, current_var1->subprogram->name);
        xbt_dynar_free(&compared_pointers);
        compared_pointers = NULL;
        return res;
      }
      cursor++;
    }
    xbt_dynar_free(&compared_pointers);
    compared_pointers = NULL;
    return 0;
  }
}

int snapshot_compare(void *state1, void *state2)
{

  mc_snapshot_t s1, s2;
  int num1, num2;

  if (_sg_mc_property_file && _sg_mc_property_file[0] != '\0') {        /* Liveness MC */
    s1 = ((mc_visited_pair_t) state1)->graph_state->system_state;
    s2 = ((mc_visited_pair_t) state2)->graph_state->system_state;
    num1 = ((mc_visited_pair_t) state1)->num;
    num2 = ((mc_visited_pair_t) state2)->num;
    /* Firstly compare automaton state */
    /*if(xbt_automaton_state_compare(((mc_pair_t)state1)->automaton_state, ((mc_pair_t)state2)->automaton_state) != 0)
       return 1;
       if(xbt_automaton_propositional_symbols_compare_value(((mc_pair_t)state1)->atomic_propositions, ((mc_pair_t)state2)->atomic_propositions) != 0)
       return 1; */
  } else {                      /* Safety MC */
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

  int i = 0;
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
  res_init =
      init_heap_information((xbt_mheap_t) s1->regions[0]->data,
                            (xbt_mheap_t) s2->regions[0]->data, s1->to_ignore,
                            s2->to_ignore);
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
        compare_local_variables(s1, s2, stack1, stack2, s1->regions[0]->data,
                                s2->regions[0]->data);
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
  if (mmalloc_compare_heap(s1, s2, (xbt_mheap_t) s1->regions[0]->data,
                           (xbt_mheap_t) s2->regions[0]->data) > 0) {

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

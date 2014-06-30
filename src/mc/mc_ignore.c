/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "internal_config.h"
#include "mc_private.h"
#include "smpi/private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_ignore, mc,
                                "Logging specific to MC ignore mechanism");


/**************************** Global variables ******************************/
xbt_dynar_t mc_checkpoint_ignore;
extern xbt_dynar_t mc_heap_comparison_ignore;
extern xbt_dynar_t stacks_areas;

/**************************** Structures ******************************/
typedef struct s_mc_stack_ignore_variable {
  char *var_name;
  char *frame;
} s_mc_stack_ignore_variable_t, *mc_stack_ignore_variable_t;

/**************************** Free functions ******************************/

static void stack_ignore_variable_free(mc_stack_ignore_variable_t v)
{
  xbt_free(v->var_name);
  xbt_free(v->frame);
  xbt_free(v);
}

static void stack_ignore_variable_free_voidp(void *v)
{
  stack_ignore_variable_free((mc_stack_ignore_variable_t) * (void **) v);
}

void heap_ignore_region_free(mc_heap_ignore_region_t r)
{
  xbt_free(r);
}

void heap_ignore_region_free_voidp(void *r)
{
  heap_ignore_region_free((mc_heap_ignore_region_t) * (void **) r);
}

static void checkpoint_ignore_region_free(mc_checkpoint_ignore_region_t r)
{
  xbt_free(r);
}

static void checkpoint_ignore_region_free_voidp(void *r)
{
  checkpoint_ignore_region_free((mc_checkpoint_ignore_region_t) * (void **) r);
}

/***********************************************************************/

void MC_ignore_heap(void *address, size_t size)
{

  int raw_mem_set = (mmalloc_get_current_heap() == mc_heap);

  MC_SET_MC_HEAP;

  mc_heap_ignore_region_t region = NULL;
  region = xbt_new0(s_mc_heap_ignore_region_t, 1);
  region->address = address;
  region->size = size;

  region->block =
      ((char *) address -
       (char *) ((xbt_mheap_t) std_heap)->heapbase) / BLOCKSIZE + 1;

  if (((xbt_mheap_t) std_heap)->heapinfo[region->block].type == 0) {
    region->fragment = -1;
    ((xbt_mheap_t) std_heap)->heapinfo[region->block].busy_block.ignore++;
  } else {
    region->fragment =
        ((uintptr_t) (ADDR2UINT(address) % (BLOCKSIZE))) >> ((xbt_mheap_t)
                                                             std_heap)->
        heapinfo[region->block].type;
    ((xbt_mheap_t) std_heap)->heapinfo[region->block].busy_frag.ignore[region->
                                                                       fragment]++;
  }

  if (mc_heap_comparison_ignore == NULL) {
    mc_heap_comparison_ignore =
        xbt_dynar_new(sizeof(mc_heap_ignore_region_t),
                      heap_ignore_region_free_voidp);
    xbt_dynar_push(mc_heap_comparison_ignore, &region);
    if (!raw_mem_set)
      MC_SET_STD_HEAP;
    return;
  }

  unsigned int cursor = 0;
  mc_heap_ignore_region_t current_region = NULL;
  int start = 0;
  int end = xbt_dynar_length(mc_heap_comparison_ignore) - 1;

  while (start <= end) {
    cursor = (start + end) / 2;
    current_region =
        (mc_heap_ignore_region_t) xbt_dynar_get_as(mc_heap_comparison_ignore,
                                                   cursor,
                                                   mc_heap_ignore_region_t);
    if (current_region->address == address) {
      heap_ignore_region_free(region);
      if (!raw_mem_set)
        MC_SET_STD_HEAP;
      return;
    } else if (current_region->address < address) {
      start = cursor + 1;
    } else {
      end = cursor - 1;
    }
  }

  if (current_region->address < address)
    xbt_dynar_insert_at(mc_heap_comparison_ignore, cursor + 1, &region);
  else
    xbt_dynar_insert_at(mc_heap_comparison_ignore, cursor, &region);

  if (!raw_mem_set)
    MC_SET_STD_HEAP;
}

void MC_remove_ignore_heap(void *address, size_t size)
{

  int raw_mem_set = (mmalloc_get_current_heap() == mc_heap);

  MC_SET_MC_HEAP;

  unsigned int cursor = 0;
  int start = 0;
  int end = xbt_dynar_length(mc_heap_comparison_ignore) - 1;
  mc_heap_ignore_region_t region;
  int ignore_found = 0;

  while (start <= end) {
    cursor = (start + end) / 2;
    region =
        (mc_heap_ignore_region_t) xbt_dynar_get_as(mc_heap_comparison_ignore,
                                                   cursor,
                                                   mc_heap_ignore_region_t);
    if (region->address == address) {
      ignore_found = 1;
      break;
    } else if (region->address < address) {
      start = cursor + 1;
    } else {
      if ((char *) region->address <= ((char *) address + size)) {
        ignore_found = 1;
        break;
      } else {
        end = cursor - 1;
      }
    }
  }

  if (ignore_found == 1) {
    xbt_dynar_remove_at(mc_heap_comparison_ignore, cursor, NULL);
    MC_remove_ignore_heap(address, size);
  }

  if (!raw_mem_set)
    MC_SET_STD_HEAP;

}

void MC_ignore_global_variable(const char *name)
{

  int raw_mem_set = (mmalloc_get_current_heap() == mc_heap);

  MC_SET_MC_HEAP;

  xbt_assert(mc_libsimgrid_info, "MC subsystem not initialized");

  unsigned int cursor = 0;
  dw_variable_t current_var;
  int start = 0;
  int end = xbt_dynar_length(mc_libsimgrid_info->global_variables) - 1;

  while (start <= end) {
    cursor = (start + end) / 2;
    current_var =
        (dw_variable_t) xbt_dynar_get_as(mc_libsimgrid_info->global_variables,
                                         cursor, dw_variable_t);
    if (strcmp(current_var->name, name) == 0) {
      xbt_dynar_remove_at(mc_libsimgrid_info->global_variables, cursor, NULL);
      start = 0;
      end = xbt_dynar_length(mc_libsimgrid_info->global_variables) - 1;
    } else if (strcmp(current_var->name, name) < 0) {
      start = cursor + 1;
    } else {
      end = cursor - 1;
    }
  }

  if (!raw_mem_set)
    MC_SET_STD_HEAP;
}

/** \brief Ignore a local variable in a scope
 *
 *  Ignore all instances of variables with a given name in
 *  any (possibly inlined) subprogram with a given namespaced
 *  name.
 *
 *  \param var_name        Name of the local variable (or parameter to ignore)
 *  \param subprogram_name Name of the subprogram fo ignore (NULL for any)
 *  \param subprogram      (possibly inlined) Subprogram of the scope
 *  \param scope           Current scope
 */
static void mc_ignore_local_variable_in_scope(const char *var_name,
                                              const char *subprogram_name,
                                              dw_frame_t subprogram,
                                              dw_frame_t scope)
{
  // Processing of direct variables:

  // If the current subprogram matche the given name:
  if (subprogram_name == NULL || strcmp(subprogram_name, subprogram->name) == 0) {

    // Try to find the variable and remove it:
    int start = 0;
    int end = xbt_dynar_length(scope->variables) - 1;

    // Dichotomic search:
    while (start <= end) {
      int cursor = (start + end) / 2;
      dw_variable_t current_var =
          (dw_variable_t) xbt_dynar_get_as(scope->variables, cursor,
                                           dw_variable_t);

      int compare = strcmp(current_var->name, var_name);
      if (compare == 0) {
        // Variable found, remove it:
        xbt_dynar_remove_at(scope->variables, cursor, NULL);

        // and start again:
        start = 0;
        end = xbt_dynar_length(scope->variables) - 1;
      } else if (compare < 0) {
        start = cursor + 1;
      } else {
        end = cursor - 1;
      }
    }

  }
  // And recursive processing in nested scopes:
  unsigned cursor = 0;
  dw_frame_t nested_scope = NULL;
  xbt_dynar_foreach(scope->scopes, cursor, nested_scope) {
    // The new scope may be an inlined subroutine, in this case we want to use its
    // namespaced name in recursive calls:
    dw_frame_t nested_subprogram =
        nested_scope->tag ==
        DW_TAG_inlined_subroutine ? nested_scope : subprogram;

    mc_ignore_local_variable_in_scope(var_name, subprogram_name,
                                      nested_subprogram, nested_scope);
  }
}

static void MC_ignore_local_variable_in_object(const char *var_name,
                                               const char *subprogram_name,
                                               mc_object_info_t info)
{
  xbt_dict_cursor_t cursor2;
  dw_frame_t frame;
  char *key;
  xbt_dict_foreach(info->subprograms, cursor2, key, frame) {
    mc_ignore_local_variable_in_scope(var_name, subprogram_name, frame, frame);
  }
}

void MC_ignore_local_variable(const char *var_name, const char *frame_name)
{

  int raw_mem_set = (mmalloc_get_current_heap() == mc_heap);

  if (strcmp(frame_name, "*") == 0)
    frame_name = NULL;

  MC_SET_MC_HEAP;

  MC_ignore_local_variable_in_object(var_name, frame_name, mc_libsimgrid_info);
  if (frame_name != NULL)
    MC_ignore_local_variable_in_object(var_name, frame_name, mc_binary_info);

  if (!raw_mem_set)
    MC_SET_STD_HEAP;

}

/** @brief Register a stack in the model checker
 *
 *  The stacks are allocated in the heap. The MC handle them especially
 *  when we analyse/compare the content of theap so it must be told where
 *  they are with this function.
 *
 *  @param stack
 *  @param process Process owning the stack
 *  @param context
 *  @param size    Size of the stack
 */
void MC_new_stack_area(void *stack, smx_process_t process, void *context, size_t size)
{

  int raw_mem_set = (mmalloc_get_current_heap() == mc_heap);

  MC_SET_MC_HEAP;

  if (stacks_areas == NULL)
    stacks_areas = xbt_dynar_new(sizeof(stack_region_t), NULL);

  stack_region_t region = NULL;
  region = xbt_new0(s_stack_region_t, 1);
  region->address = stack;
  region->process_name = process && process->name ? strdup(process->name) : NULL;
  region->context = context;
  region->size = size;
  region->block =
      ((char *) stack -
       (char *) ((xbt_mheap_t) std_heap)->heapbase) / BLOCKSIZE + 1;
#ifdef HAVE_SMPI
  if (smpi_privatize_global_variables && process) {
    region->process_index = smpi_process_index_of_smx_process(process);
  } else
#endif
  region->process_index = -1;

  xbt_dynar_push(stacks_areas, &region);

  if (!raw_mem_set)
    MC_SET_STD_HEAP;
}

void MC_ignore(void *addr, size_t size)
{

  int raw_mem_set = (mmalloc_get_current_heap() == mc_heap);

  MC_SET_MC_HEAP;

  if (mc_checkpoint_ignore == NULL)
    mc_checkpoint_ignore =
        xbt_dynar_new(sizeof(mc_checkpoint_ignore_region_t),
                      checkpoint_ignore_region_free_voidp);

  mc_checkpoint_ignore_region_t region =
      xbt_new0(s_mc_checkpoint_ignore_region_t, 1);
  region->addr = addr;
  region->size = size;

  if (xbt_dynar_is_empty(mc_checkpoint_ignore)) {
    xbt_dynar_push(mc_checkpoint_ignore, &region);
  } else {

    unsigned int cursor = 0;
    int start = 0;
    int end = xbt_dynar_length(mc_checkpoint_ignore) - 1;
    mc_checkpoint_ignore_region_t current_region = NULL;

    while (start <= end) {
      cursor = (start + end) / 2;
      current_region =
          (mc_checkpoint_ignore_region_t) xbt_dynar_get_as(mc_checkpoint_ignore,
                                                           cursor,
                                                           mc_checkpoint_ignore_region_t);
      if (current_region->addr == addr) {
        if (current_region->size == size) {
          checkpoint_ignore_region_free(region);
          if (!raw_mem_set)
            MC_SET_STD_HEAP;
          return;
        } else if (current_region->size < size) {
          start = cursor + 1;
        } else {
          end = cursor - 1;
        }
      } else if (current_region->addr < addr) {
        start = cursor + 1;
      } else {
        end = cursor - 1;
      }
    }

    if (current_region->addr == addr) {
      if (current_region->size < size) {
        xbt_dynar_insert_at(mc_checkpoint_ignore, cursor + 1, &region);
      } else {
        xbt_dynar_insert_at(mc_checkpoint_ignore, cursor, &region);
      }
    } else if (current_region->addr < addr) {
      xbt_dynar_insert_at(mc_checkpoint_ignore, cursor + 1, &region);
    } else {
      xbt_dynar_insert_at(mc_checkpoint_ignore, cursor, &region);
    }
  }

  if (!raw_mem_set)
    MC_SET_STD_HEAP;
}

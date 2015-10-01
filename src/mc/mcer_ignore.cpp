/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "internal_config.h"
#include "mc_dwarf.hpp"
#include "mc/mc_private.h"
#include "smpi/private.h"
#include "mc/mc_snapshot.h"
#include "mc/mc_ignore.h"
#include "mc/mc_protocol.h"
#include "mc/mc_client.h"

#include "mc/Frame.hpp"
#include "mc/Variable.hpp"
#include "mc/ObjectInformation.hpp"

extern "C" {

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mcer_ignore, mc,
                                "Logging specific to MC ignore mechanism");

// ***** Ignore heap chunks

extern XBT_PRIVATE xbt_dynar_t mc_heap_comparison_ignore;

static void heap_ignore_region_free(mc_heap_ignore_region_t r)
{
  xbt_free(r);
}

static void heap_ignore_region_free_voidp(void *r)
{
  heap_ignore_region_free((mc_heap_ignore_region_t) * (void **) r);
}


XBT_PRIVATE void MC_heap_region_ignore_insert(mc_heap_ignore_region_t region)
{
  if (mc_heap_comparison_ignore == NULL) {
    mc_heap_comparison_ignore =
        xbt_dynar_new(sizeof(mc_heap_ignore_region_t),
                      heap_ignore_region_free_voidp);
    xbt_dynar_push(mc_heap_comparison_ignore, &region);
    return;
  }

  unsigned int cursor = 0;
  mc_heap_ignore_region_t current_region = NULL;
  int start = 0;
  int end = xbt_dynar_length(mc_heap_comparison_ignore) - 1;

  // Find the position where we want to insert the mc_heap_ignore_region_t:
  while (start <= end) {
    cursor = (start + end) / 2;
    current_region =
        (mc_heap_ignore_region_t) xbt_dynar_get_as(mc_heap_comparison_ignore,
                                                   cursor,
                                                   mc_heap_ignore_region_t);
    if (current_region->address == region->address) {
      heap_ignore_region_free(region);
      return;
    } else if (current_region->address < region->address) {
      start = cursor + 1;
    } else {
      end = cursor - 1;
    }
  }

  // Insert it mc_heap_ignore_region_t:
  if (current_region->address < region->address)
    xbt_dynar_insert_at(mc_heap_comparison_ignore, cursor + 1, &region);
  else
    xbt_dynar_insert_at(mc_heap_comparison_ignore, cursor, &region);
}

XBT_PRIVATE void MC_heap_region_ignore_remove(void *address, size_t size)
{
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
}

// ***** Ignore global variables

XBT_PRIVATE void MCer_ignore_global_variable(const char *name)
{
  simgrid::mc::Process* process = &mc_model_checker->process();
  xbt_assert(!process->object_infos.empty(), "MC subsystem not initialized");

  for (std::shared_ptr<simgrid::mc::ObjectInformation> const& info : process->object_infos) {

    // Binary search:
    int start = 0;
    int end = info->global_variables.size() - 1;
    while (start <= end) {
      unsigned int cursor = (start + end) / 2;
      simgrid::mc::Variable* current_var = &info->global_variables[cursor];
      int cmp = current_var->name.compare(name);
      if (cmp == 0) {
        info->global_variables.erase(
          info->global_variables.begin() + cursor);
        start = 0;
        end = info->global_variables.size() - 1;
      } else if (cmp < 0) {
        start = cursor + 1;
      } else {
        end = cursor - 1;
      }
    }
  }
}

// ***** Ignore local variables

static void mc_ignore_local_variable_in_scope(const char *var_name,
                                              const char *subprogram_name,
                                              simgrid::mc::Frame* subprogram,
                                              simgrid::mc::Frame* scope);
static void MC_ignore_local_variable_in_object(const char *var_name,
                                               const char *subprogram_name,
                                               simgrid::mc::ObjectInformation* info);

void MC_ignore_local_variable(const char *var_name, const char *frame_name)
{
  simgrid::mc::Process* process = &mc_model_checker->process();
  if (strcmp(frame_name, "*") == 0)
    frame_name = NULL;

  for (std::shared_ptr<simgrid::mc::ObjectInformation> const& info : process->object_infos)
    MC_ignore_local_variable_in_object(var_name, frame_name, info.get());
}

static void MC_ignore_local_variable_in_object(const char *var_name,
                                               const char *subprogram_name,
                                               simgrid::mc::ObjectInformation* info)
{
  for (auto& entry : info->subprograms)
    mc_ignore_local_variable_in_scope(
      var_name, subprogram_name, &entry.second, &entry.second);
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
                                              simgrid::mc::Frame* subprogram,
                                              simgrid::mc::Frame* scope)
{
  // Processing of direct variables:

  // If the current subprogram matches the given name:
  if (subprogram_name == nullptr ||
      (!subprogram->name.empty()
        && subprogram->name == subprogram_name)) {

    // Try to find the variable and remove it:
    int start = 0;
    int end = scope->variables.size() - 1;

    // Dichotomic search:
    while (start <= end) {
      int cursor = (start + end) / 2;
      simgrid::mc::Variable* current_var = &scope->variables[cursor];

      int compare = current_var->name.compare(var_name);
      if (compare == 0) {
        // Variable found, remove it:
        scope->variables.erase(scope->variables.begin() + cursor);

        // and start again:
        start = 0;
        end = scope->variables.size() - 1;
      } else if (compare < 0) {
        start = cursor + 1;
      } else {
        end = cursor - 1;
      }
    }

  }
  // And recursive processing in nested scopes:
  for (simgrid::mc::Frame& nested_scope : scope->scopes) {
    // The new scope may be an inlined subroutine, in this case we want to use its
    // namespaced name in recursive calls:
    simgrid::mc::Frame* nested_subprogram =
        nested_scope.tag ==
        DW_TAG_inlined_subroutine ? &nested_scope : subprogram;

    mc_ignore_local_variable_in_scope(var_name, subprogram_name,
                                      nested_subprogram, &nested_scope);
  }
}

extern XBT_PRIVATE xbt_dynar_t stacks_areas;

XBT_PRIVATE void MC_stack_area_add(stack_region_t stack_area)
{
  if (stacks_areas == NULL)
    stacks_areas = xbt_dynar_new(sizeof(stack_region_t), NULL);
  xbt_dynar_push(stacks_areas, &stack_area);
}

}

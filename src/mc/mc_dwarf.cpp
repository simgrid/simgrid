/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdint.h>

#include <algorithm>
#include <memory>

#include <stdlib.h>
#define DW_LANG_Objc DW_LANG_ObjC       /* fix spelling error in older dwarf.h */

#include <simgrid/util.hpp>
#include <xbt/log.h>
#include <xbt/sysdep.h>

#include <simgrid/util.hpp>

#include "mc/mc_dwarf.hpp"
#include "mc/mc_dwarf.hpp"
#include "mc/mc_private.h"
#include "mc/mc_process.h"

#include "mc/ObjectInformation.hpp"
#include "mc/Variable.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_dwarf, mc, "DWARF processing");

// ***** Functions index

static int MC_compare_frame_index_items(simgrid::mc::FunctionIndexEntry* a,
                                        simgrid::mc::FunctionIndexEntry* b)
{
  if (a->low_pc < b->low_pc)
    return -1;
  else if (a->low_pc == b->low_pc)
    return 0;
  else
    return 1;
}

static void MC_make_functions_index(simgrid::mc::ObjectInformation* info)
{
  info->functions_index.clear();

  for (auto& e : info->subprograms) {
    if (e.second.low_pc == nullptr)
      continue;
    simgrid::mc::FunctionIndexEntry entry;
    entry.low_pc = e.second.low_pc;
    entry.function = &e.second;
    info->functions_index.push_back(entry);
  }

  info->functions_index.shrink_to_fit();

  // Sort the array by low_pc:
  std::sort(info->functions_index.begin(), info->functions_index.end(),
        [](simgrid::mc::FunctionIndexEntry const& a,
          simgrid::mc::FunctionIndexEntry const& b)
        {
          return a.low_pc < b.low_pc;
        });
}

static void MC_post_process_variables(simgrid::mc::ObjectInformation* info)
{
  // Someone needs this to be sorted but who?
  std::sort(info->global_variables.begin(), info->global_variables.end(),
    MC_compare_variable);

  for(simgrid::mc::Variable& variable : info->global_variables)
    if (variable.type_id)
      variable.type = simgrid::util::find_map_ptr(
        info->types, variable.type_id);
}

static void mc_post_process_scope(
  simgrid::mc::ObjectInformation* info, simgrid::mc::Frame* scope)
{

  if (scope->tag == DW_TAG_inlined_subroutine) {
    // Attach correct namespaced name in inlined subroutine:
    auto i = info->subprograms.find(scope->abstract_origin_id);
    xbt_assert(i != info->subprograms.end(),
      "Could not lookup abstract origin %" PRIx64,
      (uint64_t) scope->abstract_origin_id);
    scope->name = i->second.name;
  }

  // Direct:
  for (simgrid::mc::Variable& variable : scope->variables)
    if (variable.type_id)
      variable.type = simgrid::util::find_map_ptr(
        info->types, variable.type_id);

  // Recursive post-processing of nested-scopes:
  for (simgrid::mc::Frame& nested_scope : scope->scopes)
      mc_post_process_scope(info, &nested_scope);

}

/** \brief Fill/lookup the "subtype" field.
 */
static void MC_resolve_subtype(
  simgrid::mc::ObjectInformation* info, simgrid::mc::Type* type)
{
  if (!type->type_id)
    return;
  type->subtype = simgrid::util::find_map_ptr(info->types, type->type_id);
  if (type->subtype == nullptr)
    return;
  if (type->subtype->byte_size != 0)
    return;
  if (type->subtype->name.empty())
    return;
  // Try to find a more complete description of the type:
  // We need to fix in order to support C++.
  simgrid::mc::Type** subtype = simgrid::util::find_map_ptr(
    info->full_types_by_name, type->subtype->name);
  if (subtype)
    type->subtype = *subtype;
}

static void MC_post_process_types(simgrid::mc::ObjectInformation* info)
{
  // Lookup "subtype" field:
  for(auto& i : info->types) {
    MC_resolve_subtype(info, &(i.second));
    for (simgrid::mc::Type& member : i.second.members)
      MC_resolve_subtype(info, &member);
  }
}

/** \brief Finds informations about a given shared object/executable */
std::shared_ptr<simgrid::mc::ObjectInformation> MC_find_object_info(
  std::vector<simgrid::mc::VmMap> const& maps, const char *name, int executable)
{
  std::shared_ptr<simgrid::mc::ObjectInformation> result =
    std::make_shared<simgrid::mc::ObjectInformation>();
  if (executable)
    result->flags |= simgrid::mc::ObjectInformation::Executable;
  result->file_name = name;
  MC_find_object_address(maps, result.get());
  MC_dwarf_get_variables(result.get());
  MC_post_process_variables(result.get());
  MC_post_process_types(result.get());
  for (auto& entry : result.get()->subprograms)
    mc_post_process_scope(result.get(), &entry.second);
  MC_make_functions_index(result.get());
  return std::move(result);
}

/*************************************************************************/

void MC_post_process_object_info(simgrid::mc::Process* process, simgrid::mc::ObjectInformation* info)
{
  for (auto& i : info->types) {

    simgrid::mc::Type* type = &(i.second);
    simgrid::mc::Type* subtype = type;
    while (subtype->type == DW_TAG_typedef
        || subtype->type == DW_TAG_volatile_type
        || subtype->type == DW_TAG_const_type)
      if (subtype->subtype)
        subtype = subtype->subtype;
      else
        break;

    // Resolve full_type:
    if (!subtype->name.empty() && subtype->byte_size == 0) {
      for (auto const& object_info : process->object_infos) {
        auto i = object_info->full_types_by_name.find(subtype->name);
        if (i != object_info->full_types_by_name.end()
            && !i->second->name.empty() && i->second->byte_size) {
          type->full_type = i->second;
          break;
        }
      }
    } else type->full_type = subtype;

  }
}

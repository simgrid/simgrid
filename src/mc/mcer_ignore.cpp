/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "src/internal_config.h"
#include "mc_object_info.h"
#include "src/mc/mc_private.h"
#include "src/smpi/private.h"
#include "src/mc/mc_snapshot.h"
#include "src/mc/mc_ignore.h"
#include "src/mc/mc_protocol.h"
#include "src/mc/mc_client.h"

#include "src/mc/Frame.hpp"
#include "src/mc/Variable.hpp"
#include "src/mc/ObjectInformation.hpp"

extern "C" {

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mcer_ignore, mc,
                                "Logging specific to MC ignore mechanism");

}

// ***** Ignore local variables

static void mc_ignore_local_variable_in_scope(const char *var_name,
                                              const char *subprogram_name,
                                              simgrid::mc::Frame* subprogram,
                                              simgrid::mc::Frame* scope);
static void MC_ignore_local_variable_in_object(const char *var_name,
                                               const char *subprogram_name,
                                               simgrid::mc::ObjectInformation* info);

extern "C"
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
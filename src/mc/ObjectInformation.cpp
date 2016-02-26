/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdint>

#include <vector>

#include "src/mc/Frame.hpp"
#include "src/mc/ObjectInformation.hpp"
#include "src/mc/Variable.hpp"

namespace simgrid {
namespace mc {

ObjectInformation::ObjectInformation()
{
  this->flags = 0;
  this->start = nullptr;
  this->end = nullptr;
  this->start_exec = nullptr;
  this->end_exec = nullptr;
  this->start_rw = nullptr;
  this->end_rw = nullptr;
  this->start_ro = nullptr;
  this->end_ro = nullptr;
}

/** Find the DWARF offset for this ELF object
 *
 *  An offset is applied to address found in DWARF:
 *
 *  * for an executable obejct, addresses are virtual address
 *    (there is no offset) i.e.
 *    \f$\text{virtual address} = \{dwarf address}\f$;
 *
 *  * for a shared object, the addreses are offset from the begining
 *    of the shared object (the base address of the mapped shared
 *    object must be used as offset
 *    i.e. \f$\text{virtual address} = \text{shared object base address}
 *             + \text{dwarf address}\f$.
 */
void *ObjectInformation::base_address() const
{
  if (this->executable())
    return nullptr;

  void *result = this->start_exec;
  if (this->start_rw != nullptr && result > (void *) this->start_rw)
    result = this->start_rw;
  if (this->start_ro != nullptr && result > (void *) this->start_ro)
    result = this->start_ro;
  return result;
}

/* Find a function by instruction pointer */
simgrid::mc::Frame* ObjectInformation::find_function(const void *ip) const
{
  /* This is implemented by binary search on a sorted array.
   *
   * We do quite a lot ot those so we want this to be cache efficient.
   * We pack the only information we need in the index entries in order
   * to successfully do the binary search. We do not need the high_pc
   * during the binary search (only at the end) so it is not included
   * in the index entry. We could use parallel arrays as well.
   *
   * We cannot really use the std:: alogrithm for this.
   * We could use std::binary_search by including the high_pc inside
   * the FunctionIndexEntry.
   */
  const simgrid::mc::FunctionIndexEntry* base =
    this->functions_index.data();
  int i = 0;
  int j = this->functions_index.size() - 1;
  while (j >= i) {
    int k = i + ((j - i) / 2);

    /* In most of the search, we do not dereference the base[k].function.
     * This way the memory accesses are located in the base[k] array. */
    if (ip < base[k].low_pc)
      j = k - 1;
    else if (k < j && ip >= base[k + 1].low_pc)
      i = k + 1;

    /* At this point, the search is over.
     * Either we have found the correct function or we do not know
     * any function corresponding to this instruction address.
     * Only at the point do we derefernce the function pointer. */
    else if ((std::uint64_t) ip < base[k].function->range.end())
      return base[k].function;
    else
      return nullptr;
  }
  return nullptr;
}

simgrid::mc::Variable* ObjectInformation::find_variable(const char* name) const
{
  for (simgrid::mc::Variable& variable : this->global_variables)
    if(variable.name == name)
      return &variable;
  return nullptr;
}

void ObjectInformation::remove_global_variable(const char* name)
{
  typedef std::vector<Variable>::size_type size_type;

  if (this->global_variables.empty())
    return;

  // Binary search:
  size_type first = 0;
  size_type last = this->global_variables.size() - 1;

  while (first <= last) {
    size_type cursor = first + (last - first) / 2;
    simgrid::mc::Variable& current_var = this->global_variables[cursor];
    int cmp = current_var.name.compare(name);

    if (cmp == 0) {
  
      // Find the whole range:
      size_type first = cursor;
      while (first != 0 && this->global_variables[first - 1].name == name)
        first--;
      size_type size = this->global_variables.size();
      size_type last = cursor;
      while (last != size - 1 && this->global_variables[last + 1].name == name)
        last++;
  
      // Remove the whole range:
      this->global_variables.erase(
        this->global_variables.begin() + first,
        this->global_variables.begin() + last + 1);
  
      return;
    } else if (cmp < 0)
      first = cursor + 1;
    else if (cursor != 0)
      last = cursor - 1;
    else
      break;
  }
}

/** \brief Ignore a local variable in a scope
 *
 *  Ignore all instances of variables with a given name in
 *  any (possibly inlined) subprogram with a given namespaced
 *  name.
 *
 *  \param var_name        Name of the local variable (or parameter to ignore)
 *  \param subprogram_name Name of the subprogram fo ignore (nullptr for any)
 *  \param subprogram      (possibly inlined) Subprogram of the scope
 *  \param scope           Current scope
 */
static void remove_local_variable(simgrid::mc::Frame& scope,
                            const char *var_name,
                            const char *subprogram_name,
                            simgrid::mc::Frame const& subprogram)
{
  typedef std::vector<Variable>::size_type size_type;

  // If the current subprogram matches the given name:
  if ((subprogram_name == nullptr ||
      (!subprogram.name.empty()
        && subprogram.name == subprogram_name))
      && !scope.variables.empty()) {

    // Try to find the variable and remove it:
    size_type start = 0;
    size_type end = scope.variables.size() - 1;

    // Binary search:
    while (start <= end) {
      size_type cursor = start + (end - start) / 2;
      simgrid::mc::Variable& current_var = scope.variables[cursor];
      int compare = current_var.name.compare(var_name);
      if (compare == 0) {
        // Variable found, remove it:
        scope.variables.erase(scope.variables.begin() + cursor);
        break;
      } else if (compare < 0)
        start = cursor + 1;
      else if (cursor != 0)
        end = cursor - 1;
      else
        break;
    }
  }

  // And recursive processing in nested scopes:
  for (simgrid::mc::Frame& nested_scope : scope.scopes) {
    // The new scope may be an inlined subroutine, in this case we want to use its
    // namespaced name in recursive calls:
    simgrid::mc::Frame const& nested_subprogram =
        nested_scope.tag ==
        DW_TAG_inlined_subroutine ? nested_scope : subprogram;
    remove_local_variable(nested_scope, var_name, subprogram_name,
                          nested_subprogram);
  }
}

void ObjectInformation::remove_local_variable(
  const char* var_name, const char* subprogram_name)
{
  for (auto& entry : this->subprograms)
    simgrid::mc::remove_local_variable(entry.second,
      var_name, subprogram_name, entry.second);
}

}
}
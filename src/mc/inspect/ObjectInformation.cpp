/* Copyright (c) 2014-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <algorithm>
#include <cstdint>
#include <sys/mman.h> // PROT_READ and friends
#include <vector>

#include "src/mc/inspect/Frame.hpp"
#include "src/mc/inspect/ObjectInformation.hpp"
#include "src/mc/inspect/Variable.hpp"
#include "src/mc/mc_private.hpp"
#include "xbt/file.hpp"

namespace simgrid {
namespace mc {

/* For an executable object, addresses are virtual address (there is no offset) i.e.
 *  \f$\text{virtual address} = \{dwarf address}\f$
 *
 * For a shared object, the addresses are offset from the beginning of the shared object (the base address of the
 * mapped shared object must be used as offset
 * i.e. \f$\text{virtual address} = \text{shared object base address}
 *             + \text{dwarf address}\f$.
 */
void* ObjectInformation::base_address() const
{
  // For an executable (more precisely for an ET_EXEC) the base it 0:
  if (this->executable())
    return nullptr;

  // For an a shared-object (ET_DYN, including position-independent executables) the base address is its lowest address:
  void* result = this->start_exec;
  if (this->start_rw != nullptr && result > (void*)this->start_rw)
    result = this->start_rw;
  if (this->start_ro != nullptr && result > (void*)this->start_ro)
    result = this->start_ro;
  return result;
}

Frame* ObjectInformation::find_function(const void* ip) const
{
  /* This is implemented by binary search on a sorted array.
   *
   * We do quite a lot of those so we want this to be cache efficient.
   * We pack the only information we need in the index entries in order
   * to successfully do the binary search. We do not need the high_pc
   * during the binary search (only at the end) so it is not included
   * in the index entry. We could use parallel arrays as well.
   *
   * We cannot really use the std:: algorithm for this.
   * We could use std::binary_search by including the high_pc inside
   * the FunctionIndexEntry.
   */
  const FunctionIndexEntry* base = this->functions_index.data();
  int i                          = 0;
  int j                          = this->functions_index.size() - 1;
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
     * Only at the point do we dereference the function pointer. */
    else if ((std::uint64_t)ip < base[k].function->range.end())
      return base[k].function;
    else
      return nullptr;
  }
  return nullptr;
}

const Variable* ObjectInformation::find_variable(const char* name) const
{
  for (Variable const& variable : this->global_variables) {
    if (variable.name == name)
      return &variable;
  }
  return nullptr;
}

void ObjectInformation::remove_global_variable(const char* name)
{
  // Binary search:
  auto pos1 = std::lower_bound(this->global_variables.begin(), this->global_variables.end(), name,
                               [](auto const& var, const char* var_name) { return var.name.compare(var_name) < 0; });
  // Find the whole range:
  auto pos2 = std::upper_bound(pos1, this->global_variables.end(), name,
                               [](const char* var_name, auto const& var) { return var.name.compare(var_name) > 0; });
  // Remove the whole range:
  this->global_variables.erase(pos1, pos2);
}

/** Ignore a local variable in a scope
 *
 *  Ignore all instances of variables with a given name in any (possibly inlined) subprogram with a given namespaced
 *  name.
 *
 *  @param var_name        Name of the local variable to ignore
 *  @param subprogram_name Name of the subprogram to ignore (nullptr for any)
 *  @param subprogram      (possibly inlined) Subprogram of the scope current scope
 *  @param scope           Current scope
 */
static void remove_local_variable(Frame& scope, const char* var_name, const char* subprogram_name,
                                  Frame const& subprogram)
{
  // If the current subprogram matches the given name:
  if (subprogram_name == nullptr || (not subprogram.name.empty() && subprogram.name == subprogram_name)) {
    // Try to find the variable and remove it:

    // Binary search:
    auto pos = std::lower_bound(scope.variables.begin(), scope.variables.end(), var_name,
                                [](auto const& var, const char* var_name) { return var.name.compare(var_name) < 0; });
    if (pos != scope.variables.end() && pos->name.compare(var_name) == 0) {
      // Variable found, remove it:
      scope.variables.erase(pos);
    }
  }

  // And recursive processing in nested scopes:
  for (Frame& nested_scope : scope.scopes) {
    // The new scope may be an inlined subroutine, in this case we want to use its
    // namespaced name in recursive calls:
    Frame const& nested_subprogram = nested_scope.tag == DW_TAG_inlined_subroutine ? nested_scope : subprogram;
    remove_local_variable(nested_scope, var_name, subprogram_name, nested_subprogram);
  }
}

void ObjectInformation::remove_local_variable(const char* var_name, const char* subprogram_name)
{
  for (auto& entry : this->subprograms)
    mc::remove_local_variable(entry.second, var_name, subprogram_name, entry.second);
}

/** @brief Fills the position of the segments (executable, read-only, read/write) */
// TODO, use the ELF segment information for more robustness
void find_object_address(std::vector<xbt::VmMap> const& maps, ObjectInformation* result)
{
  const int PROT_RW = PROT_READ | PROT_WRITE;
  const int PROT_RX = PROT_READ | PROT_EXEC;

  std::string name = xbt::Path(result->file_name).get_base_name();

  for (size_t i = 0; i < maps.size(); ++i) {
    simgrid::xbt::VmMap const& reg = maps[i];
    if (maps[i].pathname.empty())
      continue;
    std::string map_basename = simgrid::xbt::Path(maps[i].pathname).get_base_name();
    if (map_basename != name)
      continue;

    // This is the non-GNU_RELRO-part of the data segment:
    if (reg.prot == PROT_RW) {
      xbt_assert(not result->start_rw, "Multiple read-write segments for %s, not supported", maps[i].pathname.c_str());
      result->start_rw = (char*)reg.start_addr;
      result->end_rw   = (char*)reg.end_addr;

      // The next VMA might be end of the data segment:
      if (i + 1 < maps.size() && maps[i + 1].pathname.empty() && maps[i + 1].prot == PROT_RW &&
          maps[i + 1].start_addr == reg.end_addr)
        result->end_rw = (char*)maps[i + 1].end_addr;
    }

    // This is the text segment:
    else if (reg.prot == PROT_RX) {
      xbt_assert(not result->start_exec, "Multiple executable segments for %s, not supported",
                 maps[i].pathname.c_str());
      result->start_exec = (char*)reg.start_addr;
      result->end_exec   = (char*)reg.end_addr;

      // The next VMA might be end of the data segment:
      if (i + 1 < maps.size() && maps[i + 1].pathname.empty() && maps[i + 1].prot == PROT_RW &&
          maps[i + 1].start_addr == reg.end_addr) {
        result->start_rw = (char*)maps[i + 1].start_addr;
        result->end_rw   = (char*)maps[i + 1].end_addr;
      }
    }

    // This is the GNU_RELRO-part of the data segment:
    else if (reg.prot == PROT_READ) {
      xbt_assert(not result->start_ro,
                 "Multiple read-only segments for %s, not supported. Compiling with the following may help: "
                 "-Wl,-znorelro -Wl,-znoseparate-code",
                 maps[i].pathname.c_str());
      result->start_ro = (char*)reg.start_addr;
      result->end_ro   = (char*)reg.end_addr;
    }
  }

  result->start = result->start_rw;
  if ((const void*)result->start_ro < result->start)
    result->start = result->start_ro;
  if ((const void*)result->start_exec < result->start)
    result->start = result->start_exec;

  result->end = result->end_rw;
  if (result->end_ro && (const void*)result->end_ro > result->end)
    result->end = result->end_ro;
  if (result->end_exec && (const void*)result->end_exec > result->end)
    result->end = result->end_exec;

  xbt_assert(result->start_exec || result->start_rw || result->start_ro);
}

} // namespace mc
} // namespace simgrid

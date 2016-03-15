/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_OBJECT_INFORMATION_HPP
#define SIMGRID_MC_OBJECT_INFORMATION_HPP

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

#include <xbt/base.h>

#include "src/xbt/memory_map.hpp"
#include "src/mc/mc_forward.hpp"
#include "src/mc/Type.hpp"
#include "src/mc/Frame.hpp"

#include "src/smpi/private.h"

namespace simgrid {
namespace mc {

/** An entry in the functions index
 *
 *  See the code of ObjectInformation::find_function.
 */
struct FunctionIndexEntry {
  void* low_pc;
  simgrid::mc::Frame* function;
};

/** Information about an (ELF) executable/sharedobject
 *
 *  This contain sall the information we have at runtime about an
 *  executable/shared object in the target (modelchecked) process:
 *  - where it is located in the virtual address space;
 *  - where are located it's different memory mapping in the the
 *    virtual address space ;
 *  - all the debugging (DWARF) information,
 *    - location of the functions,
 *    - types
 *  - etc.
 *
 *  It is not copyable because we are taking pointers to Types/Frames.
 *  We'd have to update/rebuild some data structures in order to copy
 *  successfully.
 */

class ObjectInformation {
public:
  ObjectInformation();

  // Not copyable:
  ObjectInformation(ObjectInformation const&) = delete;
  ObjectInformation& operator=(ObjectInformation const&) = delete;

  // Flag:
  static const int Executable = 1;

  /** Bitfield of flags */
  int flags;
  std::string file_name;
  const void* start;
  const void *end;
  char *start_exec;
  char *end_exec; // Executable segment
  char *start_rw;
  char *end_rw; // Read-write segment
  char *start_ro;
  char *end_ro; // read-only segment
  std::unordered_map<std::uint64_t, simgrid::mc::Frame> subprograms;
  // TODO, remove the mutable (to remove it we'll have to add a lot of const everywhere)
  mutable std::vector<simgrid::mc::Variable> global_variables;
  std::unordered_map<std::uint64_t, simgrid::mc::Type> types;
  std::unordered_map<std::string, simgrid::mc::Type*> full_types_by_name;

  /** Index of functions by IP
   *
   * The entries are sorted by low_pc and a binary search can be used to look
   * them up. Should we used a binary tree instead?
   */
  std::vector<FunctionIndexEntry> functions_index;

  bool executable() const
  {
    return this->flags & simgrid::mc::ObjectInformation::Executable;
  }

  void* base_address() const;

  simgrid::mc::Frame* find_function(const void *ip) const;
  simgrid::mc::Variable* find_variable(const char* name) const;
  void remove_global_variable(const char* name);
  void remove_local_variable(
    const char* name, const char* scope);
};

XBT_PRIVATE std::shared_ptr<ObjectInformation> createObjectInformation(
  std::vector<simgrid::xbt::VmMap> const& maps, const char* name);
XBT_PRIVATE void postProcessObjectInformation(
  simgrid::mc::Process* process, simgrid::mc::ObjectInformation* info);

}
}

#endif

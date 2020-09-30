/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_OBJECT_INFORMATION_HPP
#define SIMGRID_MC_OBJECT_INFORMATION_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "src/mc/inspect/Frame.hpp"
#include "src/mc/inspect/Type.hpp"
#include "src/mc/mc_forward.hpp"
#include "src/xbt/memory_map.hpp"

#include "src/smpi/include/private.hpp"

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

/** Information about an ELF module (executable or shared object)
 *
 *  This contains all the information we need about an executable or
 *  shared-object in the model-checked process:
 *
 *  - where it is located in the virtual address space;
 *
 *  - where are located its different memory mappings in the the
 *    virtual address space;
 *
 *  - all the debugging (DWARF) information
 *    - types,
 *    - location of the functions and their local variables,
 *    - global variables,
 *
 *  - etc.
 */
class ObjectInformation {
public:
  ObjectInformation() = default;

  // Not copiable:
  ObjectInformation(ObjectInformation const&) = delete;
  ObjectInformation& operator=(ObjectInformation const&) = delete;

  // Flag:
  static const int Executable = 1;

  /** Bitfield of flags */
  int flags = 0;
  std::string file_name;
  const void* start = nullptr;
  const void* end   = nullptr;
  // Location of its text segment:
  char* start_exec = nullptr;
  char* end_exec   = nullptr;
  // Location of the read-only part of its data segment:
  char* start_rw = nullptr;
  char* end_rw   = nullptr;
  // Location of the read/write part of its data segment:
  char* start_ro = nullptr;
  char* end_ro   = nullptr;

  /** All of its subprograms indexed by their address */
  std::unordered_map<std::uint64_t, simgrid::mc::Frame> subprograms;

  /** Index of functions by instruction address
   *
   * We need to efficiently find the function from any given instruction
   * address inside its range. This index is sorted by low_pc
   *
   * The entries are sorted by low_pc and a binary search can be used to look
   * them up. In order to have a better cache locality, we only keep the
   * information we need for the lookup in this vector. We could probably
   * replace subprograms by an ordered vector of Frame and replace this one b
   * a parallel `std::vector<void*>`.
   */
  std::vector<FunctionIndexEntry> functions_index;

  std::vector<simgrid::mc::Variable> global_variables;

  /** Types indexed by DWARF ID */
  std::unordered_map<std::uint64_t, simgrid::mc::Type> types;

  /** Types indexed by name
   *
   *  Different compilation units have their separate type definitions
   *  (for the same type). When we find an opaque type in one compilation unit,
   *  we use this in order to try to find its definition in another compilation
   *  unit.
   */
  std::unordered_map<std::string, simgrid::mc::Type*> full_types_by_name;

  /** Whether this module is an executable
   *
   *  More precisely we check if this is an ET_EXE ELF. These ELF files
   *  use fixed addresses instead of base-address relative addresses.
   *  Position independent executables are in fact ET_DYN.
   */
  bool executable() const { return this->flags & simgrid::mc::ObjectInformation::Executable; }

  /** Base address of the module
   *
   *  All the location information in ELF and DWARF are expressed as an offsets
   *  from this base address:
   *
   *  - location of the functions and global variables
   *
   *  - the DWARF instruction `OP_addr` pushes this on the DWARF stack.
   **/
  void* base_address() const;

  /** Find a function by instruction address
   *
   *  @param ip instruction address
   *  @return corresponding function (if any) or nullptr
   */
  simgrid::mc::Frame* find_function(const void* ip) const;

  /** Find a global variable by name
   *
   *  This is used to ignore global variables and to find well-known variables
   *  (`__mmalloc_default_mdp`).
   *
   *  @param name scopes name of the global variable (`myproject::Foo::count`)
   *  @return corresponding variable (if any) or nullptr
   */
  const simgrid::mc::Variable* find_variable(const char* name) const;

  /** Remove a global variable (in order to ignore it)
   *
   *  This is used to ignore a global variable for the snapshot comparison.
   */
  void remove_global_variable(const char* name);

  /** Remove a local variables (in order to ignore it)
   *
   *  @param name Name of the local variable
   *  @param scope scopes name name of the function (myproject::Foo::count) or null for all functions
   */
  void remove_local_variable(const char* name, const char* scope);
};

XBT_PRIVATE std::shared_ptr<ObjectInformation> createObjectInformation(std::vector<simgrid::xbt::VmMap> const& maps,
                                                                       const char* name);

/** Augment the current module with information about the other ones */
XBT_PRIVATE void postProcessObjectInformation(const simgrid::mc::RemoteSimulation* process,
                                              simgrid::mc::ObjectInformation* info);
} // namespace mc
} // namespace simgrid

#endif

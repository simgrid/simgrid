/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_ADDRESS_SPACE_H
#define MC_ADDRESS_SPACE_H

#include <cstdint>
#include <type_traits>

#include <xbt/misc.h>

#include <stdint.h>

#include "mc_forward.h"

// Compatibility stuff, will be removed:
#define MC_ADDRESS_SPACE_READ_FLAGS_NONE ::simgrid::mc::AddressSpace::Normal
#define MC_ADDRESS_SPACE_READ_FLAGS_LAZY ::simgrid::mc::AddressSpace::Lazy

// Compatibility stuff, will be removed:
#define MC_PROCESS_INDEX_MISSING ::simgrid::mc::ProcessIndexMissing
#define MC_PROCESS_INDEX_DISABLED ::simgrid::mc::ProcessIndexDisabled
#define MC_PROCESS_INDEX_ANY ::simgrid::mc::ProcessIndexAny

namespace simgrid {
namespace mc {

/** Process index used when no process is available
 *
 *  The expected behaviour is that if a process index is needed it will fail.
 * */
const int ProcessIndexMissing = -1;

/** Process index used when we don't care about the process index
 * */
const int ProcessIndexDisabled = -2;

/** Constant used when any process will do.
 *
 *  This is is index of the first process.
 */
const int ProcessIndexAny = 0;

class AddressSpace {
public:
  enum ReadMode {
    Normal,
    /** Allows the `read_bytes` to return a pointer to another buffer
     *  where the data ins available instead of copying the data into the buffer
     */
    Lazy
  };
  virtual ~AddressSpace();
  virtual const void* read_bytes(void* buffer, std::size_t size,
    std::uint64_t address, int process_index = ProcessIndexAny,
    ReadMode mode = Normal) = 0;
  template<class T>
  T read(uint64_t address, int process_index = ProcessIndexMissing)
  {
    static_assert(std::is_trivial<T>::value, "Cannot read a non-trivial type");
    T res;
    this->read_bytes(&res, sizeof(T), address, process_index);
    return res;
  }
};

}
}

// Deprecated compatibility wrapper:
static inline
const void* MC_address_space_read(
  mc_address_space_t address_space, simgrid::mc::AddressSpace::ReadMode mode,
  void* target, const void* addr, size_t size,
  int process_index)
{
  return address_space->read_bytes(target, size, (std::uint64_t) addr,
    process_index, mode);
}

#endif

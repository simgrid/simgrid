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
    return *(T*)this->read_bytes(&res, sizeof(T), address, process_index);
  }
};

}
}

#endif

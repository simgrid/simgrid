/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_ADDRESS_SPACE_H
#define SIMGRID_MC_ADDRESS_SPACE_H

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "src/mc/mc_forward.hpp"
#include "src/mc/RemotePtr.hpp"

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

/** Options for read operations
 *
 *  This is a set of flags managed with bitwise operators. Only the
 *  meaningful operations are defined: addition, conversions to/from
 *  integers are not allowed.
 */
class ReadOptions {
  std::uint32_t value_;
  constexpr explicit ReadOptions(std::uint32_t value) : value_(value) {}
public:
  constexpr ReadOptions() : value_(0) {}

  constexpr operator bool() const { return value_ != 0; }
  constexpr bool operator!() const { return value_ == 0; }

  constexpr ReadOptions operator|(ReadOptions const& that) const
  {
    return ReadOptions(value_ | that.value_);
  }
  constexpr ReadOptions operator&(ReadOptions const& that) const
  {
    return ReadOptions(value_ & that.value_);
  }
  constexpr ReadOptions operator^(ReadOptions const& that) const
  {
    return ReadOptions(value_ ^ that.value_);
  }
  constexpr ReadOptions operator~() const
  {
    return ReadOptions(~value_);
  }

  ReadOptions& operator|=(ReadOptions const& that)
  {
    value_ |= that.value_;
    return *this;
  }
  ReadOptions& operator&=(ReadOptions const& that)
  {
    value_ &= that.value_;
    return *this;
  }
  ReadOptions& operator^=(ReadOptions const& that)
  {
    value_ &= that.value_;
    return *this;
  }

  /** Copy the data to the given buffer */
  static constexpr ReadOptions none() { return ReadOptions(0); }

  /** Allows to return a pointer to another buffer where the data is
   *  available instead of copying the data into the buffer
   */
  static constexpr ReadOptions lazy() { return ReadOptions(1); }
};

/** A given state of a given process (abstract base class)
 *
 *  Currently, this might either be:
 *
 *  * the current state of an existing process;
 *
 *  * a snapshot.
 */
class AddressSpace {
private:
  Process* process_;
public:
  AddressSpace(Process* process) : process_(process) {}
  virtual ~AddressSpace();

  simgrid::mc::Process* process() const { return process_; }

  /** Read data from the address space
   *
   *  @param buffer        target buffer for the data
   *  @param size          number of bytes
   *  @param address       remote source address of the data
   *  @param process_index which process (used for SMPI privatization)
   *  @param options
   */
  virtual const void* read_bytes(void* buffer, std::size_t size,
    RemotePtr<void> address, int process_index = ProcessIndexAny,
    ReadOptions options = ReadOptions::none()) const = 0;

  /** Read a given data structure from the address space */
  template<class T> inline
  void read(T *buffer, RemotePtr<T> ptr, int process_index = ProcessIndexAny)
  {
    this->read_bytes(buffer, sizeof(T), ptr, process_index);
  }

  /** Read a given data structure from the address space */
  template<class T> inline
  T read(RemotePtr<T> ptr, int process_index = ProcessIndexMissing)
  {
    static_assert(std::is_trivial<T>::value, "Cannot read a non-trivial type");
    T res;
    return *(T*)this->read_bytes(&res, sizeof(T), ptr, process_index);
  }
};

}
}

#endif

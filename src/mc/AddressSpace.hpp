/* Copyright (c) 2008-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_ADDRESS_SPACE_H
#define SIMGRID_MC_ADDRESS_SPACE_H

#include "src/mc/mc_forward.hpp"
#include "src/mc/remote/RemotePtr.hpp"

namespace simgrid {
namespace mc {

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

  explicit constexpr operator bool() const { return value_ != 0; }
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
 *
 *  In order to support SMPI privatization, the can read the memory from the
 *  context of a given SMPI process: if specified, the code reads data from the
 *  correct SMPI privatization VMA.
 */
class AddressSpace {
private:
  RemoteClient* process_;

public:
  explicit AddressSpace(RemoteClient* process) : process_(process) {}
  virtual ~AddressSpace() = default;

  /** The process of this address space
   *
   *  This is where we can get debug informations, memory layout, etc.
   */
  simgrid::mc::RemoteClient* process() const { return process_; }

  /** Read data from the address space
   *
   *  @param buffer        target buffer for the data
   *  @param size          number of bytes to read
   *  @param address       remote source address of the data
   *  @param options
   */
  virtual void* read_bytes(void* buffer, std::size_t size, RemotePtr<void> address,
                           ReadOptions options = ReadOptions::none()) const = 0;

  /** Read a given data structure from the address space */
  template <class T> inline void read(T* buffer, RemotePtr<T> ptr) const { this->read_bytes(buffer, sizeof(T), ptr); }

  template <class T> inline void read(Remote<T>& buffer, RemotePtr<T> ptr) const
  {
    this->read_bytes(buffer.get_buffer(), sizeof(T), ptr);
  }

  /** Read a given data structure from the address space
   *
   *  This version returns by value.
   */
  template <class T> inline Remote<T> read(RemotePtr<T> ptr) const
  {
    Remote<T> res;
    this->read_bytes(&res, sizeof(T), ptr);
    return res;
  }

  /** Read a string of known size */
  std::string read_string(RemotePtr<char> address, std::size_t len) const
  {
    std::string res;
    res.resize(len);
    this->read_bytes(&res[0], len, address);
    return res;
  }

};

}
}

#endif

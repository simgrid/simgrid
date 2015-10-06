/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_ADDRESS_SPACE_H
#define SIMGRID_MC_ADDRESS_SPACE_H

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <xbt/misc.h>

#include "mc_forward.hpp"

namespace simgrid {
namespace mc {

/** Pointer to a remote address-space (process, snapshot)
 *
 *  With this we can clearly identify the expected type of an address in the
 *  remote process whild avoiding to use native local pointers.
 */
template<class T> class remote_ptr {
  std::uint64_t address_;
public:
  remote_ptr() : address_(0) {}
  remote_ptr(std::uint64_t address) : address_(address) {}
  remote_ptr(T* address) : address_((std::uintptr_t)address) {}
  std::uint64_t address() const { return address_; }
  operator bool() const
  {
    return address_;
  }
  bool operator!() const
  {
    return !address_;
  }
  operator remote_ptr<void>() const
  {
    return remote_ptr<void>(address_);
  }
  remote_ptr<T> operator+(std::uint64_t n) const
  {
    return remote_ptr<T>(address_ + n * sizeof(T));
  }
  remote_ptr<T> operator-(std::uint64_t n) const
  {
    return remote_ptr<T>(address_ - n * sizeof(T));
  }
  remote_ptr<T>& operator+=(std::uint64_t n) const
  {
    address_ += n * sizeof(T);
    return *this;
  }
  remote_ptr<T>& operator-=(std::uint64_t n) const
  {
    address_ -= n * sizeof(T);
    return *this;
  }
};

template<class X, class Y>
bool operator<(remote_ptr<X> const& x, remote_ptr<Y> const& y)
{
  return x.address() < y.address();
}

template<class X, class Y>
bool operator>(remote_ptr<X> const& x, remote_ptr<Y> const& y)
{
  return x.address() > y.address();
}

template<class X, class Y>
bool operator>=(remote_ptr<X> const& x, remote_ptr<Y> const& y)
{
  return x.address() >= y.address();
}

template<class X, class Y>
bool operator<=(remote_ptr<X> const& x, remote_ptr<Y> const& y)
{
  return x.address() <= y.address();
}

template<class X, class Y>
bool operator==(remote_ptr<X> const& x, remote_ptr<Y> const& y)
{
  return x.address() == y.address();
}

template<class X, class Y>
bool operator!=(remote_ptr<X> const& x, remote_ptr<Y> const& y)
{
  return x.address() != y.address();
}

template<class T> inline
remote_ptr<T> remote(T *p)
{
  return remote_ptr<T>(p);
}

template<class T=void> inline
remote_ptr<T> remote(uint64_t p)
{
  return remote_ptr<T>(p);
}

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
private:
  Process* process_;
public:
  enum ReadMode {
    Normal,
    /** Allows the `read_bytes` to return a pointer to another buffer
     *  where the data ins available instead of copying the data into the buffer
     */
    Lazy
  };
  AddressSpace(Process* process) : process_(process) {}
  virtual ~AddressSpace();

  simgrid::mc::Process* process() { return process_; }
  virtual const void* read_bytes(void* buffer, std::size_t size,
    remote_ptr<void> address, int process_index = ProcessIndexAny,
    ReadMode mode = Normal) const = 0;

  template<class T> inline
  void read(T *buffer, remote_ptr<T> ptr, int process_index = ProcessIndexAny)
  {
    this->read_bytes(buffer, sizeof(T), ptr, process_index);
  }

  template<class T> inline
  T read(remote_ptr<T> ptr, int process_index = ProcessIndexMissing)
  {
    static_assert(std::is_trivial<T>::value, "Cannot read a non-trivial type");
    T res;
    return *(T*)this->read_bytes(&res, sizeof(T), ptr, process_index);
  }
};

}
}

#endif

/* Copyright (c) 2008-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_REMOTE_PTR_HPP
#define SIMGRID_MC_REMOTE_PTR_HPP

#include "src/simix/smx_private.hpp"

namespace simgrid {
namespace mc {

/** HACK, A value from another process
 *
 *  This represents a value from another process:
 *
 *  * constructor/destructor are disabled;
 *
 *  * raw memory copy (std::memcpy) is used to copy Remote<T>;
 *
 *  * when T is a trivial type, Remote is convertible to a T.
 *
 *  We currently only handle the case where the type has the same layout
 *  in the current process and in the target process: we don't handle
 *  cross-architecture (such as 32-bit/64-bit access).
 */
template <class T> class Remote {
private:
  typename std::aligned_storage<sizeof(T), alignof(T)>::type buffer;

public:
  Remote() = default;
  explicit Remote(T const& p) { std::memcpy(&buffer, &p, sizeof buffer); }

  T* get_buffer() { return reinterpret_cast<T*>(&buffer); }
  const T* get_buffer() const { return reinterpret_cast<const T*>(&buffer); }
  std::size_t get_buffer_size() const { return sizeof(T); }
  operator T() const
  {
    static_assert(std::is_trivial<T>::value, "Cannot convert non trivial type");
    return *get_buffer();
  }
  void clear() { std::memset(&buffer, 0, sizeof buffer); }
};

/** Pointer to a remote address-space (process, snapshot)
 *
 *  With this we can clearly identify the expected type of an address in the
 *  remote process while avoiding to use native local pointers.
 *
 *  Some operators (+/-) assume use the size of the underlying element. This
 *  only works if the target applications is using the same target: it won't
 *  work for example, when inspecting a 32 bit application from a 64 bit
 *  model-checker.
 *
 *  We do not actually store the target address space because we can
 *  always detect it in context. This way `RemotePtr` is as efficient
 *  as a `uint64_t`.
 */
template <class T> class RemotePtr {
  std::uint64_t address_ = 0;

public:
  RemotePtr() = default;
  explicit RemotePtr(std::nullptr_t) {}
  explicit RemotePtr(std::uint64_t address) : address_(address) {}
  explicit RemotePtr(T* address) : address_((std::uintptr_t)address) {}
  explicit RemotePtr(Remote<T*> p) : address_((std::uintptr_t)*p.get_buffer()) {}
  std::uint64_t address() const { return address_; }

  /** Turn into a local pointer
   *
   (if the remote process is not, in fact, remote) */
  T* local() const { return (T*)address_; }

  operator bool() const { return address_; }
  bool operator!() const { return not address_; }
  operator RemotePtr<void>() const { return RemotePtr<void>(address_); }
  RemotePtr<T>& operator=(std::nullptr_t)
  {
    address_ = 0;
    return *this;
  }
  RemotePtr<T> operator+(std::uint64_t n) const { return RemotePtr<T>(address_ + n * sizeof(T)); }
  RemotePtr<T> operator-(std::uint64_t n) const { return RemotePtr<T>(address_ - n * sizeof(T)); }
  RemotePtr<T>& operator+=(std::uint64_t n)
  {
    address_ += n * sizeof(T);
    return *this;
  }
  RemotePtr<T>& operator-=(std::uint64_t n)
  {
    address_ -= n * sizeof(T);
    return *this;
  }
};

template <class X, class Y> bool operator<(RemotePtr<X> const& x, RemotePtr<Y> const& y)
{
  return x.address() < y.address();
}

template <class X, class Y> bool operator>(RemotePtr<X> const& x, RemotePtr<Y> const& y)
{
  return x.address() > y.address();
}

template <class X, class Y> bool operator>=(RemotePtr<X> const& x, RemotePtr<Y> const& y)
{
  return x.address() >= y.address();
}

template <class X, class Y> bool operator<=(RemotePtr<X> const& x, RemotePtr<Y> const& y)
{
  return x.address() <= y.address();
}

template <class X, class Y> bool operator==(RemotePtr<X> const& x, RemotePtr<Y> const& y)
{
  return x.address() == y.address();
}

template <class X, class Y> bool operator!=(RemotePtr<X> const& x, RemotePtr<Y> const& y)
{
  return x.address() != y.address();
}

template <class T> inline RemotePtr<T> remote(T* p)
{
  return RemotePtr<T>(p);
}

template <class T = void> inline RemotePtr<T> remote(uint64_t p)
{
  return RemotePtr<T>(p);
}
} // namespace mc
} // namespace simgrid

#endif

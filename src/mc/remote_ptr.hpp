/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_REMOTE_PTR_HPP
#define SIMGRID_MC_REMOTE_PTR_HPP

#include <cstdint>

namespace simgrid {
namespace mc {

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
 *  always detect it in context. This way `remote_ptr` is as efficient
 *  as a `uint64_t`.
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
  remote_ptr<T>& operator+=(std::uint64_t n)
  {
    address_ += n * sizeof(T);
    return *this;
  }
  remote_ptr<T>& operator-=(std::uint64_t n)
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

}
}

#endif

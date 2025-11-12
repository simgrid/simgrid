/* Copyright (c) 2025-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This is a reimplementation of boost::intrusive_ptr to extend it with more debugging */
/* Inspired by https://ibob.bg/blog/2023/01/01/tracking-shared-ptr-leaks/  */

#ifndef XBT_INTRUSIVE_PTR_HPP
#define XBT_INTRUSIVE_PTR_HPP

#include "xbt/asserts.h"
#include "xbt/backtrace.hpp"
#include "xbt/log.h"
#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include <cstdio>
#include <functional>
#include <ostream>
#include <unordered_map>

namespace simgrid::xbt {

//
//  intrusive_ptr
//
//  A smart pointer that uses intrusive reference counting.
//
//  Relies on unqualified calls to
//
//      void intrusive_ptr_add_ref(T * p);
//      void intrusive_ptr_release(T * p);
//
//          (p != 0)
//
//  The object is responsible for destroying itself.
//

template <class T> class intrusive_ptr {
private:
  typedef intrusive_ptr this_type;
  T* px;
  char* bt = nullptr;
  intrusive_ptr<T>* self; // copy of this that remains valid in the move constructor and move assignment operator
public:
  char* get_bt() { return bt; }

  typedef T element_type;

  constexpr intrusive_ptr() noexcept : px(0), self(this) {}

  intrusive_ptr(T* p, bool add_ref = true)
      : px(p), bt(xbt_strdup(simgrid::xbt::Backtrace().resolve().c_str())), self(this)
  {
    if (px != nullptr && add_ref) {
      intrusive_ptr_add_ref(px);
      px->reference_holder_.push(this);
    }
  }

  template <class U>
  intrusive_ptr(intrusive_ptr<U> const& rhs)
      : px(rhs.get()), bt(xbt_strdup(simgrid::xbt::Backtrace().resolve().c_str()))
  {
    if (px != nullptr) {
      intrusive_ptr_add_ref(px);
      px->reference_holder_.push(this);
    }
  }

  intrusive_ptr(intrusive_ptr const& rhs)
      : px(rhs.px), bt(xbt_strdup(simgrid::xbt::Backtrace().resolve().c_str())), self(this)
  {
    if (px != nullptr) {
      intrusive_ptr_add_ref(px);
      px->reference_holder_.push(this);
    }
  }

  ~intrusive_ptr()
  {
    if (px != nullptr) {
      px->reference_holder_.pop(this);
      intrusive_ptr_release(px);
    }
  }

  template <class U> intrusive_ptr& operator=(intrusive_ptr<U> const& rhs)
  {
    bt = bprintf("---- operator=() at \n%s\n-- Orig: \n%s", simgrid::xbt::Backtrace().resolve().c_str(), rhs.bt);
    this_type(rhs).swap(*this);
    return *this;
  }

  // Move support

  // Move Constructor
  intrusive_ptr(intrusive_ptr&& rhs) noexcept : px(rhs.px), self(this)
  {
    // Ensure bt is properly initialized
    if (rhs.bt) {
      bt = bprintf("---- move constructor at\n%s\n-- Original object was allocated at\n%s",
                   simgrid::xbt::Backtrace().resolve().c_str(), rhs.bt);
    } else {
      bt = xbt_strdup(simgrid::xbt::Backtrace().resolve().c_str());
    }
    if (px) {
      px->reference_holder_.push(this);    // Maintain reference tracking
      rhs.px = nullptr;                    // Prevent double-release
      px->reference_holder_.pop(rhs.self); // Maintain reference tracking
    }
  }

  // Move Assignment Operator
  intrusive_ptr& operator=(intrusive_ptr&& rhs) noexcept
  {
    if (this != &rhs) {
      if (px)
        px->reference_holder_.pop(this); // Release previous reference
      px = rhs.px;

      if (rhs.bt) {
        px->reference_holder_.pop(rhs.self); // Release previous reference
        bt = bprintf("---- Move assignment operator at\n%s\n-- Original object was allocated at\n%s",
                     simgrid::xbt::Backtrace().resolve().c_str(), rhs.bt);
        free(rhs.bt);
        rhs.bt = nullptr;
      } else {
        bt = xbt_strdup(simgrid::xbt::Backtrace().resolve().c_str());
      }

      if (px)
        px->reference_holder_.push(this); // Track new reference
      rhs.px = nullptr;                   // Prevent rhs from releasing it again
    }
    return *this;
  }

  template <class U> friend class intrusive_ptr;

  template <class U> intrusive_ptr(intrusive_ptr<U>&& rhs) : px(rhs.px)
  {
    px->reference_holder_.pop(rhs);
    px->reference_holder_.push(this);
    bt     = bprintf("---- move_ctor at \n%s\n-- Orig: \n%s", simgrid::xbt::Backtrace().resolve().c_str(), rhs.bt);
    rhs.px = 0;
  }

  template <class U> intrusive_ptr& operator=(intrusive_ptr<U>&& rhs) noexcept
  {
    bt = bprintf("---- move_ctor at \n%s\n-- Orig: \n%s", simgrid::xbt::Backtrace().resolve().c_str(), rhs.bt);
    this_type(static_cast<intrusive_ptr<U>&&>(rhs)).swap(*this);
    return *this;
  }

  intrusive_ptr& operator=(intrusive_ptr const& rhs)
  {
    bt = bprintf("---- move_ctor at \n%s\n-- Orig: \n%s", simgrid::xbt::Backtrace().resolve().c_str(), rhs.bt);
    this_type(rhs).swap(*this);
    return *this;
  }

  intrusive_ptr& operator=(T* rhs)
  {
    bt = bprintf("---- operator=(T*) at \n%s", simgrid::xbt::Backtrace().resolve().c_str());
    if (px != nullptr) {
      px->reference_holder_.pop(this);
      intrusive_ptr_release(px);
    }
    px = rhs;
    if (px)
      px->reference_holder_.push(this); // Track new reference

    return *this;
  }

  void reset()
  {
    if (px != nullptr) {
      px->reference_holder_.pop(this);
      intrusive_ptr_release(px);
    }
    px = nullptr;
    free(bt);
  }

  void reset(T* rhs)
  {
    px->reference_holder_.pop(this);
    bt = bprintf("---- reset at \n%s\n-- Orig: \n%s", simgrid::xbt::Backtrace().resolve().c_str(), rhs->bt);
    this_type(rhs).swap(*this);
    px->reference_holder_.push(this);
  }

  void reset(T* rhs, bool add_ref)
  {
    px->reference_holder_.pop(this);
    bt = bprintf("---- reset at \n%s\n-- Orig: \n%s", simgrid::xbt::Backtrace().resolve().c_str(), rhs->bt);
    this_type(rhs, add_ref).swap(*this);
    px->reference_holder_.push(this);
  }

  T* get() const noexcept { return px; }

  T* detach() noexcept
  {
    T* ret = px;
    px     = 0;
    return ret;
  }

  T& operator*() const noexcept
  {
    xbt_assert(px != 0);
    return *px;
  }

  T* operator->() const noexcept
  {
    xbt_assert(px != 0);
    return px;
  }

  // implicit conversion to "bool"
  typedef element_type* this_type::* unspecified_bool_type;

  operator unspecified_bool_type() const noexcept { return px == 0 ? 0 : &this_type::px; }

  // operator! is redundant, but some compilers need it
  bool operator!() const noexcept { return px == 0; }

  void swap(intrusive_ptr& rhs) noexcept
  {
    T* tmp = px;
    px     = rhs.px;
    rhs.px = tmp;
  }
};

template <class T, class U> inline bool operator==(intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) noexcept
{
  return a.get() == b.get();
}

template <class T, class U> inline bool operator!=(intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) noexcept
{
  return a.get() != b.get();
}

template <class T, class U> inline bool operator==(intrusive_ptr<T> const& a, U* b) noexcept
{
  return a.get() == b;
}

template <class T, class U> inline bool operator!=(intrusive_ptr<T> const& a, U* b) noexcept
{
  return a.get() != b;
}

template <class T, class U> inline bool operator==(T* a, intrusive_ptr<U> const& b) noexcept
{
  return a == b.get();
}

template <class T, class U> inline bool operator!=(T* a, intrusive_ptr<U> const& b) noexcept
{
  return a != b.get();
}

template <class T> inline bool operator==(intrusive_ptr<T> const& p, std::nullptr_t) noexcept
{
  return p.get() == 0;
}

template <class T> inline bool operator==(std::nullptr_t, intrusive_ptr<T> const& p) noexcept
{
  return p.get() == 0;
}

template <class T> inline bool operator!=(intrusive_ptr<T> const& p, std::nullptr_t) noexcept
{
  return p.get() != 0;
}

template <class T> inline bool operator!=(std::nullptr_t, intrusive_ptr<T> const& p) noexcept
{
  return p.get() != 0;
}

template <class T> inline bool operator<(intrusive_ptr<T> const& a, intrusive_ptr<T> const& b) noexcept
{
  return std::less<T*>()(a.get(), b.get());
}

template <class T> void swap(intrusive_ptr<T>& lhs, intrusive_ptr<T>& rhs) noexcept
{
  lhs.swap(rhs);
}

// mem_fn support

template <class T> T* get_pointer(intrusive_ptr<T> const& p) noexcept
{
  return p.get();
}

// pointer casts

template <class T, class U> intrusive_ptr<T> static_pointer_cast(intrusive_ptr<U> const& p)
{
  return static_cast<T*>(p.get());
}

template <class T, class U> intrusive_ptr<T> const_pointer_cast(intrusive_ptr<U> const& p)
{
  return const_cast<T*>(p.get());
}

template <class T, class U> intrusive_ptr<T> dynamic_pointer_cast(intrusive_ptr<U> const& p)
{
  return dynamic_cast<T*>(p.get());
}

template <class T, class U> intrusive_ptr<T> static_pointer_cast(intrusive_ptr<U>&& p) noexcept
{
  return intrusive_ptr<T>(static_cast<T*>(p.detach()), false);
}

template <class T, class U> intrusive_ptr<T> const_pointer_cast(intrusive_ptr<U>&& p) noexcept
{
  return intrusive_ptr<T>(const_cast<T*>(p.detach()), false);
}

template <class T, class U> intrusive_ptr<T> dynamic_pointer_cast(intrusive_ptr<U>&& p) noexcept
{
  T* p2 = dynamic_cast<T*>(p.get());

  intrusive_ptr<T> r(p2, false);

  if (p2)
    p.detach();

  return r;
}

// operator<<

template <class E, class T, class Y>
std::basic_ostream<E, T>& operator<<(std::basic_ostream<E, T>& os, intrusive_ptr<Y> const& p)
{
  os << p.get();
  return os;
}

// Reference holder
template <class T> struct reference_holder {
  std::unordered_map<xbt::intrusive_ptr<T>*, const char*> references_;
  void push(xbt::intrusive_ptr<T>* ref)
  {
    references_[ref] = ref->get_bt();
    if (ref->get_bt() == nullptr) {
      XBT_CCRITICAL(root, "The bt of the pushed ref %p is null (refcount: %d). Current backtrace:", ref,
                    ref->get()->get_ref_count());
      xbt_backtrace_display_current();
    }
  }
  void pop(xbt::intrusive_ptr<T>* ref)
  {
    auto where = references_.find(ref);
    if (where == references_.end()) {
      XBT_CCRITICAL(root, "Cannot pop %p as it's not in the list", ref);
      xbt_backtrace_display_current();
    } else
      references_.erase(where);
  }

  void tell(const char* msg)
  {
    std::fprintf(stderr, ">>>> %s -- There is %zu references to %p\n", msg, references_.size(), this);
    int cpt = 0;
    for (auto [ref, bt] : references_) {
      std::fprintf(stderr, ">> ref #%d comes from %p\n", cpt++, ref);
      if (bt) {
        std::fprintf(stderr, "%s\n", bt);
      } else {
        std::fprintf(stderr, "(null bt)\n");
      }
    }
    std::fprintf(stderr, "<<<< end of the %d references to %p\n", cpt, this);
  }
};

// hash_value

template <class T> class hash;

template <class T> std::size_t hash_value(xbt::intrusive_ptr<T> const& p) noexcept
{
  return xbt::hash<T*>()(p.get());
}

} // namespace simgrid::xbt

// std::hash

namespace std {

template <class T> struct hash<simgrid::xbt::intrusive_ptr<T>> {
  std::size_t operator()(simgrid::xbt::intrusive_ptr<T> const& p) const noexcept { return std::hash<T*>()(p.get()); }
};

} // namespace std

#endif // #ifndef XBT_INTRUSIVE_PTR_HPP

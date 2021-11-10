/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SG_POPPING_PRIVATE_HPP
#define SG_POPPING_PRIVATE_HPP

#include "simgrid/forward.h"
#include "src/kernel/activity/ActivityImpl.hpp"

#include <array>
#include <boost/intrusive_ptr.hpp>

/********************************* Simcalls *********************************/
#include "popping_enum.hpp" /* Definition of Simcall, with one value per simcall */

XBT_PUBLIC_DATA const std::array<const char*, simgrid::simix::NUM_SIMCALLS> simcall_names; /* Name of each simcall */

using simix_match_func_t     = bool (*)(void*, void*, simgrid::kernel::activity::CommImpl*);
using simix_copy_data_func_t = void (*)(simgrid::kernel::activity::CommImpl*, void*, size_t);
using simix_clean_func_t     = void (*)(void*);
using FPtr                   = void (*)(); // Hide the ugliness

/* Pack all possible scalar types in an union */
union u_smx_scalar {
  bool b;
  char c;
  short s;
  int i;
  long l;
  long long ll;
  unsigned char uc;
  unsigned short us;
  unsigned int ui;
  unsigned long ul;
  unsigned long long ull;
  double d;
  void* dp;
  FPtr fp;
};

/**
 * @brief Represents a simcall to the kernel.
 */
struct s_smx_simcall {
  simgrid::simix::Simcall call_                      = simgrid::simix::Simcall::NONE;
  simgrid::kernel::actor::ActorImpl* issuer_                 = nullptr;
  std::shared_ptr<simgrid::kernel::timer::Timer> timeout_cb_ = nullptr; // Callback to timeouts
  simgrid::kernel::actor::SimcallObserver* observer_ = nullptr; // makes that simcall observable by the MC
  unsigned int mc_max_consider_ =
      0; // How many times this simcall should be used. If >1, this will be a fork in the state space.
  int mc_value_                           = 0;
  std::array<u_smx_scalar, 11> args_      = {};
  u_smx_scalar result_                    = {};
};

/******************************** General *************************************/

XBT_PRIVATE const char* SIMIX_simcall_name(const s_smx_simcall& simcall);
XBT_PRIVATE void SIMIX_run_kernel(std::function<void()> const* code);
XBT_PRIVATE void SIMIX_run_blocking(std::function<void()> const* code);

/* Defines the marshal/unmarshal functions for each type of parameters.
 *
 * They will be used in popping_accessors.hpp to define the functions allowing
 * to retrieve/set each parameter of each simcall.
 *
 * There is a unmarshal_raw() function, which is exactly similar to unmarshal()
 * for all types but boost::intrusive_ptr(T). For that type, the unmarshal()
 * function builds a new intrusive_ptr wrapping the pointer (that is stored raw
 * within the simcall) while the unmarshal_raw retrieves the raw pointer.
 *
 * This is used in <simcall>_getraw_<param> functions, that allow the
 * model-checker, to read the data in the remote memory of the MCed.
 */

namespace simgrid {
namespace simix {

template <class T> class type {
  constexpr bool operator==(type) const { return true; }
  template <class U> constexpr bool operator==(type<U>) const { return false; }
  constexpr bool operator!=(type) const { return false; }
  template <class U> constexpr bool operator!=(type<U>) const { return true; }
};

template <typename T> struct marshal_t {
};
#define SIMIX_MARSHAL(T, field)                                                                                        \
  inline void marshal(type<T>, u_smx_scalar& simcall, T value) { simcall.field = value; }                              \
  inline T unmarshal(type<T>, u_smx_scalar const& simcall) { return simcall.field; }                                   \
  inline T unmarshal_raw(type<T>, u_smx_scalar const& simcall)                                                         \
  { /* Exactly same as unmarshal. It differs only for intrusive_ptr */ return simcall.field; }

SIMIX_MARSHAL(bool, b)
SIMIX_MARSHAL(char, c)
SIMIX_MARSHAL(short, s)
SIMIX_MARSHAL(int, i)
SIMIX_MARSHAL(long, l)
SIMIX_MARSHAL(unsigned char, uc)
SIMIX_MARSHAL(unsigned short, us)
SIMIX_MARSHAL(unsigned int, ui)
SIMIX_MARSHAL(unsigned long, ul)
SIMIX_MARSHAL(unsigned long long, ull)
SIMIX_MARSHAL(long long, ll)
SIMIX_MARSHAL(float, d)
SIMIX_MARSHAL(double, d)
SIMIX_MARSHAL(FPtr, fp)

inline void unmarshal(type<void>, u_smx_scalar const& /*simcall*/)
{
  /* Nothing to do for void data */
}
inline void unmarshal_raw(type<void>, u_smx_scalar const& /*simcall*/)
{
  /* Nothing to do for void data */
}

template <class T> inline void marshal(type<T*>, u_smx_scalar& simcall, T* value)
{
  simcall.dp = (void*)value;
}
template <class T> inline T* unmarshal(type<T*>, u_smx_scalar const& simcall)
{
  return static_cast<T*>(simcall.dp);
}
template <class T> inline T* unmarshal_raw(type<T*>, u_smx_scalar const& simcall)
{
  return static_cast<T*>(simcall.dp);
}

template <class T>
inline void marshal(type<boost::intrusive_ptr<T>>, u_smx_scalar& simcall, boost::intrusive_ptr<T> value)
{
  if (value.get() == nullptr) { // Sometimes we return nullptr in an intrusive_ptr...
    simcall.dp = nullptr;
  } else {
    intrusive_ptr_add_ref(value.get());
    simcall.dp = static_cast<void*>(value.get());
  }
}
template <class T> inline boost::intrusive_ptr<T> unmarshal(type<boost::intrusive_ptr<T>>, u_smx_scalar const& simcall)
{
  // refcount was already increased during the marshaling, thus the "false" as last argument
  boost::intrusive_ptr<T> res = boost::intrusive_ptr<T>(static_cast<T*>(simcall.dp), false);
  return res;
}
template <class T> inline T* unmarshal_raw(type<boost::intrusive_ptr<T>>, u_smx_scalar const& simcall)
{
  return static_cast<T*>(simcall.dp);
}

template <class R, class... T> inline void marshal(type<R (*)(T...)>, u_smx_scalar& simcall, R (*value)(T...))
{
  simcall.fp = (FPtr)value;
}
template <class R, class... T> inline auto unmarshal(type<R (*)(T...)>, u_smx_scalar simcall) -> R (*)(T...)
{
  return (R(*)(T...))simcall.fp;
}
template <class R, class... T> inline auto unmarshal_raw(type<R (*)(T...)>, u_smx_scalar simcall) -> R (*)(T...)
{
  return (R(*)(T...))simcall.fp;
}

template <class T> inline void marshal(u_smx_scalar& simcall, T const& value)
{
  return marshal(type<T>(), simcall, value);
}
template <class T> inline typename std::remove_reference_t<T> unmarshal(u_smx_scalar& simcall)
{
  return unmarshal(type<T>(), simcall);
}
template <class T> inline typename std::remove_reference_t<T> unmarshal_raw(u_smx_scalar& simcall)
{
  return unmarshal(type<T>(), simcall);
}

template <std::size_t I> inline void marshal_args(const s_smx_simcall* /*simcall*/)
{
  /* Nothing to do when no args */
}

template <std::size_t I, class A> inline void marshal_args(smx_simcall_t simcall, A const& a)
{
  marshal(simcall->args_[I], a);
}

template <std::size_t I, class A, class... B> inline void marshal_args(smx_simcall_t simcall, A const& a, B const&... b)
{
  marshal(simcall->args_[I], a);
  marshal_args<I + 1>(simcall, b...);
}

/** Initialize the simcall */
template <class... A> inline void marshal(smx_simcall_t simcall, Simcall call, A const&... a)
{
  simcall->call_ = call;
  memset(&simcall->result_, 0, sizeof simcall->result_);
  memset(simcall->args_.data(), 0, simcall->args_.size() * sizeof simcall->args_[0]);
  marshal_args<0>(simcall, a...);
}
}
}

#include "popping_accessors.hpp"

#endif

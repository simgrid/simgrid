/* Copyright (c) 2007-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _POPPING_PRIVATE_H
#define _POPPING_PRIVATE_H

#include <xbt/base.h>
#include <simgrid/simix.h>

SG_BEGIN_DECL()

/********************************* Simcalls *********************************/
XBT_PUBLIC_DATA(const char*) simcall_names[]; /* Name of each simcall */

#include "popping_enum.h" /* Definition of e_smx_simcall_t, with one value per simcall */

typedef int (*simix_match_func_t)(void *, void *, smx_synchro_t);
typedef void (*simix_copy_data_func_t)(smx_synchro_t, void*, size_t);
typedef void (*simix_clean_func_t)(void *);
typedef void (*FPtr)(void); // Hide the ugliness

/* Pack all possible scalar types in an union */
union u_smx_scalar {
  char            c;
  const char*     cc;
  short           s;
  int             i;
  long            l;
  unsigned char   uc;
  unsigned short  us;
  unsigned int    ui;
  unsigned long   ul;
  float           f;
  double          d;
  size_t          sz;
  sg_size_t       sgsz;
  sg_offset_t     sgoff;
  void*           dp;
  FPtr            fp;
  const void*     cp;
};

/**
 * \brief Represents a simcall to the kernel.
 */
typedef struct s_smx_simcall {
  e_smx_simcall_t call;
  smx_process_t issuer;
  int mc_value;
  union u_smx_scalar args[11];
  union u_smx_scalar result;
} s_smx_simcall_t, *smx_simcall_t;

#define SIMCALL_SET_MC_VALUE(simcall, value) ((simcall)->mc_value = (value))
#define SIMCALL_GET_MC_VALUE(simcall) ((simcall)->mc_value)

#include "popping_accessors.h"

/******************************** General *************************************/

XBT_PRIVATE void SIMIX_simcall_answer(smx_simcall_t);
XBT_PRIVATE void SIMIX_simcall_handle(smx_simcall_t, int);
XBT_PRIVATE void SIMIX_simcall_exit(smx_synchro_t);
XBT_PRIVATE const char *SIMIX_simcall_name(e_smx_simcall_t kind);
XBT_PRIVATE void SIMIX_run_kernel(void* code);

SG_END_DECL()

#ifdef __cplusplus

namespace simgrid {
namespace simix {

template<typename T> struct marshal_t {};
#define SIMIX_MARSHAL(T, field) \
  template<> struct marshal_t<T> { \
    static void marshal(u_smx_scalar& simcall, T value) \
    { \
      simcall.field = value; \
    } \
    static T unmarshal(u_smx_scalar const& simcall) \
    { \
      return simcall.field; \
    } \
  };

SIMIX_MARSHAL(char, c);
SIMIX_MARSHAL(short, s);
SIMIX_MARSHAL(int, i);
SIMIX_MARSHAL(long, l);
SIMIX_MARSHAL(unsigned char, uc);
SIMIX_MARSHAL(unsigned short, us);
SIMIX_MARSHAL(unsigned int, ui);
SIMIX_MARSHAL(unsigned long, ul);
SIMIX_MARSHAL(float, f);
SIMIX_MARSHAL(double, d);
SIMIX_MARSHAL(FPtr, fp);

template<typename T> struct marshal_t<T*> {
  static void marshal(u_smx_scalar& simcall, T* value)
  {
    simcall.dp = value;
  }
  static T* unmarshal(u_smx_scalar const& simcall)
  {
    return simcall.dp;
  }
};

template<> struct marshal_t<void> {
  static void unmarshal(u_smx_scalar const& simcall) {}
};

template<class T> inline
void marshal(u_smx_scalar& simcall, T const& value)
{
  marshal_t<T>::marshal(simcall, value);
}

template<class T> inline
T unmarshal(u_smx_scalar const& simcall)
{
  return marshal_t<T>::unmarshal(simcall);
}

template<class A> inline
void marshal(smx_simcall_t simcall, e_smx_simcall_t call, A&& a)
{
  simcall->call = call;
  memset(&simcall->result, 0, sizeof(simcall->result));
  marshal(simcall->args[0], std::forward<A>(a));
}

template<class A, class B> inline
void marshal(smx_simcall_t simcall, e_smx_simcall_t call, A&& a, B&& b)
{
  simcall->call = call;
  memset(&simcall->result, 0, sizeof(simcall->result));
  marshal(simcall->args[0], std::forward<A>(a));
  marshal(simcall->args[1], std::forward<B>(b));
}

template<class A, class B, class C> inline
void marshal(smx_simcall_t simcall, e_smx_simcall_t call, A&& a, B&& b, C&& c)
{
  simcall->call = call;
  memset(&simcall->result, 0, sizeof(simcall->result));
  marshal(simcall->args[0], std::forward<A>(a));
  marshal(simcall->args[1], std::forward<B>(b));
  marshal(simcall->args[2], std::forward<C>(c));
}

template<class A, class B, class C, class D> inline
void marshal(smx_simcall_t simcall, e_smx_simcall_t call, A&& a, B&& b, C&& c, D&& d)
{
  simcall->call = call;
  memset(&simcall->result, 0, sizeof(simcall->result));
  marshal(simcall->args[0], std::forward<A>(a));
  marshal(simcall->args[1], std::forward<B>(b));
  marshal(simcall->args[2], std::forward<C>(c));
  marshal(simcall->args[3], std::forward<D>(d));
}

template<class A, class B, class C, class D, class E> inline
void marshal(smx_simcall_t simcall, e_smx_simcall_t call, A&& a, B&& b, C&& c, D&& d, E&& e)
{
  simcall->call = call;
  memset(&simcall->result, 0, sizeof(simcall->result));
  marshal(simcall->args[0], std::forward<A>(a));
  marshal(simcall->args[1], std::forward<B>(b));
  marshal(simcall->args[2], std::forward<C>(c));
  marshal(simcall->args[3], std::forward<D>(d));
  marshal(simcall->args[4], std::forward<E>(e));
}

template<class A, class B, class C, class D, class E, class F> inline
void marshal(smx_simcall_t simcall, e_smx_simcall_t call,
  A&& a, B&& b, C&& c, D&& d, E&& e, F&& f)
{
  simcall->call = call;
  memset(&simcall->result, 0, sizeof(simcall->result));
  marshal(simcall->args[0], std::forward<A>(a));
  marshal(simcall->args[1], std::forward<B>(b));
  marshal(simcall->args[2], std::forward<C>(c));
  marshal(simcall->args[3], std::forward<D>(d));
  marshal(simcall->args[4], std::forward<E>(e));
  marshal(simcall->args[5], std::forward<F>(f));
}

template<class A, class B, class C, class D, class E, class F, class G> inline
void marshal(smx_simcall_t simcall, e_smx_simcall_t call,
  A&& a, B&& b, C&& c, D&& d, E&& e, F&& f, G&& g)
{
  simcall->call = call;
  memset(&simcall->result, 0, sizeof(simcall->result));
  marshal(simcall->args[0], std::forward<A>(a));
  marshal(simcall->args[1], std::forward<B>(b));
  marshal(simcall->args[2], std::forward<C>(c));
  marshal(simcall->args[3], std::forward<D>(d));
  marshal(simcall->args[4], std::forward<E>(e));
  marshal(simcall->args[5], std::forward<F>(f));
  marshal(simcall->args[6], std::forward<G>(g));
}

template<class A, class B, class C,
          class D, class E, class F, class G, class H> inline
void marshal(smx_simcall_t simcall, e_smx_simcall_t call,
  A&& a, B&& b, C&& c, D&& d, E&& e, F&& f, G&& g, H&& h)
{
  simcall->call = call;
  memset(&simcall->result, 0, sizeof(simcall->result));
  marshal(simcall->args[0], std::forward<A>(a));
  marshal(simcall->args[1], std::forward<B>(b));
  marshal(simcall->args[2], std::forward<C>(c));
  marshal(simcall->args[3], std::forward<D>(d));
  marshal(simcall->args[4], std::forward<E>(e));
  marshal(simcall->args[5], std::forward<F>(f));
  marshal(simcall->args[6], std::forward<G>(g));
  marshal(simcall->args[7], std::forward<H>(h));
}

template<class A, class B, class C,
          class D, class E, class F,
          class G, class H, class I> inline
void marshal(smx_simcall_t simcall, e_smx_simcall_t call,
  A&& a, B&& b, C&& c, D&& d, E&& e, F&& f, G&& g, H&& h, I&& i)
{
  simcall->call = call;
  memset(&simcall->result, 0, sizeof(simcall->result));
  marshal(simcall->args[0], std::forward<A>(a));
  marshal(simcall->args[1], std::forward<B>(b));
  marshal(simcall->args[2], std::forward<C>(c));
  marshal(simcall->args[3], std::forward<D>(d));
  marshal(simcall->args[4], std::forward<E>(e));
  marshal(simcall->args[5], std::forward<F>(f));
  marshal(simcall->args[6], std::forward<G>(g));
  marshal(simcall->args[7], std::forward<H>(h));
  marshal(simcall->args[8], std::forward<I>(i));
}

}
}

#endif

#endif

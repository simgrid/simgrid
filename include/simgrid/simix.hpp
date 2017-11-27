/* Copyright (c) 2007-2010, 2012-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SIMIX_HPP
#define SIMGRID_SIMIX_HPP

#include <simgrid/simix.h>
#include <xbt/functional.hpp>
#include <xbt/future.hpp>
#include <xbt/signal.hpp>

#include <map>
#include <string>

XBT_PUBLIC(void) simcall_run_kernel(std::function<void()> const& code);

/** Execute some code in the kernel and block
 *
 * run_blocking() is a generic blocking simcall. It is given a callback
 * which is executed immediately in the SimGrid kernel. The callback is
 * responsible for setting the suitable logic for waking up the process
 * when needed.
 *
 * @ref simix::kernelSync() is a higher level wrapper for this.
 */
XBT_PUBLIC(void) simcall_run_blocking(std::function<void()> const& code);

template<class F> inline
void simcall_run_kernel(F& f)
{
  simcall_run_kernel(std::function<void()>(std::ref(f)));
}
template<class F> inline
void simcall_run_blocking(F& f)
{
  simcall_run_blocking(std::function<void()>(std::ref(f)));
}

namespace simgrid {

namespace simix {

/** Execute some code in the kernel/maestro
 *
 *  This can be used to enforce mutual exclusion with other simcall.
 *  More importantly, this enforces a deterministic/reproducible ordering
 *  of the operation with respect to other simcalls.
 */
template<class F>
typename std::result_of<F()>::type kernelImmediate(F&& code)
{
  // If we are in the maestro, we take the fast path and execute the
  // code directly without simcall mashalling/unmarshalling/dispatch:
  if (SIMIX_is_maestro())
    return std::forward<F>(code)();

  // If we are in the application, pass the code to the maestro which
  // executes it for us and reports the result. We use a std::future which
  // conveniently handles the success/failure value for us.
  typedef typename std::result_of<F()>::type R;
  simgrid::xbt::Result<R> result;
  simcall_run_kernel([&]{
    xbt_assert(SIMIX_is_maestro(), "Not in maestro");
    simgrid::xbt::fulfillPromise(result, std::forward<F>(code));
  });
  return result.get();
}

XBT_PUBLIC(const std::vector<smx_actor_t>&) process_get_runnable();

XBT_PUBLIC(void) set_maestro(std::function<void()> code);
XBT_PUBLIC(void) create_maestro(std::function<void()> code);

// What's executed as SIMIX actor code:
typedef std::function<void()> ActorCode;

// Create ActorCode based on argv:
typedef std::function<ActorCode(std::vector<std::string> args)> ActorCodeFactory;

XBT_PUBLIC(void) registerFunction(const char* name, ActorCodeFactory factory);

/** These functions will be called when we detect a deadlock: any remaining process is locked on an action
 *
 * If these functions manage to unlock some of the processes, then the deadlock will be avoided.
 */
extern simgrid::xbt::signal<void()> onDeadlock;
}
}

/*
 * Type of function that creates a process.
 * The function must accept the following parameters:
 * void* process: the process created will be stored there
 * const char *name: a name for the object. It is for user-level information and can be NULL
 * xbt_main_func_t code: is a function describing the behavior of the process
 * void *data: data a pointer to any data one may want to attach to the new object.
 * sg_host_t host: the location where the new process is executed
 * int argc, char **argv: parameters passed to code
 * std::map<std::string, std::string>* props: properties
 */
typedef smx_actor_t (*smx_creation_func_t)(
    /* name */ const char*, std::function<void()> code,
    /* userdata */ void*,
    /* hostname */ sg_host_t,
    /* props */ std::map<std::string, std::string>*,
    /* parent_process */ smx_actor_t);

extern "C"
XBT_PUBLIC(void) SIMIX_function_register_process_create(smx_creation_func_t function);

XBT_PUBLIC(smx_actor_t)
simcall_process_create(const char* name, std::function<void()> code, void* data, sg_host_t host,
                       std::map<std::string, std::string>* properties);

XBT_PUBLIC(smx_timer_t) SIMIX_timer_set(double date, simgrid::xbt::Task<void()> callback);

template<class F> inline
smx_timer_t SIMIX_timer_set(double date, F callback)
{
  return SIMIX_timer_set(date, simgrid::xbt::Task<void()>(std::move(callback)));
}

template<class R, class T> inline
smx_timer_t SIMIX_timer_set(double date, R(*callback)(T*), T* arg)
{
  return SIMIX_timer_set(date, [=](){ callback(arg); });
}

#endif

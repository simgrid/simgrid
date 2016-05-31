/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SIMIX_BLOCKING_SIMCALL_HPP
#define SIMGRID_SIMIX_BLOCKING_SIMCALL_HPP

#include <iostream>

#include <exception>

#include <xbt/sysdep.h>

#include <future>

#include <xbt/future.hpp>
#include <simgrid/kernel/future.hpp>
#include <simgrid/simix.h>

XBT_PUBLIC(void) simcall_run_blocking(std::function<void()> const& code);

namespace simgrid {
namespace simix {

XBT_PUBLIC(void) unblock(smx_process_t process);

/** Execute some code in kernel mode and wakes up the process when
 *  the result is available.
 *
 *  The code given is executed in SimGrid kernel and expected to return
 *  a `simgrid::kernel::Future`. The current process is resumed whenever
 *  the Future becomes ready and gets the value or exception of the future:
 *
 *  This can be used to implement blocking calls in without adding new simcalls.
 *  One downside of this approach is that we don't have any semantic on what
 *  the process is waiting. This might be a problem for the model-checker and
 *  we'll have to device a way to make it work.
 *
 *  @param     code Kernel code returning a `simgrid::kernel::Future<T>`
 *  @return         Value of the kernel future
 *  @exception      Exception from the kernel future
 */
template<class F>
auto blocking_simcall(F code) -> decltype(code().get())
{
  typedef decltype(code().get()) T;
  if (SIMIX_is_maestro())
    xbt_die("Can't execute blocking call in kernel mode");

  smx_process_t self = SIMIX_process_self();
  simgrid::xbt::Result<T> result;

  simcall_run_blocking([&result, self, &code]{
    try {
      auto future = code();
      future.then([&result, self](simgrid::kernel::Future<T> value) {
        simgrid::xbt::setPromise(result, value);
        simgrid::simix::unblock(self);
      });
    }
    catch (...) {
      result.set_exception(std::current_exception());
      simgrid::simix::unblock(self);
    }
  });
  return result.get();
}

}
}

#endif

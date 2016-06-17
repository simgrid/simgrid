/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <future>
#include <list>

#include <xbt/future.hpp>

#include <simgrid/simix.hpp>
#include <simgrid/simix/blocking_simcall.hpp>
#include <simgrid/kernel/future.hpp>
#include <xbt/log.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "my log messages");

namespace example {

/** Execute the code in the kernel at some time
 *
 *  @param date  when we should execute the code
 *  @param code  code to execute
 *  @return      future with the result of the call
 */
template<class F>
auto kernel_defer(double date, F code) -> simgrid::kernel::Future<decltype(code())>
{
  typedef decltype(code()) T;
  auto promise = std::make_shared<simgrid::kernel::Promise<T>>();
  auto future = promise->get_future();
  SIMIX_timer_set(date, [promise, code] {
    simgrid::xbt::fulfillPromise(*promise, std::move(code));
  });
  return future;
}

static int master(int argc, char *argv[])
{
  // Test the simple immediate execution:
  XBT_INFO("Start");
  simgrid::simix::kernelImmediate([] {
    XBT_INFO("kernel");
  });
  XBT_INFO("kernel, returned");

  // Synchronize on a successful Future<void>:
  simgrid::simix::kernelSync([&] {
    return kernel_defer(10, [] {
      XBT_INFO("kernelSync with void");
    });
  });
  XBT_INFO("kernelSync with void, returned");

  // Synchronize on a failing Future<void>:
  try {
    simgrid::simix::kernelSync([&] {
      return kernel_defer(20, [] {
        throw std::runtime_error("Exception throwed from kernel_defer");
      });
    });
    XBT_ERROR("No exception caught!");
  }
  catch(std::runtime_error& e) {
    XBT_INFO("Exception caught: %s", e.what());
  }

  // Synchronize on a successul Future<int> and get the value:
  int res = simgrid::simix::kernelSync([&] {
    return kernel_defer(30, [] {
      XBT_INFO("kernelSync with value");
      return 42;
    });
  });
  XBT_INFO("kernelSync with value returned with %i", res);

  // Synchronize on a successul Future<int> and get the value:
  simgrid::simix::Future<int> future = simgrid::simix::kernelAsync([&] {
    return kernel_defer(50, [] {
      XBT_INFO("kernelAsync with value");
      return 43;
    });
  });
  res = future.get();
  XBT_INFO("kernelAsync with value returned with %i", res);

  return 0;
}

}

int main(int argc, char *argv[])
{
  SIMIX_global_init(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform.xml\n", argv[0]);
  SIMIX_function_register("master", example::master);
  SIMIX_create_environment(argv[1]);
  simcall_process_create("master", example::master, NULL, "Tremblay", -1, 0, NULL, NULL, 0);
  SIMIX_run();
  return 0;
}

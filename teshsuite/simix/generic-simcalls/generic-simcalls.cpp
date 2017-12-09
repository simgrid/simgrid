/* Copyright (c) 2016-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <memory>
#include <stdexcept>

#include <xbt/future.hpp>

#include <simgrid/kernel/future.hpp>
#include <simgrid/simix.hpp>
#include <simgrid/simix/blocking_simcall.hpp>
#include <xbt/log.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "my log messages");

namespace example {

class exception : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

/** Create a future which becomes ready when the date is reached */
static simgrid::kernel::Future<void> kernel_wait_until(double date)
{
  auto promise = std::make_shared<simgrid::kernel::Promise<void>>();
  auto future  = promise->get_future();
  SIMIX_timer_set(date, [promise] { promise->set_value(); });
  return future;
}

static int master(int argc, char* argv[])
{
  // Test the simple immediate execution:
  XBT_INFO("Start");
  simgrid::simix::kernelImmediate([] { XBT_INFO("kernel"); });
  XBT_INFO("kernel, returned");

  // Synchronize on a successful Future<void>:
  simgrid::simix::kernelSync([] {
    return kernel_wait_until(10).then([](simgrid::kernel::Future<void> future) {
      future.get();
      XBT_INFO("kernelSync with void");
    });
  });
  XBT_INFO("kernelSync with void, returned");

  // Synchronize on a failing Future<void>:
  try {
    simgrid::simix::kernelSync([] {
      return kernel_wait_until(20).then([](simgrid::kernel::Future<void> future) {
        future.get();
        throw example::exception("Exception throwed from kernel_defer");
      });
    });
    XBT_ERROR("No exception caught!");
  } catch (const example::exception& e) {
    XBT_INFO("Exception caught: %s", e.what());
  }

  // Synchronize on a successul Future<int> and get the value:
  int res = simgrid::simix::kernelSync([] {
    return kernel_wait_until(30).then([](simgrid::kernel::Future<void> future) {
      future.get();
      XBT_INFO("kernelSync with value");
      return 42;
    });
  });
  XBT_INFO("kernelSync with value returned with %i", res);

  // Synchronize on a successul Future<int> and get the value:
  simgrid::simix::Future<int> future = simgrid::simix::kernelAsync([] {
    return kernel_wait_until(50).then([](simgrid::kernel::Future<void> future) {
      future.get();
      XBT_INFO("kernelAsync with value");
      return 43;
    });
  });
  res = future.get();
  XBT_INFO("kernelAsync with value returned with %i", res);

  // Synchronize on a successul Future<int> and get the value:
  future = simgrid::simix::kernelAsync([] {
    return kernel_wait_until(60).then([](simgrid::kernel::Future<void> future) {
      future.get();
      XBT_INFO("kernelAsync with value");
      return 43;
    });
  });
  XBT_INFO("The future is %s", future.is_ready() ? "ready" : "not ready");
  future.wait();
  XBT_INFO("The future is %s", future.is_ready() ? "ready" : "not ready");
  res = future.get();
  XBT_INFO("kernelAsync with value returned with %i", res);

  return 0;
}
}

int main(int argc, char* argv[])
{
  SIMIX_global_init(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform.xml\n", argv[0]);
  SIMIX_function_register("master", example::master);
  SIMIX_create_environment(argv[1]);
  simcall_process_create("master", example::master, NULL, sg_host_by_name("Tremblay"), 0, NULL, NULL);
  SIMIX_run();
  return 0;
}

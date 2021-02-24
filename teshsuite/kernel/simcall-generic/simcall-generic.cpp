/* Copyright (c) 2016-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <memory>
#include <stdexcept>

#include <xbt/promise.hpp>

#include <simgrid/kernel/future.hpp>
#include <simgrid/s4u/Actor.hpp>
#include <simgrid/s4u/Engine.hpp>
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
  simgrid::simix::Timer::set(date, [promise] { promise->set_value(); });
  return future;
}

static void master()
{
  // Test the simple immediate execution:
  XBT_INFO("Start");
  simgrid::kernel::actor::simcall([] { XBT_INFO("kernel"); });
  XBT_INFO("kernel, returned");

  // Synchronize on a successful Future<void>:
  simgrid::simix::kernel_sync([] {
    return kernel_wait_until(10).then([](simgrid::kernel::Future<void> f) {
      f.get();
      XBT_INFO("kernel_sync with void");
    });
  });
  XBT_INFO("kernel_sync with void, returned");

  // Synchronize on a failing Future<void>:
  try {
    simgrid::simix::kernel_sync([] {
      return kernel_wait_until(20).then([](simgrid::kernel::Future<void> f) {
        f.get();
        throw example::exception("Exception thrown from kernel_defer");
      });
    });
    XBT_ERROR("No exception caught!");
  } catch (const example::exception& e) {
    XBT_INFO("Exception caught: %s", e.what());
  }

  // Synchronize on a successful Future<int> and get the value:
  int res = simgrid::simix::kernel_sync([] {
    return kernel_wait_until(30).then([](simgrid::kernel::Future<void> f) {
      f.get();
      XBT_INFO("kernel_sync with value");
      return 42;
    });
  });
  XBT_INFO("kernel_sync with value returned with %i", res);

  // Synchronize on a successful Future<int> and get the value:
  simgrid::simix::Future<int> future = simgrid::simix::kernel_async([] {
    return kernel_wait_until(50).then([](simgrid::kernel::Future<void> f) {
      f.get();
      XBT_INFO("kernel_async with value");
      return 43;
    });
  });
  res = future.get();
  XBT_INFO("kernel_async with value returned with %i", res);

  // Synchronize on a successful Future<int> and get the value:
  future = simgrid::simix::kernel_async([] {
    return kernel_wait_until(60).then([](simgrid::kernel::Future<void> f) {
      f.get();
      XBT_INFO("kernel_async with value");
      return 43;
    });
  });
  XBT_INFO("The future is %s", future.is_ready() ? "ready" : "not ready");
  future.wait();
  XBT_INFO("The future is %s", future.is_ready() ? "ready" : "not ready");
  res = future.get();
  XBT_INFO("kernel_async with value returned with %i", res);
}
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform.xml\n", argv[0]);
  e.load_platform(argv[1]);
  simgrid::s4u::Actor::create("master", e.host_by_name("Tremblay"), example::master);
  e.run();
  return 0;
}

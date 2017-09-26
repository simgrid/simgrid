/* Copyright (c) 2005-2017. The SimGrid Team.
 * All rights reserved. */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdlib>

#include <atomic>
#include <exception>
#include <string>
#include <typeinfo>
#include <vector>
#include <memory>
#include <mutex>

#include <xbt/backtrace.hpp>
#include <xbt/config.hpp>
#include <xbt/ex.hpp>
#include <xbt/exception.hpp>
#include <xbt/log.h>
#include <xbt/log.hpp>

extern "C" {
XBT_LOG_EXTERNAL_CATEGORY(xbt);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_exception, xbt, "Exceptions");
}

namespace simgrid {
namespace xbt {

WithContextException::~WithContextException() = default;

void logException(
  e_xbt_log_priority_t prio,
  const char* context, std::exception const& exception)
{
  try {
    auto name = simgrid::xbt::demangle(typeid(exception).name());

    auto with_context =
      dynamic_cast<const simgrid::xbt::WithContextException*>(&exception);
    if (with_context != nullptr)
      XBT_LOG(prio, "%s %s by %s/%d: %s",
        context, name.get(),
        with_context->processName().c_str(), with_context->pid(),
        exception.what());
    else
      XBT_LOG(prio, "%s %s: %s", context, name.get(), exception.what());

    // Do we have a backtrace?
    if (with_context != nullptr && not xbt_cfg_get_boolean("exception/cutpath")) {
      auto backtrace = simgrid::xbt::resolveBacktrace(
        with_context->backtrace().data(), with_context->backtrace().size());
      for (std::string const& s : backtrace)
        XBT_LOG(prio, "  -> %s", s.c_str());
    }

    // Do we have a nested exception?
    auto with_nested = dynamic_cast<const std::nested_exception*>(&exception);
    if (with_nested == nullptr ||  with_nested->nested_ptr() == nullptr)
      return;
    try {
      with_nested->rethrow_nested();
    }
    catch (std::exception& nested_exception) {
      logException(prio, "Caused by", nested_exception);
    }
    // We could catch nested_exception or WithContextException but we don't bother:
    catch (...) {
      XBT_LOG(prio, "Caused by an unknown exception");
    }
  }
  catch (...) {
    // Don't log exceptions we got when trying to log exception
  }
}

static void showBacktrace(std::vector<xbt_backtrace_location_t>& bt)
{
  if (xbt_cfg_get_boolean("exception/cutpath")) {
    XBT_LOG(xbt_log_priority_critical, "Display of current backtrace disabled by --cfg=exception/cutpath.");
    return;
  }
  std::vector<std::string> res = resolveBacktrace(&bt[0], bt.size());
  XBT_LOG(xbt_log_priority_critical, "Current backtrace:");
  for (std::string const& s : res)
    XBT_LOG(xbt_log_priority_critical, "  -> %s", s.c_str());
}

static std::terminate_handler previous_terminate_handler = nullptr;

static void handler()
{
  // Avoid doing crazy things if we get an uncaught exception inside
  // an uncaught exception
  static std::atomic_flag lock = ATOMIC_FLAG_INIT;
  if (lock.test_and_set()) {
    XBT_ERROR("Handling an exception raised an exception. Bailing out.");
    std::abort();
  }

  // Get the current backtrace and exception
  auto e = std::current_exception();
  auto bt = backtrace();
  try {
    std::rethrow_exception(e);
  }

  // We manage C++ exception ourselves
  catch (std::exception& e) {
    logException(xbt_log_priority_critical, "Uncaught exception", e);
    showBacktrace(bt);
    std::abort();
  }

  // We don't know how to manage other exceptions
  catch (...) {
    // If there was another handler let's delegate to it
    if (previous_terminate_handler)
      previous_terminate_handler();
    else {
      XBT_ERROR("Unknown uncaught exception");
      showBacktrace(bt);
      std::abort();
    }
  }

}

void installExceptionHandler()
{
  static std::once_flag handler_flag;
  std::call_once(handler_flag, [] {
    previous_terminate_handler = std::set_terminate(handler);
  });
}

}
}

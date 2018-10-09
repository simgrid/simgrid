/* Copyright (c) 2005-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include <xbt/config.hpp>
#include <xbt/log.hpp>

#include <mutex>
#include <sstream>

XBT_LOG_EXTERNAL_CATEGORY(xbt);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_exception, xbt, "Exceptions");

// DO NOT define ~xbt_ex() in exception.hpp.
// Defining it here ensures that xbt_ex is defined only in libsimgrid, but not in libsimgrid-java.
// Doing otherwise naturally breaks things (at least on freebsd with clang).

xbt_ex::~xbt_ex() = default;

void _xbt_throw(char* message, xbt_errcat_t errcat, int value, const char* file, int line, const char* func)
{
  xbt_ex e(simgrid::xbt::ThrowPoint(file, line, func, simgrid::xbt::backtrace(), xbt_procname(), xbt_getpid()),
           message);
  xbt_free(message);
  e.category = errcat;
  e.value    = value;
  throw e;
}

/** @brief shows an exception content and the associated stack if available */
void xbt_ex_display(xbt_ex_t* e)
{
  simgrid::xbt::log_exception(xbt_log_priority_critical, "UNCAUGHT EXCEPTION", *e);
}

/** @brief returns a short name for the given exception category */
const char* xbt_ex_catname(xbt_errcat_t cat)
{
  switch (cat) {
    case unknown_error:
      return "unknown error";
    case arg_error:
      return "invalid argument";
    case bound_error:
      return "out of bounds";
    case mismatch_error:
      return "mismatch";
    case not_found_error:
      return "not found";
    case system_error:
      return "system error";
    case network_error:
      return "network error";
    case timeout_error:
      return "timeout";
    case cancel_error:
      return "action canceled";
    case thread_error:
      return "thread error";
    case host_error:
      return "host failed";
    case tracing_error:
      return "tracing error";
    case io_error:
      return "io error";
    case vm_error:
      return "vm error";
    default:
      return "INVALID ERROR";
  }
  return "INVALID ERROR";
}

namespace simgrid {
namespace xbt {

void log_exception(e_xbt_log_priority_t prio, const char* context, std::exception const& exception)
{
  try {
    auto name = simgrid::xbt::demangle(typeid(exception).name());

    auto* with_context = dynamic_cast<const simgrid::Exception*>(&exception);
    if (with_context != nullptr)
      XBT_LOG(prio, "%s %s by %s/%d: %s", context, name.get(), with_context->throw_point().procname_.c_str(),
              with_context->throw_point().pid_, exception.what());
    else
      XBT_LOG(prio, "%s %s: %s", context, name.get(), exception.what());

    // Do we have a backtrace?
    if (with_context != nullptr && not simgrid::config::get_value<bool>("exception/cutpath")) {
      auto backtrace = simgrid::xbt::resolve_backtrace(with_context->throw_point().backtrace_.data(),
                                                       with_context->throw_point().backtrace_.size());
      for (std::string const& s : backtrace)
        XBT_LOG(prio, "  -> %s", s.c_str());
    }

    // Do we have a nested exception?
    auto* with_nested = dynamic_cast<const std::nested_exception*>(&exception);
    if (with_nested == nullptr ||  with_nested->nested_ptr() == nullptr)
      return;
    try {
      with_nested->rethrow_nested();
    }
    catch (std::exception& nested_exception) {
      log_exception(prio, "Caused by", nested_exception);
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
  if (simgrid::config::get_value<bool>("exception/cutpath")) {
    XBT_LOG(xbt_log_priority_critical, "Display of current backtrace disabled by --cfg=exception/cutpath.");
    return;
  }
  std::vector<std::string> res = resolve_backtrace(&bt[0], bt.size());
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
    log_exception(xbt_log_priority_critical, "Uncaught exception", e);
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

void install_exception_handler()
{
  static std::once_flag handler_flag;
  std::call_once(handler_flag, [] {
    previous_terminate_handler = std::set_terminate(handler);
  });
}
// deprecated
void logException(e_xbt_log_priority_t priority, const char* context, std::exception const& exception)
{
  log_exception(priority, context, exception);
}
void installExceptionHandler()
{
  install_exception_handler();
}

} // namespace xbt
} // namespace simgrid

void xbt_throw_impossible(const char* file, int line, const char* func)
{
  std::stringstream ss;
  ss << file << ":" << line << ":" << func << ": The Impossible Did Happen (yet again). Please report this bug.";
  throw std::runtime_error(ss.str());
}
void xbt_throw_unimplemented(const char* file, int line, const char* func)
{
  std::stringstream ss;
  ss << file << ":" << line << ":" << func << ": Feature unimplemented yet. Please report this bug.";
  throw std::runtime_error(ss.str());
}

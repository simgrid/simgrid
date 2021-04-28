/* Copyright (c) 2005-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "src/kernel/context/Context.hpp"
#include <xbt/config.hpp>
#include <xbt/log.hpp>

#include <boost/core/demangle.hpp>
#include <mutex>
#include <sstream>
#include <typeinfo>

XBT_LOG_EXTERNAL_CATEGORY(xbt);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_exception, xbt, "Exceptions");

void _xbt_throw(char* message, const char* file, int line, const char* func)
{
  simgrid::Exception e(
      simgrid::xbt::ThrowPoint(file, line, func, simgrid::xbt::Backtrace(), xbt_procname(), xbt_getpid()),
      message ? message : "");
  xbt_free(message);
  throw e;
}

namespace simgrid {
namespace xbt {

ImpossibleError::~ImpossibleError()         = default;
InitializationError::~InitializationError() = default;
UnimplementedError::~UnimplementedError()   = default;

void log_exception(e_xbt_log_priority_t prio, const char* context, std::exception const& exception)
{
  try {
    std::string name = boost::core::demangle(typeid(exception).name());

    auto* with_context = dynamic_cast<const simgrid::Exception*>(&exception);
    if (with_context != nullptr) {
      XBT_LOG(prio, "%s %s by %s/%d: %s", context, name.c_str(), with_context->throw_point().procname_.c_str(),
              with_context->throw_point().pid_, exception.what());
      // Do we have a backtrace?
      if (not simgrid::config::get_value<bool>("exception/cutpath")) {
        auto backtrace = with_context->resolve_backtrace();
        XBT_LOG(prio, "Backtrace:\n%s", backtrace.c_str());
      }
    } else {
      XBT_LOG(prio, "%s %s: %s", context, name.c_str(), exception.what());
    }
  } catch (...) {
    // Don't log exceptions we got when trying to log exception
    XBT_LOG(prio, "Ignoring exception caught while while trying to log an exception!");
  }

  try {
    // Do we have a nested exception?
    auto* with_nested = dynamic_cast<const std::nested_exception*>(&exception);
    if (with_nested != nullptr && with_nested->nested_ptr() != nullptr)
      with_nested->rethrow_nested();
  } catch (const std::exception& nested_exception) {
    log_exception(prio, "Caused by", nested_exception);
  } catch (...) {
    // We could catch nested_exception or WithContextException but we don't bother:
    XBT_LOG(prio, "Caused by an unknown exception");
  }
}

static void show_backtrace(const simgrid::xbt::Backtrace& bt)
{
  if (simgrid::config::get_value<bool>("exception/cutpath")) {
    XBT_CRITICAL("Display of current backtrace disabled by --cfg=exception/cutpath.");
    return;
  }
  std::string res = bt.resolve();
  XBT_CRITICAL("Current backtrace:\n%s", res.c_str());
}

static std::terminate_handler previous_terminate_handler = nullptr;

XBT_ATTRIB_NORETURN static void handler()
{
  // Avoid doing crazy things if we get an uncaught exception inside an uncaught exception
  static std::atomic_flag lock = ATOMIC_FLAG_INIT;
  if (lock.test_and_set()) {
    XBT_ERROR("Handling an exception raised an exception. Bailing out.");
    std::abort();
  }

  // Get the current backtrace and exception
  simgrid::xbt::Backtrace bt = simgrid::xbt::Backtrace();
  try {
    std::rethrow_exception(std::current_exception());
  }

  // Parse error are handled differently, as the call stack does not matter, only the file location
  catch (const simgrid::ParseError& e) {
    XBT_ERROR("%s", e.what());
    XBT_ERROR("Exiting now.");
    std::abort();
  }

  // We manage C++ exception ourselves
  catch (const std::exception& e) {
    log_exception(xbt_log_priority_critical, "Uncaught exception", e);
  }

  catch (const simgrid::ForcefulKillException&) {
    XBT_ERROR("Received a ForcefulKillException at the top-level exception handler. Maybe a Java->C++ call that is not "
              "protected in a try/catch?");
  }

  // We don't know how to manage other exceptions
  catch (...) {
    // If there was another handler let's delegate to it
    if (previous_terminate_handler) {
      previous_terminate_handler();
      XBT_ERROR("Unexpected return from delegated terminate handler");
    }
    XBT_ERROR("Unknown uncaught exception");
  }
  show_backtrace(bt);
  std::abort();
}

void install_exception_handler()
{
  static std::once_flag handler_flag;
  std::call_once(handler_flag, [] {
    previous_terminate_handler = std::set_terminate(handler);
  });
}

} // namespace xbt
} // namespace simgrid

void xbt_throw_impossible(const char* file, int line, const char* func)
{
  std::stringstream ss;
  ss << file << ":" << line << ":" << func
     << ": The Impossible Did Happen (yet again). Please report this bug (after reading https://xkcd.com/2200 :)";
  throw simgrid::xbt::ImpossibleError(ss.str());
}
void xbt_throw_unimplemented(const char* file, int line, const char* func)
{
  std::stringstream ss;
  ss << file << ":" << line << ":" << func << ": Feature unimplemented yet. Please report this bug.";
  throw simgrid::xbt::UnimplementedError(ss.str());
}

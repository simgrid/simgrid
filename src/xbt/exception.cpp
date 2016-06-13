/* Copyright (c) 2005-2016. The SimGrid Team.
 * All rights reserved. */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <exception>
#include <typeinfo>
#include <memory>

#include <xbt/backtrace.hpp>
#include <xbt/exception.hpp>
#include <xbt/log.h>
#include <xbt/log.hpp>

extern "C" {
XBT_LOG_EXTERNAL_CATEGORY(xbt);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_exception, xbt, "Exceptions");
}

namespace simgrid {
namespace xbt {

WithContextException::~WithContextException() {}

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
    if (with_context != nullptr) {
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

}
}

/* Copyright (c) 2005-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"

#include <xbt/backtrace.hpp>
#include <xbt/string.hpp>
#include <xbt/sysdep.h>
#include <xbt/virtu.h>

#include <cstdio>
#include <cstdlib>
#include <sstream>

// Try to detect and use the C++ itanium ABI for name demangling:
#ifdef __GXX_ABI_VERSION
#include <cxxabi.h>
#endif

#if HAVE_BOOST_STACKTRACE_BACKTRACE
#define BOOST_STACKTRACE_USE_BACKTRACE
#include <boost/stacktrace.hpp>
#elif HAVE_BOOST_STACKTRACE_ADDR2LINE
#define BOOST_STACKTRACE_USE_ADDR2LINE
#include <boost/stacktrace.hpp>
#endif

/** @brief show the backtrace of the current point (lovely while debugging) */
void xbt_backtrace_display_current()
{
  simgrid::xbt::Backtrace().display();
}

namespace simgrid {
namespace xbt {

std::unique_ptr<char, std::function<void(char*)>> demangle(const char* name)
{
#ifdef __GXX_ABI_VERSION
  int status;
  std::unique_ptr<char, std::function<void(char*)>> res(abi::__cxa_demangle(name, nullptr, nullptr, &status),
                                                        &std::free);
  if (res != nullptr)
    return res;
  // We did not manage to resolve this. Probably because this is not a mangled symbol:
#endif
  // Return the symbol:
  return std::unique_ptr<char, std::function<void(char*)>>(xbt_strdup(name), &xbt_free_f);
}

class BacktraceImpl {
#if HAVE_BOOST_STACKTRACE_BACKTRACE || HAVE_BOOST_STACKTRACE_ADDR2LINE
  const boost::stacktrace::stacktrace st = boost::stacktrace::stacktrace();
#else
  const char st[1] = ""; // fallback value
#endif
public:
  std::string resolve() const
  {
    std::stringstream ss;
    ss << st;
    return ss.str();
  }
};

Backtrace::Backtrace() : impl_(std::make_shared<BacktraceImpl>()) {}

std::string Backtrace::resolve() const
{
  return impl_->resolve();
}

void Backtrace::display() const
{
  std::string backtrace = resolve();
  std::fprintf(stderr, "Backtrace (displayed in actor %s):\n%s\n", xbt_procname(),
               backtrace.empty() ? "(backtrace not set -- did you install Boost.Stacktrace?)" : backtrace.c_str());
}

} // namespace xbt
} // namespace simgrid

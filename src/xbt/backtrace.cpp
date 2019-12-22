/* Copyright (c) 2005-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"

#include <xbt/backtrace.hpp>
#include <xbt/string.hpp>
#include <xbt/sysdep.h>
#include <xbt/virtu.h>

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <vector>

#include <boost/algorithm/string.hpp>

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
  short refcount_ = 1;

public:
  void ref() { refcount_++; }
  bool unref()
  {
    refcount_--;
    if (refcount_ == 0) {
      delete this;
      return true;
    } else {
      return false;
    }
  }
#if HAVE_BOOST_STACKTRACE_BACKTRACE || HAVE_BOOST_STACKTRACE_ADDR2LINE
  boost::stacktrace::stacktrace st;
#endif
};

Backtrace::Backtrace()
{
#if HAVE_BOOST_STACKTRACE_BACKTRACE || HAVE_BOOST_STACKTRACE_ADDR2LINE
  impl_     = new BacktraceImpl();
  impl_->st = boost::stacktrace::stacktrace();
#endif
}

Backtrace::Backtrace(const Backtrace& bt) : impl_(bt.impl_)
{
  if (impl_)
    impl_->ref();
}

Backtrace::Backtrace(Backtrace&& bt)
{
  std::swap(impl_, bt.impl_);
}

Backtrace& Backtrace::operator=(const Backtrace& rhs)
{
  if (this != &rhs) {
    if (impl_)
      impl_->unref();
    impl_ = rhs.impl_;
    if (impl_)
      impl_->ref();
  }
  return *this;
}

Backtrace& Backtrace::operator=(Backtrace&& rhs)
{
  if (this != &rhs) {
    if (impl_)
      impl_->unref();
    impl_     = rhs.impl_;
    rhs.impl_ = nullptr;
  }
  return *this;
}

Backtrace::~Backtrace()
{
  if (impl_)
    impl_->unref();
}

std::string const Backtrace::resolve() const
{
  std::string result("");

#if HAVE_BOOST_STACKTRACE_BACKTRACE || HAVE_BOOST_STACKTRACE_ADDR2LINE
  std::stringstream ss;
  ss << impl_->st;
  result.append(ss.str());
#endif
  return result;
}

void Backtrace::display() const
{
  std::string backtrace = resolve();
  if (backtrace.empty()) {
    fprintf(stderr, "(backtrace not set -- did you install Boost.Stacktrace?)\n");
    return;
  }
  fprintf(stderr, "Backtrace (displayed in actor %s):\n", xbt_procname());
  std::fprintf(stderr, "%s\n", backtrace.c_str());
}

} // namespace xbt
} // namespace simgrid

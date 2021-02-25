/* Copyright (c) 2005-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"

#include <xbt/backtrace.hpp>
#include <xbt/string.hpp>
#include <xbt/sysdep.h>
#include <xbt/virtu.h>

#include <boost/algorithm/string/predicate.hpp>
#include <cstdio>
#include <cstdlib>
#include <sstream>

#if HAVE_BOOST_STACKTRACE_BACKTRACE
#define BOOST_STACKTRACE_USE_BACKTRACE
#include <boost/stacktrace.hpp>
#include <boost/stacktrace/detail/frame_decl.hpp>
#elif HAVE_BOOST_STACKTRACE_ADDR2LINE
#define BOOST_STACKTRACE_USE_ADDR2LINE
#include <boost/stacktrace.hpp>
#include <boost/stacktrace/detail/frame_decl.hpp>
#endif

/** @brief show the backtrace of the current point (lovely while debugging) */
void xbt_backtrace_display_current()
{
  simgrid::xbt::Backtrace().display();
}

namespace simgrid {
namespace xbt {

class BacktraceImpl {
#if HAVE_BOOST_STACKTRACE_BACKTRACE || HAVE_BOOST_STACKTRACE_ADDR2LINE
  const boost::stacktrace::stacktrace st;

public:
  std::string resolve() const
  {
    std::stringstream ss;

    int frame_count = 0;
    int state       = 0;

    for (boost::stacktrace::frame frame : st) {
      if (state == 1) {
        if (boost::starts_with(frame.name(), "simgrid::xbt::MainFunction") ||
            boost::starts_with(frame.name(), "simgrid::kernel::context::Context::operator()()"))
          break;
        ss << "  ->  " << frame_count++ << "# " << frame.name() << " at " << frame.source_file() << ":"
           << frame.source_line() << std::endl;
        if (frame.name() == "main")
          break;
      } else {
        if (frame.name() == "simgrid::xbt::Backtrace::Backtrace()")
          state = 1;
      }
    }

    return ss.str();
  }
#else

public:
  std::string resolve() const { return ""; } // fallback value
#endif
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

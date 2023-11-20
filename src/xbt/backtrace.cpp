/* Copyright (c) 2005-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"

#include <simgrid/actor.h>
#include <simgrid/s4u/Actor.hpp>
#include <xbt/backtrace.hpp>
#include <xbt/string.hpp>
#include <xbt/sysdep.h>

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

namespace simgrid::xbt {

class BacktraceImpl {
#if HAVE_BOOST_STACKTRACE_BACKTRACE || HAVE_BOOST_STACKTRACE_ADDR2LINE
  const boost::stacktrace::stacktrace st;

public:
  std::string resolve() const
  {
    std::stringstream ss;

    int frame_count = 0;
    bool print      = false;

    for (boost::stacktrace::frame const& frame : st) {
      const std::string frame_name = frame.name();
      if (print) {
        if (frame_name.rfind("simgrid::xbt::MainFunction", 0) == 0 ||
            frame_name.rfind("simgrid::kernel::context::Context::operator()()", 0) == 0 ||
            frame_name.rfind("auto sthread_create::{lambda") == 0)
          break;
        ss << "  ->  #" << frame_count++ << " ";
        if (xbt_log_no_loc) // Don't display file source and line if so
          ss << (frame_name.empty() ? "(debug info not found and log:no_loc activated)" : frame_name) << "\n";
        else
          ss << frame << "\n";
        // If we are displaying the user side of a simcall, remove the crude details of context switching
        if (frame_name.find("simgrid::kernel::actor::simcall_answered") != std::string::npos ||
            frame_name.find("simgrid::kernel::actor::simcall_blocking") != std::string::npos ||
            frame_name.find("simcall_run_answered") != std::string::npos ||
            frame_name.find("simcall_run_blocking") != std::string::npos) {
          frame_count = 0;
          ss.str(""); // This is how you clear a stringstream in C++. clear() is something else :'(
        }
        if (frame_name == "main")
          break;
      } else {
        if (frame_name == "simgrid::xbt::Backtrace::Backtrace()")
          print = true;
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
  std::fprintf(stderr, "Backtrace (displayed in actor %s%s):\n%s\n",
               simgrid::s4u::Actor::is_maestro() ? "maestro" : sg_actor_self_get_name(),
               (xbt_log_no_loc ? " -- short trace because of --log=no_loc" : ""),
               backtrace.empty() ? "(backtrace not set -- did you install Boost.Stacktrace?)" : backtrace.c_str());
}

} // namespace simgrid::xbt

/* Copyright (c) 2005-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"
#include "xbt/config.hpp"

#include <simgrid/actor.h>
#include <simgrid/s4u/Actor.hpp>
#include <xbt/backtrace.hpp>
#include <xbt/string.hpp>
#include <xbt/sysdep.h>

#include <cstdio>
#include <cstdlib>
#include <sstream>

#if HAVE_STD_STACKTRACE
#include <stacktrace>
#endif

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

simgrid::config::Flag<bool> cfg_fullstdstack{"debug/fullstack",
                                             "Whether the std::stacktrace should be displayed completely. The default "
                                             "value 'no' requests the stack to be trimed for user comfort.",
                                             true};

namespace simgrid::xbt {

class BacktraceImpl {
#if HAVE_STD_STACKTRACE
  std::stacktrace st = std::stacktrace::current();

public:
  std::string resolve() const
  {
    std::stringstream ss;

    int frame_count = 0;
    bool print      = cfg_fullstdstack; // Start printing right away if we are requested not to trim

    for (const auto& frame : st) {
      const std::string frame_name = frame.description();

      if (print) {
        if (not cfg_fullstdstack || // Do not trim the stack end if requested so
            frame_name.rfind("simgrid::xbt::MainFunction", 0) == 0 ||
            frame_name.rfind("simgrid::kernel::context::Context::operator()()", 0) == 0 ||
            frame_name.rfind("auto sthread_create::{lambda") == 0)
          break;
        ss << "  ->  #" << frame_count++ << " ";
        if (xbt_log_no_loc) // Don't display file source and line if so
          ss << (frame_name.empty() ? "(debug info not found and log:no_loc activated)" : frame_name) << "\n";
        else
          ss << frame << "\n";
        if (not cfg_fullstdstack || // Do not trim the stack end if requested so
            frame_name == "main")
          break;
      } else if (frame_name ==
                     "std::shared_ptr<simgrid::xbt::BacktraceImpl> std::make_shared<simgrid::xbt::BacktraceImpl>()" ||
                 frame_name == "xbt_backtrace_display_current") {
        print = true;
      }
    }
    if (ss.str() == "")
      return "The C++23 backtrace returned an empty string. You may want to use --cfg:debug/fullstack:yes to get ride "
             "of the stack trimming logic. If it helps, please report this bug (including a full stack obtained with "
             "the additional config flag).";
    return ss.str();
  }
#elif HAVE_BOOST_STACKTRACE_BACKTRACE || HAVE_BOOST_STACKTRACE_ADDR2LINE
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
               backtrace.empty() ? "(the backtrace is empty -- please use a C++23 compiler or install Boost.Stacktrace)"
                                 : backtrace.c_str());
}

} // namespace simgrid::xbt

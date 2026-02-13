/* Copyright (c) 2005-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"
#include "src/sthread/sthread.h"
#include "xbt/config.hpp"
#include "xbt/log.h"

#include <cstring>
#include <memory>
#include <simgrid/actor.h>
#include <simgrid/s4u/Actor.hpp>
#include <sys/wait.h>
#include <xbt/backtrace.hpp>
#include <xbt/string.hpp>
#include <xbt/sysdep.h>

#include <cstdio>
#include <cstdlib>
#include <sstream>

#if HAVE_STD_STACKTRACE
#include <stacktrace>
#endif

#if HAVE_BOOST_STACKTRACE_ADDR2LINE
#define BOOST_STACKTRACE_USE_ADDR2LINE
#include <boost/stacktrace.hpp>
#include <boost/stacktrace/detail/frame_decl.hpp>
#endif

#if HAVE_EXECINFO_H
#include <execinfo.h>
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

simgrid::config::Flag<std::string> cfg_stacktrace_kind{
    "debug/stacktrace",
    "System to use to generate the stacktraces",
#if HAVE_STD_STACKTRACE
    "c++23",
#elif HAVE_BOOST_STACKTRACE_ADDR2LINE
    "boost",
#else
    "none",
#endif
    {
#if HAVE_STD_STACKTRACE
        {"c++23", "Standard implementation from C++23 (fast but sometimes fails with sthread)."},
#endif
#if HAVE_BOOST_STACKTRACE_ADDR2LINE
        {"boost", "Boost implementation based on addr2line (slow but robust and portable)."},
#endif
#if HAVE_EXECINFO_H
        {"gcc", "Manual implementation using gcc (ultra roots, but unbreakable)."},
        {"addr2line", "Manual implementation using gcc and addr2line (very robust provided that addr2line works)."},
#endif
        {"none", "No backtrace."}}};

namespace simgrid::xbt {

class BacktraceImpl {
#if HAVE_STD_STACKTRACE
  std::stacktrace std_stacktrace;

#endif
#if HAVE_BOOST_STACKTRACE_ADDR2LINE
  std::unique_ptr<boost::stacktrace::stacktrace> stacktrace_boost = nullptr;
#endif
#if HAVE_EXECINFO_H
#define BT_BUF_SIZE 32
  size_t nptrs;
  void* buffer[BT_BUF_SIZE];
#endif

public:
  BacktraceImpl()
  {
#if HAVE_STD_STACKTRACE
    if (cfg_stacktrace_kind == "c++23")
      std_stacktrace = std::stacktrace::current();
#endif
#if HAVE_EXECINFO_H
    if (cfg_stacktrace_kind == "gcc" || cfg_stacktrace_kind == "addr2line")
      nptrs = backtrace(buffer, BT_BUF_SIZE);
#endif
#if HAVE_BOOST_STACKTRACE_ADDR2LINE
    if (cfg_stacktrace_kind == "boost")
      stacktrace_boost = std::make_unique<boost::stacktrace::stacktrace>();
#endif
  }

  // Returns whether we should stop printing this stack
  bool resolve_one(std::stringstream& ss, const std::string& frame_name, const std::string& frame_loc, bool& print,
                   int& frame_count) const
  {
    if (print) {
      if (not cfg_fullstdstack || // Do not trim the stack end if requested so
          frame_name.rfind("simgrid::xbt::MainFunction", 0) == 0 ||
          frame_name.rfind("simgrid::kernel::context::Context::operator()()", 0) == 0 ||
          frame_name.rfind("auto sthread_create::{lambda") == 0)
        return true;
      ss << "  ->  #" << frame_count++ << " ";
      if (xbt_log_no_loc) // Don't display file source and line if so
        ss << (frame_name.empty() ? "(debug info not found and log:no_loc activated)" : frame_name) << "\n";
      else
        ss << frame_loc << "\n";
      if (not cfg_fullstdstack || // Do not trim the stack end if requested so
          frame_name == "main")
        return true;
    } else if (frame_name ==
                   "std::shared_ptr<simgrid::xbt::BacktraceImpl> std::make_shared<simgrid::xbt::BacktraceImpl>()" ||
               frame_name == "xbt_backtrace_display_current") {
      print = true;
    }
    return false;
  }
  std::string resolve() const
  {
    std::stringstream ss;

    int frame_count = 0;
    bool print      = cfg_fullstdstack; // Start printing right away if we are requested not to trim

#if HAVE_STD_STACKTRACE
    if (cfg_stacktrace_kind == "c++23") {
      bool problem = false;
      for (const auto& frame : std_stacktrace) {
        std::stringstream frame_loc;
        frame_loc << frame.source_file() << ":" << frame.source_line();
        const std::string frame_name = frame.description();
        if (frame_name == "")
          problem = true;
        if (resolve_one(ss, frame_name, frame_loc.str(), print, frame_count))
          break;
      }
      if (problem)
        XBT_CERROR(root, "Some stack frames could not be symbolized. You may want to use the slow but robust addr2line "
                         "stacktraces with --cfg=debug/stacktrace:addr2line");
    }
#endif

#if HAVE_BOOST_STACKTRACE_ADDR2LINE
    if (cfg_stacktrace_kind == "boost") {
      bool problem = false;
      sthread_disable();
      for (boost::stacktrace::frame const& frame : *stacktrace_boost) {
        std::stringstream frame_loc;
        frame_loc << frame.source_file() << ":" << frame.source_line();
        std::string frame_name = frame.name();
        std::stringstream fn;
        if (frame_name == "")
          problem = true;
        if (resolve_one(ss, std::to_string((long)frame.address()), frame_loc.str(), print, frame_count))
          break;
      }
      sthread_enable();
      if (problem)
        XBT_CERROR(root, "Some stack frames could not be symbolized. You may want to use the hardcore addr2line "
                         "stacktraces with --cfg=debug/stacktrace:addr2line");
    }
#endif

#if HAVE_EXECINFO_H
    if (cfg_stacktrace_kind == "gcc") {
      char** strings = backtrace_symbols(buffer, nptrs);
      xbt_assert(strings != NULL, "backtrace_symbols failed");

      bool problem = false;
      for (size_t j = 0; j < nptrs; j++) {
        char* pre  = strchr(strings[j], '(');
        char* post = strrchr(strings[j], ')');
        if (pre != nullptr && post != nullptr) { // We can pretty print
          *pre  = ' ';
          *post = '\0';
          ss << "  addr2line -Cfpe " << strings[j] << "\n";
        } else { // The output format changed, do not pretty print
          problem = true;
          ss << "  " << strings[j] << "\n";
        }
      }

      free(strings);
      if (problem)
        ss << "The format of some lines changed. Sorry, you need to figure by yourself how to run addr2line.\n";
      else
        ss << "Call addr2line as advised to display the info in a readable way.\n";
    }

    if (cfg_stacktrace_kind == "addr2line") {
      char** strings = backtrace_symbols(buffer, nptrs);
      xbt_assert(strings != NULL, "backtrace_symbols failed");

#define ERRMSG                                                                                                         \
  "The 'addr2line' stacktrace backed is unusable. Try the raw gcc stacktraces with --cfg=debug/stacktrace:gcc"

      sthread_disable();
      for (size_t j = 0; j < nptrs; j++) {
        char* pre  = strchr(strings[j], '(');
        char* post = strrchr(strings[j], ')');
        xbt_assert(pre != nullptr && post != nullptr, "The output format changed. " ERRMSG);
        *pre  = '\0';
        *post = '\0';

        int pipes[2];
        int res = pipe(pipes);
        xbt_assert(res == 0, "pipe() failed (error: %s). " ERRMSG, strerror(errno));
        int pid = fork();
        xbt_assert(pid >= 0, "Fork failed (error: %s). " ERRMSG, strerror(errno));
        if (pid == 0) { // child
          const char* argv[5] = {"addr2line", "-Cfpe", strings[j], pre + 1, (char*)NULL};

          close(pipes[0]);
          close(1);
          res = dup2(pipes[1], 1);
          xbt_assert(res >= 0, "dup2() failed (error: %s). " ERRMSG, strerror(errno));
          close(pipes[1]);
          execvp("addr2line", (char* const*)argv);
          xbt_die("Failed to exec addr2line (error: %s). " ERRMSG, strerror(errno));
        } else { // parent
          close(pipes[1]);
          char buffer[1024];
          int res;
          ss << "  ";
          while ((res = read(pipes[0], &buffer, 1024)) > 0) {
            buffer[res] = '\0';
            ss << buffer;
          }
          close(pipes[0]);
          waitpid(pid, nullptr, 0);
        }
      }
      sthread_enable();
      ss << "If some frame symbols are not shown in this trace, compile with '-fno-omit-frame-pointer -rdynamic' "
            "and/or use --cfg=debug/stacktrace:gcc and figure the missing elements manually.";

#undef ERRMSG
      free(strings);
    }
#endif

    if (ss.str() == "" && cfg_stacktrace_kind != "none")
      return "The backtrace returned an empty string. You may want to use --cfg:debug/fullstack:yes to get ride "
             "of the stack trimming logic. If it helps, please report this bug (including a full stack obtained with "
             "the additional config flag).";

    return ss.str();
  }
};

Backtrace::Backtrace(bool capture) : impl_(capture ? std::make_shared<BacktraceImpl>() : nullptr) {}

void Backtrace::reset()
{
  impl_ = std::make_shared<BacktraceImpl>();
}

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

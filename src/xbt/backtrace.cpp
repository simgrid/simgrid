/* Copyright (c) 2005-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"
#include "src/sthread/sthread.h"
#include "xbt/config.hpp"
#include "xbt/log.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <cstring>
#include <memory>
#include <simgrid/actor.h>
#include <simgrid/s4u/Actor.hpp>
#include <stdexcept>
#include <string.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
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

#if HAVE_DWELF_STACKTRACE
#include <elfutils/libdw.h>
#include <elfutils/libdwfl.h>

struct DwelfResolved {
  std::string module;
  std::string module_short;
  std::string func;
  std::string location;
};

static Dwfl* dwelf_handle = nullptr;
static std::unordered_map<Dwarf_Addr, DwelfResolved> dwelf_cache;

static const Dwfl_Callbacks callbacks = {.find_elf        = dwfl_linux_proc_find_elf,
                                         .find_debuginfo  = dwfl_standard_find_debuginfo,
                                         .section_address = dwfl_offline_section_address,
                                         .debuginfo_path  = nullptr};
static void dwelf_init()
{
  if (dwelf_handle)
    return;

  dwelf_handle = dwfl_begin(&callbacks);
  xbt_assert(dwelf_handle != nullptr, "Failed to init dwfl: %s", dwfl_errmsg(dwfl_errno()));

  int ret = dwfl_linux_proc_report(dwelf_handle, getpid());
  if (ret != 0)
    xbt_die("Failed to feed process pid:%d into dwfl. ret: %d, Error: %s", getpid(), ret, dwfl_errmsg(dwfl_errno()));

  dwfl_report_end(dwelf_handle, nullptr, nullptr);
}

static const DwelfResolved& dwelf_resolve_addr(Dwarf_Addr addr)
{
  auto it = dwelf_cache.find(addr);
  if (it != dwelf_cache.end())
    return it->second;

  DwelfResolved r;
  r.func     = "??";
  r.location = "??:0";

  Dwfl_Module* mod = dwfl_addrmodule(dwelf_handle, addr);
  if (mod == nullptr) {
    // Try to reload the process, just in case there was a recent dlopen
    int ret = dwfl_linux_proc_report(dwelf_handle, getpid());
    if (ret != 0)
      xbt_die("Failed to feed process pid:%d into dwfl. ret: %d, Error: %s", getpid(), ret, dwfl_errmsg(dwfl_errno()));
    dwfl_report_end(dwelf_handle, nullptr, nullptr);

    if (mod) // Found the module and reloaded it -- we need to invalidate all addresses
      dwelf_cache.clear();
  }
  if (mod) {
    const char* path = dwfl_module_info(mod, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    if (path) {
      r.module        = path;
      const char* sep = strrchr(path, '/');
      r.module_short  = sep == nullptr ? path : sep + 1;
      if (strncmp(r.module_short.c_str(), "libsthread.so", strlen("libsthread.so")) == 0)
        r.module_short = "libsthread.so";
      if (strncmp(r.module_short.c_str(), "libsimgrid.so", strlen("libsimgrid.so")) == 0)
        r.module_short = "libsimgrid.so";
    }
    const char* func = dwfl_module_addrname(mod, addr);
    if (func) {
      r.func = boost::core::demangle(func);
      if (r.func.find('(') == r.func.npos)
        r.func += "()";
    }

    int column           = 0;
    Dwfl_Line* dwfl_line = dwfl_getsrc(dwelf_handle, addr);
    if (dwfl_line) {
      int line;
      const char* file = dwfl_lineinfo(dwfl_line, nullptr, &line, &column, nullptr, nullptr);
      if (file) {
        const char* sep = strrchr(file, '/');
        std::stringstream frame_loc;
        frame_loc << (sep ? sep + 1 : file) << ":" << line;
        r.location = frame_loc.str();
      }
    }
  }

  auto res = dwelf_cache.emplace(addr, std::move(r));
  return res.first->second;
}
#endif

/** @brief show the backtrace of the current point (lovely while debugging) */
void xbt_backtrace_display_current()
{
  simgrid::xbt::Backtrace().display();
}

simgrid::config::Flag<bool> cfg_fullstdstack{"debug/fullstack",
                                             "Whether the std::stacktrace should be displayed completely. The "
                                             "value 'no' requests the stack to be trimed for user comfort.",
                                             true};

simgrid::config::Flag<std::string> cfg_stacktrace_kind{
    "debug/stacktrace",
    "System to use to generate the stacktraces",
#if HAVE_DWELF_STACKTRACE
    "dwelf",
#elif HAVE_STD_STACKTRACE
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
#if HAVE_DWELF_STACKTRACE
        {"dwelf", "Ad-hoc implementation with libdw and libelf (recommended when using sthread)."},
#endif
#if HAVE_BOOST_STACKTRACE_ADDR2LINE
        {"boost", "Boost implementation based on addr2line (slow but robust and portable)."},
#endif
#if HAVE_EXECINFO_H
        {"gcc", "Manual implementation using gcc (ultra roots, but unbreakable)."},
        {"addr2line", "Manual implementation using gcc and addr2line (very robust provided that addr2line works)."},
#endif
        {"none", "No backtrace."}}};

static std::vector<std::string> cfg_stacktrace_ignored_elements;
static simgrid::config::Flag<std::string> cfg_stacktrace_ignore{
    "debug/stacktrace/ignore",
    "Comma-separated list of object files to ignore while displaying the stacktraces (addr2line only)", "",
    [](std::string_view value) {
      std::vector<std::string> result;
      if (value.empty())
        return;
      try {
        boost::algorithm::split(result, value, boost::is_any_of(","));
        for (const auto& s : result) {
          cfg_stacktrace_ignored_elements.push_back(s);
        }
      } catch (const std::exception& e) {
        throw std::string("Invalid value for debug/stacktrace/ignore: ") + e.what();
      }
    }};

namespace simgrid::xbt {

class BacktraceImpl {
#if HAVE_STD_STACKTRACE
  std::stacktrace std_stacktrace;

#endif
#if HAVE_BOOST_STACKTRACE_ADDR2LINE
  std::unique_ptr<boost::stacktrace::stacktrace> stacktrace_boost = nullptr;
#endif
#if HAVE_EXECINFO_H || HAVE_DWELF_STACKTRACE
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
#if HAVE_DWELF_STACKTRACE
    if (cfg_stacktrace_kind == "dwelf")
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
        XBT_CERROR(root, "Some stack frames could not be symbolized. You may want to use --cfg=debug/stacktrace:dwelf "
                         "(install libdw to get it) or --cfg=debug/stacktrace:addr2line (robust but ultra slow)");
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

#if HAVE_DWELF_STACKTRACE
    if (cfg_stacktrace_kind == "dwelf") {
      sthread_disable();
      dwelf_init();

      unsigned begin = 0;
      unsigned end   = nptrs;
      for (unsigned i = 0; i < nptrs; i++) {
        // The address points to the instruction after the current one, so use addr -1
        // https://stackoverflow.com/questions/11579509/wrong-line-numbers-from-addr2line

        Dwarf_Addr addr        = (Dwarf_Addr)((long)buffer[i] - 1);
        const DwelfResolved& r = dwelf_resolve_addr(addr);
        if (r.func == "simgrid::xbt::Backtrace::Backtrace(bool)" || r.func == "xbt_backtrace_display_current()" ||
            r.func == "__mcsimgrid_write()" || r.func == "__mcsimgrid_read()" ||
            // The lambda of sthread_create have complex names, that depend on the compiler
            // r.func.find("std::_Bind<sthread_create::{lambda(auto") ||
            // TODO: starts_with in C++20 instead of rfind
            r.func.rfind("sthread_", 0) != std::string::npos)
          begin = i + 1;
      }
      for (unsigned i = nptrs - 1; i > 0; i--) {
        Dwarf_Addr addr        = (Dwarf_Addr)buffer[i];
        const DwelfResolved& r = dwelf_resolve_addr(addr);
        if (r.func == "main()")
          end = i + 1;

        if (r.func == "smx_ctx_wrapper()" || r.func == "std::function<void ()>::operator()() const" ||
            r.func == "std::_Function_handler<void (), std::_Bind<sthread_create::$_0 (void* (*)(void*), void*)> "
                      ">::_M_invoke(std::_Any_data const&)" ||
            r.func == "auto sthread_create::{lambda(auto:1*, auto:2*)#1}::operator()<void* (void*), void>(void* "
                      "(*)(void*), void*) const")
          end = i;
      }
      int frame_count = 0;
      for (unsigned i = begin; i < end; ++i) {
        Dwarf_Addr addr        = (Dwarf_Addr)buffer[i];
        const DwelfResolved& r = dwelf_resolve_addr(addr);
        bool ignored_line      = false;
        for (auto const& ignored : cfg_stacktrace_ignored_elements) {
          if (strstr(r.module.c_str(), ignored.c_str()) != nullptr) {
            ignored_line = true;
            break;
          }
        }
        if (ignored_line)
          continue;

        ss << "  #" << frame_count++ << " " << r.func;
        if (not xbt_log_no_loc)
          ss << " at " << r.location;
        ss << " in " << r.module_short << "\n";
      }
    }
    sthread_enable();
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
          ss << "  addr2line --basenames -Cfpe " << strings[j] << "\n";
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
      bool problem = false;
      for (size_t j = 0; j < nptrs; j++) {
        bool ignored_line = false;
        for (auto const& ignored : cfg_stacktrace_ignored_elements) {
          if (strstr(strings[j], ignored.c_str()) != nullptr) {
            ignored_line = true;
            break;
          }
        }
        if (ignored_line)
          continue;

        char* pre  = strchr(strings[j], '(');
        char* post = strrchr(strings[j], ')');
        xbt_assert(pre != nullptr && post != nullptr, "The output format changed. " ERRMSG);
        *pre  = '\0';
        *post = '\0';
        // The address points to the instruction after the current one
        // https://stackoverflow.com/questions/11579509/wrong-line-numbers-from-addr2line
        // Try to modify it if it's a pure address. TODO: also handle the cases where pre+1 contains something like
        // "pthread_mutex_lock+0x24"
        char buffer[256] = {0};
        try {
          // fprintf(stderr, "%s | %s\n", strings[j], pre+1);
          long addr = std::stol(pre + 1, 0, 16);
          if (addr > 0)
            addr--;
          sprintf(buffer, "%p", (void*)addr);
        } catch (const std::invalid_argument&) {
          strcpy(buffer, pre + 1);
        }

        int pipes[2];
        int res = pipe(pipes);
        xbt_assert(res == 0, "pipe() failed (error: %s). " ERRMSG, strerror(errno));
        int pid = fork();
        xbt_assert(pid >= 0, "Fork failed (error: %s). " ERRMSG, strerror(errno));
        if (pid == 0) { // child
          setenv("LC_ALL", "C", 1);
          const char* argv[] = {"addr2line", "--basenames", "-Cfpe", strings[j], buffer, (char*)NULL};

          close(pipes[0]);
          close(1);
          res = dup2(pipes[1], 1);
          xbt_assert(res >= 0, "dup2() failed (error: %s). " ERRMSG, strerror(errno));
          close(pipes[1]);
          execvp("addr2line", (char* const*)argv);
          xbt_die("Failed to exec addr2line (error: %s). " ERRMSG, strerror(errno));
        } else { // parent
          close(pipes[1]);
          char buffer[2048];
          int res;
          bool first = true;
          while ((res = read(pipes[0], &buffer, 2047)) > 0) {
            buffer[res] = '\0';
            if (strcmp(buffer, "?? ??:0\n") == 0) {
              problem = true;
              *pre    = ' ';
              *post   = '\0';
              sprintf(buffer, "addr2line --basenames -Cfpe %s\n", strings[j]);
            }
            if (strstr(buffer, " sthread_impl.cpp:") == nullptr) {
              if (first)
                ss << "  ";
              first = false;
              ss << buffer;
            }
          }
          close(pipes[0]);
          waitpid(pid, nullptr, 0);
        }
      }
      sthread_enable();
      if (problem)
        ss << "Some frame symbols could not be converted. Compile with '-fno-omit-frame-pointer -rdynamic' "
              "and/or use --cfg=debug/stacktrace:gcc instead.\n";

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

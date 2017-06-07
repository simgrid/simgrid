/* Copyright (c) 2005-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <vector>

// Try to detect and use the C++ intanium ABI for name demangling:
#ifdef __GXX_ABI_VERSION
#include <cxxabi.h>
#endif

#include "simgrid/simix.h" /* SIMIX_process_self_get_name() */
#include <xbt/backtrace.h>
#include <xbt/backtrace.hpp>
#include <xbt/log.h>
#include <xbt/sysdep.h>

#include "src/internal_config.h"

extern "C" {

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_backtrace, xbt, "Backtrace");

}

static bool startWith(std::string str, const char* prefix)
{
  return strncmp(str.c_str(), prefix, strlen(prefix)) == 0;
}

void xbt_backtrace_display(xbt_backtrace_location_t* loc, std::size_t count)
{
#ifdef HAVE_BACKTRACE
  std::vector<std::string> backtrace = simgrid::xbt::resolveBacktrace(loc, count);
  if (backtrace.empty()) {
    fprintf(stderr, "(backtrace not set)\n");
    return;
  }
  fprintf(stderr, "Backtrace (displayed in process %s):\n", SIMIX_process_self_get_name());
  for (std::string const& s : backtrace) {
    if (startWith(s, "xbt_backtrace_display_current"))
      continue;

    std::fprintf(stderr, "---> '%s'\n", s.c_str());
    if (startWith(s, "SIMIX_simcall_handle") ||
        startWith(s, "simgrid::xbt::MainFunction") /* main used with thread factory */)
      break;
  }
#else
  XBT_ERROR("Cannot display backtrace when compiled without libunwind.");
#endif
}

/** @brief show the backtrace of the current point (lovely while debugging) */
void xbt_backtrace_display_current()
{
  const std::size_t size = 10;
  xbt_backtrace_location_t bt[size];
  size_t used = xbt_backtrace_current(bt, size);
  xbt_backtrace_display(bt, used);
}

namespace simgrid {
namespace xbt {

std::unique_ptr<char, void(*)(void*)> demangle(const char* name)
{
#ifdef __GXX_ABI_VERSION
  int status;
  auto res = std::unique_ptr<char, void(*)(void*)>(
    abi::__cxa_demangle(name, nullptr, nullptr, &status),
    std::free
  );
  if (res != nullptr)
    return res;
  // We did not manage to resolve this. Probably because this is not a mangled symbol:
#endif
  // Return the symbol:
  return std::unique_ptr<char, void(*)(void*)>(xbt_strdup(name), std::free);
}

std::vector<xbt_backtrace_location_t> backtrace()
{
  const std::size_t size = 10;
  xbt_backtrace_location_t loc[size];
  size_t used = xbt_backtrace_current(loc, size);
  return std::vector<xbt_backtrace_location_t>(loc, loc + used);
}

}
}

#if HAVE_BACKTRACE && HAVE_EXECINFO_H && HAVE_POPEN && defined(ADDR2LINE)
# include "src/xbt/backtrace_linux.cpp"
#else
# include "src/xbt/backtrace_dummy.cpp"
#endif

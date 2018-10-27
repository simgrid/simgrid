/* Copyright (c) 2005-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"

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
#if HAVE_EXECINFO_H
#include <execinfo.h>
#endif

#include "simgrid/simix.h" /* SIMIX_process_self_get_name() */
#include <xbt/backtrace.hpp>
#include <xbt/log.h>
#include <xbt/string.hpp>
#include <xbt/sysdep.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_backtrace, xbt, "Backtrace");

static bool startWith(std::string str, const char* prefix)
{
  return strncmp(str.c_str(), prefix, strlen(prefix)) == 0;
}

void xbt_backtrace_display(const simgrid::xbt::Backtrace& bt)
{
  std::vector<std::string> backtrace = simgrid::xbt::resolve_backtrace(bt);
  if (backtrace.empty()) {
    fprintf(stderr, "(backtrace not set -- maybe unavailable on this architecture?)\n");
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
}

/** @brief show the backtrace of the current point (lovely while debugging) */
void xbt_backtrace_display_current()
{
  simgrid::xbt::Backtrace bt = simgrid::xbt::backtrace();
  xbt_backtrace_display(bt);
}

#if HAVE_BACKTRACE
// For some reason, if I try to use it directly, GCC thinks I try to use xbt::backtrace.
// I suspect that this symbol is not presented as a regular function in execinfo.h
static int gnu_backtrace(simgrid::xbt::Backtrace& bt)
{
  return backtrace(bt.data(), bt.size());
}
#endif

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

Backtrace backtrace()
{
  simgrid::xbt::Backtrace res;
#if HAVE_BACKTRACE
  res.resize(15);
  int used = gnu_backtrace(res);
  if (used == 0) {
    std::fprintf(stderr, "The backtrace() function failed, which probably means that the memory is exhausted\n.");
    std::fprintf(stderr, "Bailing out now since there is nothing I can do without a decent amount of memory\n.");
    std::fprintf(stderr, "Please go fix the memleaks\n");
    std::exit(1);
  }
  res.shrink_to_fit();
#endif
  return res;
}

} // namespace xbt
} // namespace simgrid

namespace simgrid {
namespace xbt {

/** Find the path of the binary file from the name found in argv */
static std::string get_binary_path()
{
  struct stat stat_buf;

  if (xbt_binary_name == nullptr)
    return "";

  // We found it, we are happy:
  if (stat(xbt_binary_name, &stat_buf) == 0)
    return xbt_binary_name;

  // Not found, look in the PATH:
  char* path = getenv("PATH");
  if (path == nullptr)
    return "";

  XBT_DEBUG("Looking in the PATH: %s\n", path);
  std::vector<std::string> path_list;
  boost::split(path_list, path, boost::is_any_of(":;"));

  for (std::string const& path_item : path_list) {
    std::string binary_name = simgrid::xbt::string_printf("%s/%s", path_item.c_str(), xbt_binary_name);
    bool found              = (stat(binary_name.c_str(), &stat_buf) == 0);
    XBT_DEBUG("Looked in the PATH for the binary. %s %s", found ? "Found" : "Not found", binary_name.c_str());
    if (found)
      return binary_name;
  }

  // Not found at all:
  return "";
}

std::vector<std::string> resolve_backtrace(const Backtrace& bt)
{
  std::vector<std::string> result;

#if HAVE_BACKTRACE && HAVE_EXECINFO_H && HAVE_POPEN && defined(ADDR2LINE)
  // FIXME: This code could be greatly improved/simplified with
  //   http://cairo.sourcearchive.com/documentation/1.9.4/backtrace-symbols_8c-source.html
  if (bt.size() == 0)
    return result;

  if (xbt_binary_name == nullptr)
    XBT_WARN("XBT not initialized, the backtrace will not be resolved.");

  char** backtrace_syms   = backtrace_symbols(bt.data(), bt.size());
  std::string binary_name = get_binary_path();

  if (binary_name.empty()) {
    for (std::size_t i = 1; i < bt.size(); i++) // the first one is not interesting
      result.push_back(simgrid::xbt::string_printf("%p", bt[i]));
    return result;
  }

  // Create the system command for add2line:
  std::ostringstream stream;
  stream << ADDR2LINE << " -f -e " << binary_name << ' ';
  std::vector<std::string> addrs(bt.size());
  for (std::size_t i = 1; i < bt.size(); i++) { // the first one is not interesting
    /* retrieve this address */
    XBT_DEBUG("Retrieving address number %zu from '%s'", i, backtrace_syms[i]);
    char buff[256];
    snprintf(buff, 256, "%s", strchr(backtrace_syms[i], '[') + 1);
    char* p = strchr(buff, ']');
    *p      = '\0';
    if (strcmp(buff, "(nil)"))
      addrs[i] = buff;
    else
      addrs[i] = "0x0";
    XBT_DEBUG("Set up a new address: %zu, '%s'", i, addrs[i].c_str());
    /* Add it to the command line args */
    stream << addrs[i] << ' ';
  }
  std::string cmd = stream.str();

  /* size (in char) of pointers on this arch */
  int addr_len = addrs[0].size();

  XBT_VERB("Fire a first command: '%s'", cmd.c_str());
  FILE* pipe = popen(cmd.c_str(), "r");
  xbt_assert(pipe, "Cannot fork addr2line to display the backtrace");

  /* To read the output of addr2line */
  char line_func[1024];
  char line_pos[1024];
  for (std::size_t i = 1; i < bt.size(); i++) { // The first one is not interesting
    XBT_DEBUG("Looking for symbol %zu, addr = '%s'", i, addrs[i].c_str());
    if (fgets(line_func, 1024, pipe)) {
      line_func[strlen(line_func) - 1] = '\0';
    } else {
      XBT_VERB("Cannot run fgets to look for symbol %zu, addr %s", i, addrs[i].c_str());
      strncpy(line_func, "???", 4);
    }
    if (fgets(line_pos, 1024, pipe)) {
      line_pos[strlen(line_pos) - 1] = '\0';
    } else {
      XBT_VERB("Cannot run fgets to look for symbol %zu, addr %s", i, addrs[i].c_str());
      strncpy(line_pos, backtrace_syms[i], 1024);
    }

    if (strcmp("??", line_func) != 0) {
      auto name = simgrid::xbt::demangle(line_func);
      XBT_DEBUG("Found static symbol %s at %s", name.get(), line_pos);
      result.push_back(simgrid::xbt::string_printf("%s at %s, %p", name.get(), line_pos, bt[i]));
    } else {
      /* Damn. The symbol is in a dynamic library. Let's get wild */

      unsigned long int offset = 0;
      int found                = 0;

      /* let's look for the offset of this library in our addressing space */
      std::string maps_name = std::string("/proc/") + std::to_string(getpid()) + "/maps";
      std::ifstream maps(maps_name);
      if (not maps) {
        XBT_CRITICAL("open(\"%s\") failed: %s", maps_name.c_str(), strerror(errno));
        continue;
      }
      size_t pos;
      unsigned long int addr = std::stoul(addrs[i], &pos, 16);
      if (pos != addrs[i].length()) {
        XBT_CRITICAL("Cannot parse backtrace address '%s' (addr=%#lx)", addrs[i].c_str(), addr);
      }
      XBT_DEBUG("addr=%s (as string) =%#lx (as number)", addrs[i].c_str(), addr);

      while (not found) {
        unsigned long int first;
        unsigned long int last;

        std::string maps_buff;
        if (not std::getline(maps, maps_buff))
          break;
        if (i == 0) {
          XBT_DEBUG("map line: %s", maps_buff.c_str());
        }
        first = std::stoul(maps_buff, &pos, 16);
        maps_buff.erase(0, pos + 1);
        last = std::stoul(maps_buff, nullptr, 16);
        if (first < addr && addr < last) {
          offset = first;
          found  = 1;
        }
        if (found) {
          XBT_DEBUG("%#lx in [%#lx-%#lx]", addr, first, last);
          XBT_DEBUG("Symbol found, map lines not further displayed (even if looking for next ones)");
        }
      }
      maps.close();
      addrs[i].clear();

      if (not found) {
        XBT_VERB("Problem while reading the maps file. Following backtrace will be mangled.");
        XBT_DEBUG("No dynamic. Static symbol: %s", backtrace_syms[i]);
        result.push_back(simgrid::xbt::string_printf("?? (%s)", backtrace_syms[i]));
        continue;
      }

      /* Ok, Found the offset of the maps line containing the searched symbol.
         We now need to substract this from the address we got from backtrace.
       */

      addrs[i] = simgrid::xbt::string_printf("0x%0*lx", addr_len - 2, addr - offset);
      XBT_DEBUG("offset=%#lx new addr=%s", offset, addrs[i].c_str());

      /* Got it. We have our new address. Let's get the library path and we are set */
      std::string p(backtrace_syms[i]);
      if (p[0] == '[') {
        /* library path not displayed in the map file either... */
        snprintf(line_func, 3, "??");
      } else {
        size_t p2 = p.find_first_of("( ");
        if (p2 != std::string::npos)
          p.erase(p2);

        /* Here we go, fire an addr2line up */
        std::string subcmd = std::string(ADDR2LINE) + " -f -e " + p + " " + addrs[i];
        XBT_VERB("Fire another command: '%s'", subcmd.c_str());
        FILE* subpipe = popen(subcmd.c_str(), "r");
        if (not subpipe) {
          xbt_die("Cannot fork addr2line to display the backtrace");
        }
        if (fgets(line_func, 1024, subpipe)) {
          line_func[strlen(line_func) - 1] = '\0';
        } else {
          XBT_VERB("Cannot read result of subcommand %s", subcmd.c_str());
          strncpy(line_func, "???", 4);
        }
        if (fgets(line_pos, 1024, subpipe)) {
          line_pos[strlen(line_pos) - 1] = '\0';
        } else {
          XBT_VERB("Cannot read result of subcommand %s", subcmd.c_str());
          strncpy(line_pos, backtrace_syms[i], 1024);
        }
        pclose(subpipe);
      }

      /* check whether the trick worked */
      if (strcmp("??", line_func)) {
        auto name = simgrid::xbt::demangle(line_func);
        XBT_DEBUG("Found dynamic symbol %s at %s", name.get(), line_pos);
        result.push_back(simgrid::xbt::string_printf("%s at %s, %p", name.get(), line_pos, bt[i]));
      } else {
        /* damn, nothing to do here. Let's print the raw address */
        XBT_DEBUG("Dynamic symbol not found. Raw address = %s", backtrace_syms[i]);
        result.push_back(simgrid::xbt::string_printf("?? at %s", backtrace_syms[i]));
      }
    }
    addrs[i].clear();

    /* Mask the bottom of the stack */
    const char* const breakers[] = {
        "main",
        "_ZN7simgrid6kernel7context13ThreadContext7wrapperE", // simgrid::kernel::context::ThreadContext::wrapper
        "_ZN7simgrid6kernel7context8UContext7wrapperE"        // simgrid::kernel::context::UContext::wrapper
    };
    bool do_break = false;
    for (const char* b : breakers) {
      if (strncmp(b, line_func, strlen(b)) == 0) {
        do_break = true;
        break;
      }
    }
    if (do_break)
      break;
  }
  pclose(pipe);
  xbt_free(backtrace_syms);
#endif /* ADDR2LINE usable to resolve the backtrace */
  return result;
}

} // namespace xbt
} // namespace simgrid

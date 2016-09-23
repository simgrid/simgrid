/* Copyright (c) 2005-2019. The SimGrid Team.
 * All rights reserved. */

#ifndef SIMGRIX_XBT_BACKTRACE_HPP
#define SIMGRIX_XBT_BACKTRACE_HPP

#include <cstddef>

#include <string>
#include <memory>
#include <vector>

#include <xbt/base.h>
#include <xbt/backtrace.h>

namespace simgrid {
namespace xbt {

/** Try to demangle a C++ name
 *
 *  Return the origin string if this fails.
 */
XBT_PUBLIC() std::unique_ptr<char, void(*)(void*)> demangle(const char* name);

/** Get the current backtrace */
XBT_PUBLIC(std::vector<xbt_backtrace_location_t>) backtrace();

/* Translate the backtrace in a human friendly form
 *
 *  Try ro resolve symbols and source code location.
 */
XBT_PUBLIC(std::vector<std::string>) resolveBacktrace(
  xbt_backtrace_location_t const* loc, std::size_t count);

}
}

#endif

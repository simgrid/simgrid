/* Copyright (c) 2005-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRIX_XBT_BACKTRACE_HPP
#define SIMGRIX_XBT_BACKTRACE_HPP

#include <cstddef>

#include <string>
#include <memory>
#include <vector>

#include <xbt/backtrace.h>

namespace simgrid {
namespace xbt {

/** A backtrace
 *
 *  This is used (among other things) in exceptions to store the associated
 *  backtrace.
 *
 *  @ingroup XBT_ex
 */
typedef std::vector<xbt_backtrace_location_t> Backtrace;

/** Try to demangle a C++ name
 *
 *  Return the origin string if this fails.
 */
XBT_PUBLIC std::unique_ptr<char, void (*)(void*)> demangle(const char* name);

/** Get the current backtrace */
XBT_PUBLIC std::vector<xbt_backtrace_location_t> backtrace();

/* Translate the backtrace in a human friendly form
 *
 *  Try resolve symbols and source code location.
 */
XBT_PUBLIC std::vector<std::string> resolve_backtrace(xbt_backtrace_location_t const* loc, std::size_t count);
XBT_ATTRIB_DEPRECATED_v323("Please use xbt::resolve_backtrace()") XBT_PUBLIC std::vector<std::string> resolveBacktrace(xbt_backtrace_location_t const* loc, std::size_t count);
}
}

#endif

/* Copyright (c) 2005-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRIX_XBT_BACKTRACE_HPP
#define SIMGRIX_XBT_BACKTRACE_HPP

#include <xbt/base.h>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

SG_BEGIN_DECL()
/** @brief Shows a backtrace of the current location */
XBT_PUBLIC void xbt_backtrace_display_current();
SG_END_DECL()

typedef void* xbt_backtrace_location_t;

/** @brief reimplementation of glibc backtrace based directly on gcc library, without implicit malloc  */
XBT_PUBLIC int xbt_backtrace_no_malloc(void** bt, int size);

/** @brief Captures a backtrace for further use */
XBT_PUBLIC size_t xbt_backtrace_current(xbt_backtrace_location_t* loc, size_t count);

/** @brief Display a previously captured backtrace */
XBT_PUBLIC void xbt_backtrace_display(xbt_backtrace_location_t* loc, size_t count);

/** @brief Get current backtrace with libunwind */
XBT_PUBLIC int xbt_libunwind_backtrace(void** bt, int size);

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
XBT_PUBLIC Backtrace backtrace();

/* Translate the backtrace in a human friendly form
 *
 *  Try resolve symbols and source code location.
 */
XBT_PUBLIC std::vector<std::string> resolve_backtrace(xbt_backtrace_location_t const* loc, std::size_t count);
}
}

#endif

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

namespace simgrid {
namespace xbt {

/** A backtrace
 *
 *  This is used (among other things) in exceptions to store the associated
 *  backtrace.
 *
 *  @ingroup XBT_ex
 */
typedef std::vector<void*> Backtrace;

/** Try to demangle a C++ name
 *
 *  Return the origin string if this fails.
 */
XBT_PUBLIC std::unique_ptr<char, void (*)(void*)> demangle(const char* name);

/** @brief Captures a backtrace for further use  */
XBT_PUBLIC Backtrace backtrace();

/* Translate the backtrace in a human friendly form
 *
 *  Try resolve symbols and source code location.
 */
XBT_PUBLIC std::vector<std::string> resolve_backtrace(const Backtrace& bt);
}
}

/** @brief Display a previously captured backtrace */
XBT_PUBLIC void xbt_backtrace_display(const simgrid::xbt::Backtrace& bt);

#endif

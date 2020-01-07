/* Copyright (c) 2005-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_XBT_BACKTRACE_HPP
#define SIMGRID_XBT_BACKTRACE_HPP

#include <xbt/base.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

SG_BEGIN_DECL
/** @brief Shows a backtrace of the current location */
XBT_PUBLIC void xbt_backtrace_display_current();
SG_END_DECL

namespace simgrid {
namespace xbt {

/** Try to demangle a C++ name
 *
 *  Return the origin string if this fails.
 */
XBT_PUBLIC std::unique_ptr<char, std::function<void(char*)>> demangle(const char* name);

class BacktraceImpl;
/** A backtrace
 *
 *  This is used (among other things) in exceptions to store the associated
 *  backtrace.
 *
 *  @ingroup XBT_ex
 */
class Backtrace {
public:
  std::shared_ptr<BacktraceImpl> impl_;
  Backtrace();
  /** @brief Translate the backtrace in a human friendly form, unmangled with source code locations. */
  std::string resolve() const;
  /** @brief Display the resolved backtrace on stderr */
  void display() const;
};

}
}
#endif

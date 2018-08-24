/* Copyright (c) 2005-2018. The SimGrid Team.All rights reserved. */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_XBT_EXCEPTION_HPP
#define SIMGRID_XBT_EXCEPTION_HPP

#include <string>
#include <type_traits>
#include <vector>

#include <xbt/base.h>
#include <xbt/backtrace.h>
#include <xbt/backtrace.hpp>
#include <xbt/log.h>
#include <xbt/misc.h>  // xbt_procname
#include <xbt/virtu.h> // xbt_getpid

/** @addtogroup XBT_ex
 *  @brief Exceptions support
 */

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

/** The location of where an exception has been thrown
 *
 *  This is a tuple (__FILE__, __LINE__, __func__) and can be created with
 *  @ref XBT_THROW_POINT.
 *
 *  @ingroup XBT_ex
 */
class ThrowPoint {
 public:
  ThrowPoint() = default;
  explicit ThrowPoint(const char* file, int line, const char* function) : file_(file), line_(line), function_(function)
  {
  }
  const char* file_     = nullptr;
  int line_             = 0;
  const char* function_ = nullptr;
};

/** Create a ThrowPoint with (__FILE__, __LINE__, __func__) */
#define XBT_THROW_POINT ::simgrid::xbt::ThrowPoint(__FILE__, __LINE__, __func__)

/** A base class for exceptions with context
 *
 *  This is a base class for exceptions which store additional contextual
 *  information: backtrace, throw point, simulated process name, PID, etc.
 */
class XBT_PUBLIC WithContextException {
public:
  WithContextException() :
    backtrace_(simgrid::xbt::backtrace()),
    procname_(xbt_procname()),
    pid_(xbt_getpid())
  {}
  explicit WithContextException(Backtrace bt) : backtrace_(std::move(bt)), procname_(xbt_procname()), pid_(xbt_getpid())
  {}
  explicit WithContextException(ThrowPoint throwpoint, Backtrace bt)
      : backtrace_(std::move(bt)), procname_(xbt_procname()), pid_(xbt_getpid()), throwpoint_(throwpoint)
  {}
  virtual ~WithContextException();
  Backtrace const& backtrace() const
  {
    return backtrace_;
  }
  int pid() const { return pid_; }
  std::string const& process_name() const { return procname_; }
  ThrowPoint& throw_point() { return throwpoint_; }

  // deprecated
  XBT_ATTRIB_DEPRECATED_v323("Please use WithContextException::process_name()") std::string const& processName() const
  {
    return process_name();
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use WithContextException::throw_point()") ThrowPoint& throwPoint()
  {
    return throw_point();
  }

private:
  Backtrace backtrace_;
  std::string procname_; /**< Name of the process who thrown this */
  int pid_;              /**< PID of the process who thrown this */
  ThrowPoint throwpoint_;
};
}
}

#endif

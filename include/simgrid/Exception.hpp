/* Copyright (c) 2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_EXCEPTIONS_HPP
#define SIMGRID_EXCEPTIONS_HPP

/** @file exception.hpp SimGrid-specific Exceptions
 *
 *  Defines all possible exception that could occur in a SimGrid library.
 */

#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <xbt/backtrace.h>
#include <xbt/backtrace.hpp>
#include <xbt/base.h>
#include <xbt/ex.h>
#include <xbt/log.h>
#include <xbt/misc.h>  // xbt_procname
#include <xbt/virtu.h> // xbt_getpid

namespace simgrid {
namespace xbt {

/** The location of where an exception has been thrown
 *
 *  This is a tuple (__FILE__, __LINE__, __func__), created with @ref XBT_THROW_POINT.
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
class XBT_PUBLIC ContextedException {
public:
  ContextedException() : backtrace_(simgrid::xbt::backtrace()), procname_(xbt_procname()), pid_(xbt_getpid()) {}
  explicit ContextedException(Backtrace bt) : backtrace_(std::move(bt)), procname_(xbt_procname()), pid_(xbt_getpid())
  {
  }
  explicit ContextedException(ThrowPoint throwpoint, Backtrace bt)
      : backtrace_(std::move(bt)), procname_(xbt_procname()), pid_(xbt_getpid()), throwpoint_(throwpoint)
  {
  }
  virtual ~ContextedException();
  Backtrace const& backtrace() const { return backtrace_; }
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
} // namespace xbt

/** Ancestor class of all SimGrid exception */
class Exception : public std::runtime_error {
public:
  Exception() : std::runtime_error("") {}
  Exception(const char* message) : std::runtime_error(message) {}
};

/** Exception raised when a timeout elapsed */
class timeout_error : public simgrid::Exception {
};

/** Exception raised when an host fails */
class host_failure : public simgrid::Exception {
};

/** Exception raised when a communication fails because of the network */
class network_failure : public simgrid::Exception {
};

/** Exception raised when something got canceled before completion */
class canceled_error : public simgrid::Exception {
};

} // namespace simgrid

/** A legacy exception
 *
 *  It is defined by a category and a value within that category (as well as
 *  an optional error message).
 *
 *  This used to be a structure for C exceptions but it has been retrofitted
 *  as a C++ exception and some of its data has been moved in the
 *  @ref WithContextException base class. We should deprecate it and replace it
 *  with either C++ different exceptions or `std::system_error` which already
 *  provides this (category + error code) logic.
 *
 *  @ingroup XBT_ex_c
 */
class XBT_PUBLIC xbt_ex : public simgrid::Exception, public simgrid::xbt::ContextedException {
public:
  xbt_ex() : simgrid::Exception() {}

  /**
   *
   * @param throwpoint Throw point (use XBT_THROW_POINT)
   * @param message    Exception message
   */
  xbt_ex(simgrid::xbt::ThrowPoint throwpoint, const char* message)
      : simgrid::Exception(message), simgrid::xbt::ContextedException(throwpoint, simgrid::xbt::backtrace())
  {
  }

  ~xbt_ex(); // DO NOT define it here -- see ex.cpp for a rationale

  /** Category (what went wrong) */
  xbt_errcat_t category = unknown_error;

  /** Why did it went wrong */
  int value = 0;
};

#endif

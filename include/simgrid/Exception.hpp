/* Copyright (c) 2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_EXCEPTIONS_HPP
#define SIMGRID_EXCEPTIONS_HPP

/** @file exception.hpp SimGrid-specific Exceptions
 *
 *  Defines all possible exception that could occur in a SimGrid library.
 */

#include <xbt/backtrace.hpp>
#include <xbt/ex.h>

#include <atomic>
#include <stdexcept>
#include <string>

namespace simgrid {
namespace xbt {

/** Contextual information about an execution location (file:line:func and backtrace, procname, pid)
 *
 *  Constitute the contextual information of where an exception was thrown
 *
 *  These tuples (__FILE__, __LINE__, __func__, backtrace, procname, pid)
 *  are best created with @ref XBT_THROW_POINT.
 *
 *  @ingroup XBT_ex
 */
class ThrowPoint {
public:
  ThrowPoint() = default;
  explicit ThrowPoint(const char* file, int line, const char* function, Backtrace bt, std::string actor_name, int pid)
      : file_(file), line_(line), function_(function), backtrace_(bt), procname_(actor_name), pid_(pid)
  {
  }

  const char* file_     = nullptr;
  int line_             = 0;
  const char* function_ = nullptr;
  Backtrace backtrace_;
  std::string procname_ = ""; /**< Name of the process who thrown this */
  int pid_              = 0;  /**< PID of the process who thrown this */
};

/** Create a ThrowPoint with (__FILE__, __LINE__, __func__) */
#define XBT_THROW_POINT                                                                                                \
  ::simgrid::xbt::ThrowPoint(__FILE__, __LINE__, __func__, simgrid::xbt::backtrace(), xbt_procname(), xbt_getpid())
} // namespace xbt

/** Ancestor class of all SimGrid exception */
class Exception : public std::runtime_error {
public:
  Exception(simgrid::xbt::ThrowPoint throwpoint, std::string message)
      : std::runtime_error(message), throwpoint_(throwpoint)
  {
  }

  /** Return the information about where the exception was thrown */
  xbt::ThrowPoint const& throw_point() const { return throwpoint_; }

private:
  xbt::ThrowPoint throwpoint_;
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
class XBT_PUBLIC xbt_ex : public simgrid::Exception {
public:
  /**
   *
   * @param throwpoint Throw point (use XBT_THROW_POINT)
   * @param message    Exception message
   */
  xbt_ex(simgrid::xbt::ThrowPoint throwpoint, std::string message) : simgrid::Exception(throwpoint, message) {}

  ~xbt_ex(); // DO NOT define it here -- see ex.cpp for a rationale

  /** Category (what went wrong) */
  xbt_errcat_t category = unknown_error;

  /** Why did it went wrong */
  int value = 0;
};

namespace simgrid {

/** Exception raised when a timeout elapsed */
class TimeoutError : public xbt_ex {
public:
  TimeoutError(simgrid::xbt::ThrowPoint throwpoint, std::string message) : xbt_ex(throwpoint, message)
  {
    category = timeout_error;
  }
};

/** Exception raised when an host fails */
class HostFailureException : public xbt_ex {
public:
  HostFailureException(simgrid::xbt::ThrowPoint throwpoint, std::string message) : xbt_ex(throwpoint, message)
  {
    category = host_error;
  }
};

/** Exception raised when a communication fails because of the network */
class NetworkFailureException : public xbt_ex {
public:
  NetworkFailureException(simgrid::xbt::ThrowPoint throwpoint, std::string message) : xbt_ex(throwpoint, message)
  {
    category = network_error;
  }
};

/** Exception raised when something got canceled before completion */
class CancelException : public xbt_ex {
};

} // namespace simgrid

#endif

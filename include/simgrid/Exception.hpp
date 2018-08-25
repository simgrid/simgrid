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
#include <xbt/exception.hpp>

#include <xbt/ex.h>

namespace simgrid {

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

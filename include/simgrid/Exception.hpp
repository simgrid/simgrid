/* Copyright (c) 2018-2021. The SimGrid Team. All rights reserved.          */

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
#include <xbt/string.hpp>

#include <atomic>
#include <functional>
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
  explicit ThrowPoint(const char* file, int line, const char* function, Backtrace&& bt, std::string&& actor_name,
                      int pid)
      : file_(file)
      , line_(line)
      , function_(function)
      , backtrace_(std::move(bt))
      , procname_(std::move(actor_name))
      , pid_(pid)
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
  ::simgrid::xbt::ThrowPoint(__FILE__, __LINE__, __func__, simgrid::xbt::Backtrace(), xbt_procname(), xbt_getpid())

class XBT_PUBLIC ImpossibleError : public std::logic_error {
public:
  using std::logic_error::logic_error;
  ~ImpossibleError() override;
};

class XBT_PUBLIC InitializationError : public std::logic_error {
public:
  using std::logic_error::logic_error;
  ~InitializationError() override;
};

class XBT_PUBLIC UnimplementedError : public std::logic_error {
public:
  using std::logic_error::logic_error;
  ~UnimplementedError() override;
};

} // namespace xbt

/** Ancestor class of all SimGrid exception */
class Exception : public std::runtime_error {
public:
  Exception(const simgrid::xbt::ThrowPoint& throwpoint, const std::string& message)
      : std::runtime_error(message), throwpoint_(throwpoint)
  {
  }
  Exception(const Exception&)     = default;
  Exception(Exception&&) noexcept = default;
  ~Exception() override; // DO NOT define it here -- see Exception.cpp for a rationale

  /** Return the information about where the exception was thrown */
  xbt::ThrowPoint const& throw_point() const { return throwpoint_; }

  /** Allow to carry a value (used by testany/waitany) */
  int get_value() const { return value_; }
  void set_value(int value) { value_ = value; }

  std::string resolve_backtrace() const { return throwpoint_.backtrace_.resolve(); }

  virtual void rethrow_nested(const simgrid::xbt::ThrowPoint& throwpoint, const std::string& message) const
  {
    std::throw_with_nested(Exception(throwpoint, message));
  }

private:
  xbt::ThrowPoint throwpoint_;
  int value_ = 0;
};

#define DECLARE_SIMGRID_EXCEPTION(AnyException, ...)                                                                   \
  class AnyException : public Exception {                                                                              \
  public:                                                                                                              \
    using Exception::Exception;                                                                                        \
    __VA_ARGS__                                                                                                        \
    ~AnyException() override;                                                                                          \
    void rethrow_nested(const simgrid::xbt::ThrowPoint& throwpoint, const std::string& message) const override         \
    {                                                                                                                  \
      std::throw_with_nested(AnyException(throwpoint, message));                                                       \
    }                                                                                                                  \
  }

/** Exception raised when a timeout elapsed */
DECLARE_SIMGRID_EXCEPTION(TimeoutException);

using TimeoutError XBT_ATTRIB_DEPRECATED_v328("Please use simgrid::TimeoutException") = TimeoutException;

/** Exception raised when a host fails */
DECLARE_SIMGRID_EXCEPTION(HostFailureException);

/** Exception raised when a communication fails because of the network or because of the remote host */
DECLARE_SIMGRID_EXCEPTION(NetworkFailureException);

/** Exception raised when a storage fails */
DECLARE_SIMGRID_EXCEPTION(StorageFailureException);

/** Exception raised when a VM fails */
DECLARE_SIMGRID_EXCEPTION(VmFailureException);

/** Exception raised when something got canceled before completion */
DECLARE_SIMGRID_EXCEPTION(CancelException);

/** Exception raised when something is going wrong during the simulation tracing */
DECLARE_SIMGRID_EXCEPTION(TracingError);

/** Exception raised when something is going wrong during the parsing of XML files */
#define PARSE_ERROR_CONSTRUCTOR                                                                                        \
  ParseError(const std::string& file, int line, const std::string& msg)                                                \
      : Exception(XBT_THROW_POINT, xbt::string_printf("Parse error at %s:%d: %s", file.c_str(), line, msg.c_str()))    \
  {                                                                                                                    \
  }

DECLARE_SIMGRID_EXCEPTION(ParseError, PARSE_ERROR_CONSTRUCTOR);
#undef PARSE_ERROR_CONSTRUCTOR

#undef DECLARE_SIMGRID_EXCEPTION

class XBT_PUBLIC ForcefulKillException {
  /** @brief Exception launched to kill an actor; DO NOT BLOCK IT!
   *
   * This exception is thrown whenever the actor's host is turned off. The actor stack is properly unwinded to release
   * all objects allocated on the stack (RAII powa).
   *
   * You may want to catch this exception to perform some extra cleanups in your simulation, but YOUR ACTORS MUST NEVER
   * SURVIVE a ForcefulKillException, or your simulation will segfault.
   *
   * @verbatim
   * void* payload = malloc(512);
   *
   * try {
   *   simgrid::s4u::this_actor::execute(100000);
   * } catch (simgrid::kernel::context::ForcefulKillException& e) { // oops, my host just turned off
   *   free(malloc);
   *   throw; // I shall never survive on a host that was switched off
   * }
   * @endverbatim
   */
  /* Nope, Sonar, this should not inherit of std::exception nor of simgrid::Exception.
   * Otherwise, users may accidentally catch it with a try {} catch (std::exception)
   */
public:
  ForcefulKillException() = default;
  explicit ForcefulKillException(const std::string& msg) : msg_(std::string("Actor killed (") + msg + std::string(")."))
  {
  }
  ~ForcefulKillException();
  const char* what() const noexcept { return msg_.c_str(); }

  XBT_ATTRIB_NORETURN static void do_throw();
  static bool try_n_catch(const std::function<void()>& try_block);

private:
  std::string msg_ = std::string("Actor killed.");
};

} // namespace simgrid

using xbt_ex XBT_ATTRIB_DEPRECATED_v328("Please use simgrid::Exception") = simgrid::Exception;

#endif

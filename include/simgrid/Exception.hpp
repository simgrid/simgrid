/* Copyright (c) 2018-2020. The SimGrid Team. All rights reserved.          */

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
  explicit ImpossibleError(const std::string& arg) : std::logic_error(arg) {}
  ~ImpossibleError() override;
};

class XBT_PUBLIC InitializationError : public std::logic_error {
public:
  explicit InitializationError(const std::string& arg) : std::logic_error(arg) {}
  ~InitializationError() override;
};

class XBT_PUBLIC UnimplementedError : public std::logic_error {
public:
  explicit UnimplementedError(const std::string& arg) : std::logic_error(arg) {}
  ~UnimplementedError() override;
};

} // namespace xbt

/** Ancestor class of all SimGrid exception */
class Exception : public std::runtime_error {
public:
  Exception(simgrid::xbt::ThrowPoint&& throwpoint, std::string&& message)
      : std::runtime_error(std::move(message)), throwpoint_(std::move(throwpoint))
  {
  }
  Exception(const Exception&)     = default;
  Exception(Exception&&) noexcept = default;
  ~Exception() override; // DO NOT define it here -- see Exception.cpp for a rationale

  /** Return the information about where the exception was thrown */
  xbt::ThrowPoint const& throw_point() const { return throwpoint_; }

  std::string resolve_backtrace() const { return throwpoint_.backtrace_.resolve(); }

  /** Allow to carry a value (used by waitall/waitany) */
  int value = 0;

private:
  xbt::ThrowPoint throwpoint_;
};

/** Exception raised when a timeout elapsed */
class TimeoutException : public Exception {
public:
  TimeoutException(simgrid::xbt::ThrowPoint&& throwpoint, std::string&& message)
      : Exception(std::move(throwpoint), std::move(message))
  {
  }
  TimeoutException(const TimeoutException&)     = default;
  TimeoutException(TimeoutException&&) noexcept = default;
  ~TimeoutException() override;
};

XBT_ATTRIB_DEPRECATED_v328("Please use simgrid::TimeoutException") typedef TimeoutException TimeoutError;

/** Exception raised when a host fails */
class HostFailureException : public Exception {
public:
  HostFailureException(simgrid::xbt::ThrowPoint&& throwpoint, std::string&& message)
      : Exception(std::move(throwpoint), std::move(message))
  {
  }
  HostFailureException(const HostFailureException&)     = default;
  HostFailureException(HostFailureException&&) noexcept = default;
  ~HostFailureException() override;
};

/** Exception raised when a communication fails because of the network or because of the remote host */
class NetworkFailureException : public Exception {
public:
  NetworkFailureException(simgrid::xbt::ThrowPoint&& throwpoint, std::string&& message)
      : Exception(std::move(throwpoint), std::move(message))
  {
  }
  NetworkFailureException(const NetworkFailureException&)     = default;
  NetworkFailureException(NetworkFailureException&&) noexcept = default;
  ~NetworkFailureException() override;
};

/** Exception raised when a storage fails */
class StorageFailureException : public Exception {
public:
  StorageFailureException(simgrid::xbt::ThrowPoint&& throwpoint, std::string&& message)
      : Exception(std::move(throwpoint), std::move(message))
  {
  }
  StorageFailureException(const StorageFailureException&)     = default;
  StorageFailureException(StorageFailureException&&) noexcept = default;
  ~StorageFailureException() override;
};

/** Exception raised when a VM fails */
class VmFailureException : public Exception {
public:
  VmFailureException(simgrid::xbt::ThrowPoint&& throwpoint, std::string&& message)
      : Exception(std::move(throwpoint), std::move(message))
  {
  }
  VmFailureException(const VmFailureException&)     = default;
  VmFailureException(VmFailureException&&) noexcept = default;
  ~VmFailureException() override;
};

/** Exception raised when something got canceled before completion */
class CancelException : public Exception {
public:
  CancelException(simgrid::xbt::ThrowPoint&& throwpoint, std::string&& message)
      : Exception(std::move(throwpoint), std::move(message))
  {
  }
  CancelException(const CancelException&)     = default;
  CancelException(CancelException&&) noexcept = default;
  ~CancelException() override;
};

/** Exception raised when something is going wrong during the simulation tracing */
class TracingError : public Exception {
public:
  TracingError(simgrid::xbt::ThrowPoint&& throwpoint, std::string&& message)
      : Exception(std::move(throwpoint), std::move(message))
  {
  }
  TracingError(const TracingError&)     = default;
  TracingError(TracingError&&) noexcept = default;
  ~TracingError() override;
};

/** Exception raised when something is going wrong during the parsing of XML files */
class ParseError : public Exception {
public:
  ParseError(const std::string& file, int line, const std::string& msg)
      : Exception(XBT_THROW_POINT, xbt::string_printf("Parse error at %s:%d: %s", file.c_str(), line, msg.c_str()))
  {
  }
  ParseError(const ParseError&)     = default;
  ParseError(ParseError&&) noexcept = default;
  ~ParseError() override;
};

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
   *   throw; // I shall never survive on an host that was switched off
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

XBT_ATTRIB_DEPRECATED_v328("Please use simgrid::Exception") typedef simgrid::Exception xbt_ex;

#endif

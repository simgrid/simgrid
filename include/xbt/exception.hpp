/* Copyright (c) 2005-2016. The SimGrid Team.All rights reserved. */

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

/** The location of where an exception has been throwed
 *
 *  This is a tuple (__FILE__, __LINE__, __func__) and can be created with
 *  @ref XBT_THROW_POINT.
 *
 *  @ingroup XBT_ex
 */
struct ThrowPoint {
  ThrowPoint() {}
  ThrowPoint(const char* file, int line, const char* function) :
    file(file), line(line), function(function) {}
  const char* file = nullptr;
  int line = 0;
  const char* function = nullptr;
};

/** Create a ThrowPoint with (__FILE__, __LINE__, __func__) */
#define XBT_THROW_POINT ::simgrid::xbt::ThrowPoint(__FILE__, __LINE__, __func__)

/** A base class for exceptions with context
 *
 *  This is a base class for exceptions which store additional contextual
 *  infomations about them: backtrace, throw point, simulated process name
 *  and PID, etc.
 *
 *  You are not expected to inherit from it. Instead of you use should
 *  @ref XBT_THROW an exception which will throw a subclass of your original
 *  exception with those additional features.
 * 
 *  However, you can try `dynamic_cast` an exception to this type in order to
 *  get contextual information about the exception.
 */
XBT_PUBLIC_CLASS WithContextException {
public:
  WithContextException() :
    backtrace_(simgrid::xbt::backtrace()),
    procname_(xbt_procname()),
    pid_(xbt_getpid())
  {}
  WithContextException(Backtrace bt) :
    backtrace_(std::move(bt)),
    procname_(xbt_procname()),
    pid_(xbt_getpid())
  {}
  WithContextException(ThrowPoint throwpoint, Backtrace bt) :
    backtrace_(std::move(bt)),
    procname_(xbt_procname()),
    pid_(xbt_getpid()),
    throwpoint_(throwpoint)
  {}
  virtual ~WithContextException();
  Backtrace const& backtrace() const
  {
    return backtrace_;
  }
  int pid() const { return pid_; }
  std::string const& processName() const { return procname_; }
  ThrowPoint& throwPoint() { return throwpoint_; }
private:
  Backtrace backtrace_;
  std::string procname_; /**< Name of the process who thrown this */
  int pid_;              /**< PID of the process who thrown this */
  ThrowPoint throwpoint_;
};

/** Internal class used to mixin an exception E with WithContextException */
template<class E>
class WithContext : public E, public WithContextException
{
public:

  static_assert(!std::is_base_of<WithContextException,E>::value,
    "Trying to appli WithContext twice");

  WithContext(E exception) :
    E(std::move(exception)) {}
  WithContext(E exception, ThrowPoint throwpoint, Backtrace backtrace) :
    E(std::move(exception)),
    WithContextException(throwpoint, std::move(backtrace)) {}
  WithContext(E exception, Backtrace backtrace) :
    E(std::move(exception)),
    WithContextException(std::move(backtrace)) {}
  WithContext(E exception, WithContextException context) :
    E(std::move(exception)),
    WithContextException(std::move(context)) {}
  ~WithContext() override {}
};

/** Throw a C++ exception with some context
 *
 *  @param e Exception to throw
 *  @ingroup XBT_ex
 */
#define XBT_THROW(e) \
  throw WithContext<E>(std::move(exception), throwpoint, simgrid::xbt::backtrace())

/** Throw a C++ exception with a context and a nexted exception/cause
 *
 *  @param e Exception to throw
 *  @ingroup XBT_ex
 */
#define XBT_THROW_NESTED(e) \
  std::throw_with_nested(WithContext<E>(std::move(exception), throwpoint, simgrid::xbt::backtrace()))

}
}

#endif

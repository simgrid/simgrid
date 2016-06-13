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

namespace simgrid {
namespace xbt {

typedef std::vector<xbt_backtrace_location_t> Backtrace;

/** A polymorphic mixin class for adding context to an exception */
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
  virtual ~WithContextException();
  Backtrace const& backtrace() const
  {
    return backtrace_;
  }
  int pid() const { return pid_; }
  std::string const& processName() const { return procname_; }
private:
  Backtrace backtrace_;
  std::string procname_; /**< Name of the process who thrown this */
  int pid_;              /**< PID of the process who thrown this */
};

/** Internal class used to mixin the two classes */
template<class E>
class WithContext : public E, public WithContextException
{
public:
  WithContext(E exception)
    : E(std::move(exception)) {}
  WithContext(E exception, Backtrace backtrace)
    : E(std::move(exception)), WithContextException(std::move(backtrace)) {}
  WithContext(E exception, WithContextException context)
    : E(std::move(exception)), WithContextException(std::move(context)) {}
  ~WithContext() override {}
};

/** Throw a given exception a context
 *
 *  @param exception exception to throw
 *  @param backtrace backtrace to attach
 */
template<class E>
[[noreturn]] inline
typename std::enable_if< !std::is_base_of<WithContextException,E>::value >::type
throwWithContext(
  E exception,
  // Thanks to the default argument, we are taking the backtrace in the caller:
  Backtrace backtrace = simgrid::xbt::backtrace())
{
 throw WithContext<E>(std::move(exception), std::move(backtrace));
}

}
}

#endif

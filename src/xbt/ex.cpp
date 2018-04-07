/* ex - Exception Handling                                                  */

/* Copyright (c) 2005-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdio>
#include <cstdlib>

#include <xbt/backtrace.hpp>
#include "src/internal_config.h"           /* execinfo when available */
#include "xbt/ex.h"
#include <xbt/ex.hpp>
#include "xbt/log.h"
#include "xbt/log.hpp"
#include "xbt/backtrace.h"
#include "xbt/backtrace.hpp"
#include "src/xbt_modinter.h"       /* backtrace initialization headers */

#include "simgrid/sg_config.hpp" /* Configuration mechanism of SimGrid */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_ex, xbt, "Exception mechanism");

// DO NOT define ~xbt_ex() in ex.hpp.
// Defining it here ensures that xbt_ex is defined only in libsimgrid, but not in libsimgrid-java.
// Doing otherwise naturally breaks things (at least on freebsd with clang).

xbt_ex::~xbt_ex() = default;

void _xbt_throw(char* message, xbt_errcat_t errcat, int value, const char* file, int line, const char* func)
{
  xbt_ex e(simgrid::xbt::ThrowPoint(file, line, func), message);
  xbt_free(message);
  e.category = errcat;
  e.value = value;
  throw e;
}

/** @brief shows an exception content and the associated stack if available */
void xbt_ex_display(xbt_ex_t * e)
{
  simgrid::xbt::logException(xbt_log_priority_critical, "UNCAUGHT EXCEPTION", *e);
}

/** \brief returns a short name for the given exception category */
const char *xbt_ex_catname(xbt_errcat_t cat)
{
  switch (cat) {
  case unknown_error:
    return "unknown error";
  case arg_error:
    return "invalid argument";
  case bound_error:
    return "out of bounds";
  case mismatch_error:
    return "mismatch";
  case not_found_error:
    return "not found";
  case system_error:
    return "system error";
  case network_error:
    return "network error";
  case timeout_error:
    return "timeout";
  case cancel_error:
    return "action canceled";
  case thread_error:
    return "thread error";
  case host_error:
    return "host failed";
  case tracing_error:
    return "tracing error";
  case io_error:
    return "io error";
  case vm_error:
    return "vm error";
  default:
    return "INVALID ERROR";
  }
  return "INVALID ERROR";
}

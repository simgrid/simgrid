/* Copyright (c) 2016-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_XBT_ASSERTS_HPP
#define SIMGRID_XBT_ASSERTS_HPP

#include <simgrid/Exception.hpp>

/**
 * @brief Those are the SimGrid version of the good ol' assert macro.
 *
 * You can pass them a format message and arguments, just as if it where a printf.
 *
 * If the statement evaluates to false, then a simgrid::AsertionError is thrown.
 * This is identical to the xbt_assert macro, except that an exception is thrown instead of calling abort().
 *
 * Unlike the standard assert, xbt_enforce is never disabled, even if the macro NDEBUG is defined at compile time.
 * Note however that this macro should *not* be used with a condition that has side effects, since the exception can be
 * caught and ignored.
 */
/** @brief The condition which failed will be displayed.
    @hideinitializer  */
#define xbt_enforce(...) \
  _XBT_IF_ONE_ARG(_xbt_enforce_ARG1, _xbt_enforce_ARGN, __VA_ARGS__)(__VA_ARGS__)
#define _xbt_enforce_ARG1(cond) _xbt_enforce_ARGN((cond), "Assertion %s failed", #cond)
#define _xbt_enforce_ARGN(cond, ...)                                                                                   \
  do {                                                                                                                 \
    if (not(cond)) {                                                                                                   \
      throw simgrid::AssertionError(XBT_THROW_POINT, xbt::string_printf(__VA_ARGS__));                                 \
    }                                                                                                                  \
  } while (0)

#endif

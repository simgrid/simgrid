/*  xbt/asserts.h -- assertion mechanism                                    */

/* Copyright (c) 2005-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_ASSERTS_H
#define XBT_ASSERTS_H

#include <stdlib.h>
#include <xbt/base.h>
#include <xbt/log.h>

SG_BEGIN_DECL
XBT_PUBLIC_DATA int xbt_log_no_loc; /* Do not show the backtrace on failed backtrace when doing our tests */

XBT_PUBLIC void xbt_backtrace_display_current();

/**
 * @addtogroup XBT_error
 *
 * @{
 */
/** @brief Kill the program in silence */
XBT_ATTRIB_NORETURN XBT_PUBLIC void xbt_abort(void);

/**
 * @brief Kill the program with an error message
 * @param ... a format string and its arguments
 *
 * Things are so messed up that the only thing to do now, is to stop the program.
 *
 * The message is handled by a CRITICAL logging request, and may consist of a format string with arguments.
 */
#define xbt_die(...)                                                                                                   \
  do {                                                                                                                 \
    XBT_CCRITICAL(root, __VA_ARGS__);                                                                                  \
    xbt_abort();                                                                                                       \
  } while (0)

/**
 * @brief Those are the SimGrid version of the good ol' assert macro.
 *
 * You can pass them a format message and arguments, just as if it where a printf.
 * It is converted to a XBT_CRITICAL logging request.
 * An execution backtrace is also displayed, unless the option --log=no_loc is given at run-time.
 *
 * Unlike the standard assert, xbt_assert is never disabled, even if the macro NDEBUG is defined at compile time.  So
 * it's safe to have a condition with side effects.
 */
/** @brief The condition which failed will be displayed.
    @hideinitializer  */
#define xbt_assert(...) \
  _XBT_IF_ONE_ARG(_xbt_assert_ARG1, _xbt_assert_ARGN, __VA_ARGS__)(__VA_ARGS__)
#define _xbt_assert_ARG1(cond) _xbt_assert_ARGN((cond), "Assertion %s failed", #cond)
#define _xbt_assert_ARGN(cond, ...)                                                                                    \
  do {                                                                                                                 \
    if (!(cond)) {                                                                                                     \
      XBT_CCRITICAL(root, __VA_ARGS__);                                                                                \
      if (!xbt_log_no_loc)                                                                                             \
        xbt_backtrace_display_current();                                                                               \
      abort();                                                                                                         \
    }                                                                                                                  \
  } while (0)

/** @} */
SG_END_DECL
#endif                          /* XBT_ASSERTS_H */

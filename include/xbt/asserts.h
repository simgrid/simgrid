/*  xbt/asserts.h -- assertion mechanism                                    */

/* Copyright (c) 2005-2007, 2009-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_ASSERTS_H
#define _XBT_ASSERTS_H

#include "xbt/misc.h"
#include "xbt/log.h"
#include "xbt/ex.h"

SG_BEGIN_DECL()
/**
 * @addtogroup XBT_error
 * @brief Those are the SimGrid version of the good ol' assert macro.
 *
 * You can pass them a format message and arguments, just as if it where a printf.
 * It is converted to a XBT_CRITICAL logging request.
 * Be careful: the boolean expression that you want to test should not have side effects, because assertions are
 * disabled at compile time if NDEBUG is set.
 *
 * @{
 */
#ifdef NDEBUG
#define xbt_assert(...) ((void)0)
#else
   /** @brief The condition which failed will be displayed.
   @hideinitializer  */
#define xbt_assert(...) \
  _XBT_IF_ONE_ARG(_xbt_assert_ARG1, _xbt_assert_ARGN, __VA_ARGS__)(__VA_ARGS__)
#define _xbt_assert_ARG1(cond) \
  _xbt_assert_ARGN(cond, "Assertion %s failed", #cond)
#define _xbt_assert_ARGN(cond, ...) \
  do { if (!(cond)) THROWF(0, 0, __VA_ARGS__); } while (0)
#endif

/** @} */
SG_END_DECL()
#endif                          /* _XBT_ASSERTS_H */

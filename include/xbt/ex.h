/* Copyright (c) 2005-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_EX_H
#define XBT_EX_H

#include <stdlib.h>

#include <xbt/base.h>
#include <xbt/misc.h>
#include <xbt/sysdep.h>
#include <xbt/virtu.h>

/** @addtogroup XBT_ex_c
 *  @brief Exceptions support (C)
 *
 *  Those functions are used to throw C++ exceptions from C code. This feature
 *  should probably be removed in the future because C and exception do not
 *  exactly play nicely together.
 */

SG_BEGIN_DECL

/** Helper function used to throw exceptions in C */
XBT_ATTRIB_NORETURN XBT_PUBLIC void _xbt_throw(char* message, int value, const char* file, int line, const char* func);

/** Builds and throws an exception
 *  @ingroup XBT_ex_c
 *  @hideinitializer
 */
#define THROW(v) _xbt_throw(NULL, (v), __FILE__, __LINE__, __func__)

/** Builds and throws an exception with a printf-like formatted message
 *  @ingroup XBT_ex_c
 *  @hideinitializer
 */
#define THROWF(v, ...) _xbt_throw(bprintf(__VA_ARGS__), (v), __FILE__, __LINE__, __func__)

XBT_ATTRIB_NORETURN void xbt_throw_impossible(const char* file, int line, const char* func);
/** Throw an exception because something impossible happened
 *  @ingroup XBT_ex_c
 */
#define THROW_IMPOSSIBLE xbt_throw_impossible(__FILE__, __LINE__, __func__)

/** Throw an exception because something unimplemented stuff has been attempted
 *  @ingroup XBT_ex_c
 */
XBT_ATTRIB_NORETURN XBT_PUBLIC void xbt_throw_unimplemented(const char* file, int line, const char* func);
#define THROW_UNIMPLEMENTED xbt_throw_unimplemented(__FILE__, __LINE__, __func__)

/** Die because something impossible happened
 *  @ingroup XBT_ex_c
 */
#define DIE_IMPOSSIBLE xbt_die("The Impossible Did Happen (yet again)")

SG_END_DECL

/** @} */
#endif                          /* __XBT_EX_H__ */

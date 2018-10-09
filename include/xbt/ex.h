/* Copyright (c) 2005-2018. The SimGrid Team. All rights reserved.          */

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
 *  Those fonctions are used to throw C++ exceptions from C code. This feature
 *  should probably be removed in the future because C and exception do not
 *  exactly play nicely together.
 */

/** Categories of errors
 *
 *  This very similar to std::error_catgory and should probably be replaced
 *  by this in the future.
 *
 *  @ingroup XBT_ex_c
 */
typedef enum {
  unknown_error = 0,            /**< unknown error */
  arg_error,                    /**< Invalid argument */
  bound_error,                  /**< Out of bounds argument */
  mismatch_error,               /**< The provided ID does not match */
  not_found_error,              /**< The searched element was not found */
  system_error,                 /**< a syscall did fail */
  network_error,                /**< error while sending/receiving data */
  timeout_error,                /**< not quick enough, dude */
  cancel_error,                 /**< an action was canceled */
  thread_error,                 /**< error while [un]locking */
  host_error,                   /**< host failed */
  tracing_error,                /**< error during the simulation tracing */
  io_error,                     /**< disk or file error */
  vm_error                      /**< vm  error */
} xbt_errcat_t;

SG_BEGIN_DECL()

/** Get the name of a category
 *  @ingroup XBT_ex_c
 */
XBT_PUBLIC const char* xbt_ex_catname(xbt_errcat_t cat);

typedef struct xbt_ex xbt_ex_t;

/** Helper function used to throw exceptions in C */
XBT_ATTRIB_NORETURN XBT_PUBLIC void _xbt_throw(char* message, xbt_errcat_t errcat, int value, const char* file,
                                               int line, const char* func);

/** Builds and throws an exception
 *  @ingroup XBT_ex_c
 *  @hideinitializer
 */
#define THROW(c, v)             { _xbt_throw(NULL, (xbt_errcat_t) c, v, __FILE__, __LINE__, __func__); }

/** Builds and throws an exception with a printf-like formatted message
 *  @ingroup XBT_ex_c
 *  @hideinitializer
 */
#define THROWF(c, v, ...)       _xbt_throw(bprintf(__VA_ARGS__), (xbt_errcat_t) c, v, __FILE__, __LINE__, __func__)

XBT_ATTRIB_NORETURN void xbt_throw_impossible(const char* file, int line, const char* func);
/** Throw an exception because something impossible happened
 *  @ingroup XBT_ex_c
 */
#define THROW_IMPOSSIBLE xbt_throw_impossible(__FILE__, __LINE__, __func__)

/** Throw an exception because something unimplemented stuff has been attempted
 *  @ingroup XBT_ex_c
 */
XBT_ATTRIB_NORETURN void xbt_throw_unimplemented(const char* file, int line, const char* func);
#define THROW_UNIMPLEMENTED xbt_throw_unimplemented(__FILE__, __LINE__, __func__)

/** Die because something impossible happened
 *  @ingroup XBT_ex_c
 */
#define DIE_IMPOSSIBLE xbt_die("The Impossible Did Happen (yet again)")

/** Display an exception */
XBT_PUBLIC void xbt_ex_display(xbt_ex_t* e);

SG_END_DECL()

/** @} */
#endif                          /* __XBT_EX_H__ */

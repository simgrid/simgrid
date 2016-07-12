/* Copyright (c) 2005-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/*  Copyright (c) 2002-2004 Ralf S. Engelschall <rse@engelschall.com>       */
/*  Copyright (c) 2002-2004 The OSSP Project <http://www.ossp.org/>         */
/*  Copyright (c) 2002-2004 Cable & Wireless <http://www.cw.com/>           */
/*  All rights reserved.                                                    */

/* This code is inspirated from the OSSP version (as retrieved back in 2004)*/
/* It was heavily modified to fit the SimGrid framework.                    */

/* The OSSP version has the following copyright notice:
**  OSSP ex - Exception Handling
**  Copyright (c) 2002-2004 Ralf S. Engelschall <rse@engelschall.com>
**  Copyright (c) 2002-2004 The OSSP Project <http://www.ossp.org/>
**  Copyright (c) 2002-2004 Cable & Wireless <http://www.cw.com/>
**
**  This file is part of OSSP ex, an exception handling library
**  which can be found at http://www.ossp.org/pkg/lib/ex/.
**
**  Permission to use, copy, modify, and distribute this software for
**  any purpose with or without fee is hereby granted, provided that
**  the above copyright notice and this permission notice appear in all
**  copies.
**
**  THIS SOFTWARE IS PROVIDED `AS IS'' AND ANY EXPRESSED OR IMPLIED
**  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
**  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
**  IN NO EVENT SHALL THE AUTHORS AND COPYRIGHT HOLDERS AND THEIR
**  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
**  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
**  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
**  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
**  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
**  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
**  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
**  SUCH DAMAGE.
 */

/* The extensions made for the SimGrid project can either be distributed    */
/* under the same license, or under the LGPL v2.1                           */

#ifndef __XBT_EX_H__
#define __XBT_EX_H__

#include <stdlib.h>

#include "xbt/base.h"
#include "xbt/sysdep.h"
#include "xbt/misc.h"
#include "xbt/virtu.h"

/*-*-* Emergency debuging: define this when the exceptions get crazy *-*-*/
#undef __EX_MAYDAY
#ifdef __EX_MAYDAY
# include <stdio.h>
#include <errno.h>
#  define MAYDAY_SAVE(m)    printf("%d %s:%d save %p\n",                \
                                   xbt_getpid(), __FILE__, __LINE__,    \
                                   (m)->jb                              \
                                  ),
#  define MAYDAY_RESTORE(m) printf("%d %s:%d restore %p\n",             \
                                   xbt_getpid(), __FILE__, __LINE__,    \
                                   (m)->jb                              \
                                  ),
#  define MAYDAY_CATCH(e)   printf("%d %s:%d Catched '%s'\n",           \
                                   xbt_getpid(), __FILE__, __LINE__,    \
                                   (e).msg                              \
          ),
#else
#  define MAYDAY_SAVE(m)
#  define MAYDAY_RESTORE(m)
#  define MAYDAY_CATCH(e)
#endif
/*-*-* end of debugging stuff *-*-*/

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
XBT_PUBLIC(const char *) xbt_ex_catname(xbt_errcat_t cat);

typedef struct xbt_ex xbt_ex_t;

/** Helper function used to throw exceptions in C */
XBT_PUBLIC(void) _xbt_throw(char* message, xbt_errcat_t errcat, int value, const char* file, int line, const char* func) XBT_ATTRIB_NORETURN;

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

/** Throw an exception because someting impossible happened
 *  @ingroup XBT_ex_c
 */
#define THROW_IMPOSSIBLE \
  THROWF(unknown_error, 0, "The Impossible Did Happen (yet again)")

/** Throw an exception because someting unimplemented stuff has been attempted
 *  @ingroup XBT_ex_c
 */
#define THROW_UNIMPLEMENTED \
  THROWF(unknown_error, 0, "Function %s unimplemented",__func__)

/** Throw an exception because some dead code was reached
 *  @ingroup XBT_ex_c
 */
#define THROW_DEADCODE \
  THROWF(unknown_error, 0, "Function %s was supposed to be DEADCODE, but it's not",__func__)

/** Die because something impossible happened
 *  @ingroup XBT_ex_c
 */
#define DIE_IMPOSSIBLE xbt_die("The Impossible Did Happen (yet again)")

/** Display an exception */
XBT_PUBLIC(void) xbt_ex_display(xbt_ex_t * e);

SG_END_DECL()

/** @} */
#endif                          /* __XBT_EX_H__ */

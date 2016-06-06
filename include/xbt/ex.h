/* ex - Exception Handling                                                  */

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

#ifdef __cplusplus
#include <string>
#include <vector>
#include <stdexcept>
#endif

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

/** @brief different kind of errors */
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

#ifdef __cplusplus
XBT_PUBLIC_CLASS xbt_ex : public std::runtime_error {
public:
  xbt_ex() : std::runtime_error("") {}
  xbt_ex(const char* message) : std::runtime_error(message) {}
  ~xbt_ex() override;

  xbt_errcat_t category;        /**< category like HTTP (what went wrong) */
  int value;                    /**< like errno (why did it went wrong) */
  /* throw point */
  std::string procname;         /**< Name of the process who thrown this */
  int pid;                      /**< PID of the process who thrown this */
  const char *file;             /**< Thrown point */
  int line;                     /**< Thrown point */
  const char *func;             /**< Thrown point */
  /* Backtrace */
  std::vector<std::string>      bt_strings;
  std::vector<void*>            bt;
};
#endif

SG_BEGIN_DECL()

XBT_PUBLIC(const char *) xbt_ex_catname(xbt_errcat_t cat);

typedef struct xbt_ex xbt_ex_t;

XBT_PUBLIC(void) xbt_throw(char* message, xbt_errcat_t errcat, int value, const char* file, int line, const char* func) XBT_ATTRIB_NORETURN;

/** @brief Builds and throws an exception
    @hideinitializer */
#define THROW(c, v)             { xbt_throw(NULL, (xbt_errcat_t) c, v, __FILE__, __LINE__, __func__); }

/** @brief Builds and throws an exception with a printf-like formatted message
    @hideinitializer */
#define THROWF(c, v, ...)       xbt_throw(bprintf(__VA_ARGS__), (xbt_errcat_t) c, v, __FILE__, __LINE__, __func__)

#define THROW_IMPOSSIBLE \
  THROWF(unknown_error, 0, "The Impossible Did Happen (yet again)")
#define THROW_UNIMPLEMENTED \
  THROWF(unknown_error, 0, "Function %s unimplemented",__func__)
#define THROW_DEADCODE \
  THROWF(unknown_error, 0, "Function %s was supposed to be DEADCODE, but it's not",__func__)

#define DIE_IMPOSSIBLE xbt_die("The Impossible Did Happen (yet again)")

/** @brief The display made by an exception that is not catched */
XBT_PUBLIC(void) xbt_ex_display(xbt_ex_t * e);

/** @brief Shows a backtrace of the current location */
XBT_PUBLIC(void) xbt_backtrace_display_current(void);
/** @brief reimplementation of glibc backtrace based directly on gcc library, without implicit malloc  */
XBT_PUBLIC(int) xbt_backtrace_no_malloc(void**bt, int size);
/** @brief Captures a backtrace for further use */
XBT_PUBLIC(void) xbt_backtrace_current(xbt_ex_t * e);
/** @brief Display a previously captured backtrace */
XBT_PUBLIC(void) xbt_backtrace_display(xbt_ex_t * e);
/** @brief Get current backtrace with libunwind */
XBT_PUBLIC(int) xbt_libunwind_backtrace(void *bt[XBT_BACKTRACE_SIZE], int size);

SG_END_DECL()

/** @} */
#endif                          /* __XBT_EX_H__ */

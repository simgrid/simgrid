/* xbt.h - Public interface to the xbt (simgrid's toolbox)                     */

/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_MISC_H
#define XBT_MISC_H

#include "simgrid_config.h"
#include "base.h"
#include "dynar.h"

#include <stdarg.h>

SG_BEGIN_DECL()

/** Cache the size of a memory page for the current system. */
XBT_PUBLIC_DATA(int) xbt_pagesize;

/** Cache the number of bits of addresses inside a given page, log2(xbt_pagesize). */
XBT_PUBLIC_DATA(int) xbt_pagebits;

XBT_PUBLIC(const char *) xbt_procname(void);

/** Retrieves the version numbers of the used dynamic library (so, DLL or dynlib) , while
    SIMGRID_VERSION_MAJOR and friends give the version numbers of the used header files */
XBT_PUBLIC(void) sg_version(int *major,int *minor,int *patch);

/** Helps ensuring that header version (SIMGRID_VERSION_MAJOR and friends) and dynamic library version do match. */
XBT_PUBLIC(void) sg_version_check(int lib_version_major,int lib_version_minor,int lib_version_patch);

/** Contains all the parameters we got from the command line */
XBT_PUBLIC_DATA(xbt_dynar_t) sg_cmdline;

/* snprintf related functions */
/** @addtogroup XBT_str
  * @{ */

/** @brief print to allocated string
 *
 * Similar to vasprintf(), but returns a pointer to the newly created string (or aborts on error).
 */
XBT_PUBLIC(char *) bvprintf(const char *fmt, va_list ap);
/** @brief print to allocated string
 *
 * Similar to asprintf(), but returns a pointer to the newly created string (or aborts on error).
 */
XBT_PUBLIC(char *) bprintf(const char *fmt, ...) XBT_ATTRIB_PRINTF(1, 2);
/** @} */

SG_END_DECL()

#endif                          /* XBT_MISC_H */

/* Copyright (c) 2005-2016. The SimGrid Team. All rights reserved.          */

#ifndef SIMGRIX_XBT_BACKTRACE_H
#define SIMGRIX_XBT_BACKTRACE_H

#include <cstddef>

#include <xbt/base.h>

SG_BEGIN_DECL()

typedef void* xbt_backtrace_location_t;

/** @brief Shows a backtrace of the current location */
XBT_PUBLIC(void) xbt_backtrace_display_current(void);

/** @brief reimplementation of glibc backtrace based directly on gcc library, without implicit malloc  */
XBT_PUBLIC(int) xbt_backtrace_no_malloc(void**bt, int size);

/** @brief Captures a backtrace for further use */
XBT_PUBLIC(size_t) xbt_backtrace_current(xbt_backtrace_location_t* loc, size_t count);

/** @brief Display a previously captured backtrace */
XBT_PUBLIC(void) xbt_backtrace_display(xbt_backtrace_location_t* loc, size_t count);

/** @brief Get current backtrace with libunwind */
XBT_PUBLIC(int) xbt_libunwind_backtrace(void** bt, int size);

SG_END_DECL()

#endif

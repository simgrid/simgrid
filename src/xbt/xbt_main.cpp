/* module handling                                                          */

/* Copyright (c) 2006-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define XBT_LOG_LOCALLY_DEFINE_XBT_CHANNEL /* MSVC don't want it to be declared extern in headers and local here */

#include "simgrid/config.h"
#include "src/internal_config.h"
#include "src/simgrid/sg_config.hpp"
#include "src/sthread/sthread.h" // sthread_inside_simgrid
#include "src/xbt/coverage.h"
#include "xbt/config.hpp"
#include "xbt/dynar.h"
#include "xbt/log.h"
#include "xbt/log.hpp"
#include "xbt/misc.h"
#include "xbt/sysdep.h"

#include <cmath>
#include <cstdio>
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string>
#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(module, xbt, "module handling");

XBT_LOG_NEW_CATEGORY(smpi, "All SMPI categories"); /* lives here even if that's a bit odd to solve linking issues: this is used in xbt_log_file_appender to detect whether SMPI is used (and thus whether we should unbench the writing to disk) */

namespace simgrid::xbt {
std::string binary_name;          /* Name of the system process containing us (mandatory to retrieve neat backtraces) */
std::vector<std::string> cmdline; /* all we got in argv */
} // namespace simgrid::xbt

const int xbt_pagesize = static_cast<int>(sysconf(_SC_PAGESIZE));
const int xbt_pagebits = static_cast<int>(log2(xbt_pagesize));

XBT_ATTRIB_NOINLINE void sthread_enable()
{ // These symbols are used from ContextSwapped in any case, but they are only useful
  asm("");
}
XBT_ATTRIB_NOINLINE void sthread_disable()
{ //  when libsthread is LD_PRELOADED. In this case, sthread's implem gets used instead.
  asm("");
}

/* these two functions belong to xbt/sysdep.h, which have no corresponding .c file */
/** @brief like xbt_free, but you can be sure that it is a function  */
void xbt_free_f(void* p) noexcept(noexcept(::free))
{
  xbt_free(p);
}

/** @brief should be given a pointer to pointer, and frees the second one */
void xbt_free_ref(void* d) noexcept(noexcept(::free))
{
  xbt_free(*(void**)d);
}

/** @brief Kill the program in silence */
void xbt_abort()
{
  /* Call __gcov_flush on abort when compiling with coverage options. */
  coverage_checkpoint();
  abort();
}

#ifndef HAVE_SMPI
int SMPI_is_inited()
{
  return false;
}
#endif

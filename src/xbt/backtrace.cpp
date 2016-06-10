/* Copyright (c) 2005-2016. The SimGrid Team. All rights reserved.          */

#include <cstddef>

#include <xbt/log.h>
#include <xbt/backtrace.h>

#include "src/internal_config.h"

extern "C" {

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_backtrace, xbt, "Backtrace");

}

/** @brief show the backtrace of the current point (lovely while debuging) */
void xbt_backtrace_display_current(void)
{
  const std::size_t size = 10;
  xbt_backtrace_location_t bt[size];
  size_t used = xbt_backtrace_current(bt, size);
  xbt_backtrace_display(bt, used);
}

#if HAVE_BACKTRACE && HAVE_EXECINFO_H && HAVE_POPEN && defined(ADDR2LINE)
# include "src/xbt/backtrace_linux.cpp"
#else
# include "src/xbt/backtrace_dummy.cpp"
#endif

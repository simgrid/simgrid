/* layout_simple - a dumb log layout                                        */

/* Copyright (c) 2007-2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/strbuff.h"        /* For dynamic version when the static one fails */
#include "xbt/log_private.h"
#include "xbt/synchro.h"        /* xbt_thread_name */

#include "gras/virtu.h"
#include <stdio.h>
#include "portable.h"

extern const char *xbt_log_priority_names[8];
extern int xbt_log_no_loc;

static double simple_begin_of_time = -1;

#define XBT_LOG_BUFF_SIZE (ev->buffer_size)

/* only used after the format using: we suppose that the buffer is big enough to display our data */
#undef check_overflow
#define check_overflow \
  if (p - ev->buffer >= XBT_LOG_BUFF_SIZE) { /* buffer overflow */ \
    return 0;                                                      \
  } else ((void)0)

static int xbt_log_layout_simple_doit(xbt_log_layout_t l,
                                      xbt_log_event_t ev,
                                      const char *fmt)
{
  char *p;
  const char *procname = NULL;
  xbt_assert(ev->priority >= 0,
              "Negative logging priority naturally forbidden");
  xbt_assert(ev->priority < sizeof(xbt_log_priority_names),
              "Priority %d is greater than the biggest allowed value",
              ev->priority);

  p = ev->buffer;
  p += snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "[");
  check_overflow;

  /* Display the proc info if available */
  procname = xbt_procname();
  if (strlen(procname)) {
    p += snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "%s:%s:(%d) ",
                  gras_os_myname(), procname, (*xbt_getpid) ());
    check_overflow;
  }

  /* Display the date */
  p += snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "%f] ",
                gras_os_time() - simple_begin_of_time);
  check_overflow;

  /* Display file position if not INFO */
  if (ev->priority != xbt_log_priority_info && !xbt_log_no_loc) {
    p += snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "%s:%d: ",
                  ev->fileName, ev->lineNum);
    check_overflow;
  }

  /* Display category name */
  p += snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "[%s/%s] ",
                ev->cat->name, xbt_log_priority_names[ev->priority]);
  check_overflow;

  /* Display user-provided message */
  p += vsnprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), fmt, ev->ap);
  check_overflow;

  /* End it */
  p += snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "\n");
  check_overflow;

  return 1;
}

xbt_log_layout_t xbt_log_layout_simple_new(char *arg)
{
  xbt_log_layout_t res = xbt_new0(s_xbt_log_layout_t, 1);
  res->do_layout = xbt_log_layout_simple_doit;

  if (simple_begin_of_time < 0)
    simple_begin_of_time = gras_os_time();

  return res;
}

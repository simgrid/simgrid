/* layout_simple - a dumb log layout                                        */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
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

static void xbt_log_layout_simple_dynamic(xbt_log_layout_t l,
                                          xbt_log_event_t ev,
                                          const char *fmt,
                                          xbt_log_appender_t app)
{
  xbt_strbuff_t buff = xbt_strbuff_new();
  char loc_buff[256];
  char *p;

  /* Put every static information in a static buffer, and copy them in the dyn one */
  p = loc_buff;
  p += snprintf(p, 256 - (p - loc_buff), "[");

  if (strlen(xbt_procname()))
    p += snprintf(p, 256 - (p - loc_buff), "%s:%s:(%d) ",
                  gras_os_myname(), xbt_procname(), (*xbt_getpid) ());
  p +=
    snprintf(p, 256 - (p - loc_buff), "%f] ",
             gras_os_time() - simple_begin_of_time);
  if (ev->priority != xbt_log_priority_info && xbt_log_no_loc==0)
    p +=
      snprintf(p, 256 - (p - loc_buff), "%s:%d: ", ev->fileName,
               ev->lineNum);
  p +=
    snprintf(p, 256 - (p - loc_buff), "[%s/%s] ", ev->cat->name,
             xbt_log_priority_names[ev->priority]);

  xbt_strbuff_append(buff, loc_buff);

  vasprintf(&p, fmt, ev->ap_copy);
  xbt_strbuff_append(buff, p);
  free(p);

  xbt_strbuff_append(buff, "\n");

  app->do_append(app, buff->data);
  xbt_strbuff_free(buff);
}

/* only used after the format using: we suppose that the buffer is big enough to display our data */
#undef check_overflow
#define check_overflow \
  if (p-ev->buffer > XBT_LOG_BUFF_SIZE) { /* buffer overflow */ \
  xbt_log_layout_simple_dynamic(l,ev,fmt,app); \
  return; \
  }

static void xbt_log_layout_simple_doit(xbt_log_layout_t l,
                                       xbt_log_event_t ev,
                                       const char *fmt,
                                       xbt_log_appender_t app)
{
  char *p;
  const char *procname=NULL;
  xbt_assert0(ev->priority >= 0,
              "Negative logging priority naturally forbidden");
  xbt_assert1(ev->priority < sizeof(xbt_log_priority_names),
              "Priority %d is greater than the biggest allowed value",
              ev->priority);

  p = ev->buffer;
  p += snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "[");
  check_overflow;

  /* Display the proc info if available */
  procname=xbt_procname();
  if (strlen(procname)) {
    p += snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "%s:%s:(%d) ",
                  gras_os_myname(), procname, (*xbt_getpid) ());
    check_overflow;
  }

  /* Display the date */
  p +=
    snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "%f] ",
             gras_os_time() - simple_begin_of_time);
  check_overflow;

  /* Display file position if not INFO */
  if (ev->priority != xbt_log_priority_info && !xbt_log_no_loc)
    p +=
      snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "%s:%d: ",
               ev->fileName, ev->lineNum);

  /* Display category name */
  p += snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "[%s/%s] ",
                ev->cat->name, xbt_log_priority_names[ev->priority]);

  /* Display user-provided message */
  p += vsnprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), fmt, ev->ap);
  check_overflow;

  /* End it */
  p += snprintf(p, XBT_LOG_BUFF_SIZE - (p - ev->buffer), "\n");
  check_overflow;
  app->do_append(app, ev->buffer);
}

xbt_log_layout_t xbt_log_layout_simple_new(char *arg)
{
  xbt_log_layout_t res = xbt_new0(s_xbt_log_layout_t, 1);
  res->do_layout = xbt_log_layout_simple_doit;

  if (simple_begin_of_time < 0)
    simple_begin_of_time = gras_os_time();

  return res;
}

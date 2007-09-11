/* $Id$ */

/* layout_simple - a dumb log layout                                        */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/synchro.h" /* xbt_thread_name */
#include "gras/virtu.h"
#include <stdio.h>

extern const char *xbt_log_priority_names[7];

static void xbt_log_layout_simple_doit(xbt_log_layout_t l,
				       xbt_log_event_t ev, 
				       const char *fmt) {
  static double begin_of_time = -1;
  char *p;  

  xbt_assert0(ev->priority>=0,
	      "Negative logging priority naturally forbidden");
  xbt_assert1(ev->priority<sizeof(xbt_log_priority_names),
	      "Priority %d is greater than the biggest allowed value",
	      ev->priority);

  if (begin_of_time<0) 
    begin_of_time=gras_os_time();

  p = ev->buffer;
  p += sprintf(p,"[");;
  /* Display the proc info if available */
  if(strlen(xbt_procname()))
    p += sprintf(p,"%s:%s:(%d) ", 
		 gras_os_myname(), xbt_procname(),(*xbt_getpid)());

  /* Display the date */
  p += sprintf(p,"%f] ", gras_os_time()-begin_of_time);


  /* Display file position if not INFO*/
  if (ev->priority != xbt_log_priority_info)
    p += sprintf(p, "%s:%d: ", ev->fileName, ev->lineNum);

  /* Display category name */
  p += sprintf(p, "[%s/%s] ", 
	       ev->cat->name, xbt_log_priority_names[ev->priority] );

  /* Display user-provided message */
  p += vsprintf(p, fmt, ev->ap);

  /* End it */
  p += sprintf(p, "\n");

}

xbt_log_layout_t xbt_log_layout_simple_new(char *arg) {
  xbt_log_layout_t res = xbt_new0(s_xbt_log_layout_t,1);
  res->do_layout = xbt_log_layout_simple_doit;
  return res;
}

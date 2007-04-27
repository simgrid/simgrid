/* $Id$ */

/* file_appender - a dumb log appender which simply prints to stdout        */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include <stdio.h>
#include "gras/virtu.h"

/**
 * The root category's default logging function.
 */

extern const char *xbt_log_priority_names[7];

static void append_file(xbt_log_appender_t this_appender, xbt_log_event_t ev,
			const char *fmt);

/*
struct xbt_log_appender_file_s {
  xbt_log_appender_t* appender;
  FILE *file;
};
*/

static s_xbt_log_appender_t xbt_log_appender_file = { append_file, NULL } ;
/* appender_data=FILE* */

xbt_log_appender_t xbt_log_default_appender  = &xbt_log_appender_file;

static const char* xbt_logappender_verbose_information(void) {
  static char buffer[256];
  static double begin_of_time = -1;
  
  if (begin_of_time<0) 
    begin_of_time=gras_os_time();

  if(strlen(xbt_procname()))
    sprintf(buffer,"%s:%s:(%ld) %f", gras_os_myname(),
	    xbt_procname(),gras_os_getpid(),gras_os_time()-begin_of_time);
  else 
    sprintf(buffer,"%f", gras_os_time()-begin_of_time);
  
  return buffer;
}

static void append_file(xbt_log_appender_t this_appender,
			xbt_log_event_t ev, 
			const char *fmt) {

  /* TODO: define a format field in struct for timestamp, etc.
     struct DefaultLogAppender* this = (struct DefaultLogAppender*)this0;*/

  char *procname = (char*)xbt_logappender_verbose_information();
  if (!procname) 
     procname = (char*)"";
   
    if ((FILE*)(this_appender->appender_data) == NULL)
      this_appender->appender_data = (void*)stderr;
    
    xbt_assert0(ev->priority>=0,
		 "Negative logging priority naturally forbidden");
    xbt_assert1(ev->priority<sizeof(xbt_log_priority_names),
		 "Priority %d is greater than the biggest allowed value",
		 ev->priority);

    if (ev->priority != xbt_log_priority_info)
     fprintf(stderr, "[%s] %s:%d: ", procname, ev->fileName, ev->lineNum);
    else
     fprintf(stderr, "[%s] ", procname);
    fprintf(stderr, "[%s/%s] ", 
	    ev->cat->name, xbt_log_priority_names[ev->priority] );
    vfprintf(stderr, fmt, ev->ap);
    fprintf(stderr, "\n");
}

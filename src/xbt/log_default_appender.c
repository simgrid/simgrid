/* $Id$ */

/* file_appender - a dumb log appender which simply prints to stdout        */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003, 2004 Martin Quinson.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_private.h"
#include <stdio.h>

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(log_app,log);


/**
 * The root category's default logging function.
 */

extern const char *gras_log_priority_names[7];

static void append_file(gras_log_appender_t* this, gras_log_event_t* ev,
			const char *fmt);

/*
struct gras_log_appender_file_s {
  gras_log_appender_t* appender;
  FILE *file;
};
*/

static gras_log_appender_t gras_log_appender_file = { append_file, NULL } ;
/* appender_data=FILE* */

gras_log_appender_t* gras_log_default_appender  = &gras_log_appender_file;

static void append_file(gras_log_appender_t* this, 
			gras_log_event_t* ev, 
			const char *fmt) {

    // TODO: define a format field in struct for timestamp, etc.
    //    struct DefaultLogAppender* this = (struct DefaultLogAppender*)this0;
    
    if ((FILE*)(this->appender_data) == NULL)
      this->appender_data = (void*)stderr;
    
    gras_assert0(ev->priority>=0,
		 "Negative logging priority naturally forbidden");
    gras_assert1(ev->priority<sizeof(gras_log_priority_names),
		 "Priority %d is greater than the biggest allowed value",
		 ev->priority);

    fprintf(stderr, "%s:%d: ", ev->fileName, ev->lineNum);
    fprintf(stderr, "[%s/%s] ", 
	    ev->cat->name, gras_log_priority_names[ev->priority]);
    vfprintf(stderr, fmt, ev->ap);
    fprintf(stderr, "\n");
}

/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_SURF_PARSE_H
#define _SURF_SURF_PARSE_H

#include "xbt/misc.h"
#include "surf/trace_mgr.h"

typedef enum {
  TOKEN_EMPTY = 0,
  TOKEN_LP = 512,
  TOKEN_RP,
  TOKEN_BEGIN_SECTION,
  TOKEN_END_SECTION,
  TOKEN_CLOSURE,
  TOKEN_WORD,
  TOKEN_NEWLINE,
  TOKEN_ERROR
} e_surf_token_t;

#define MAX_STR_CONST 1024

extern char *surf_parse_text;
extern int line_pos;
extern int char_pos;
extern int tok_num;

e_surf_token_t surf_parse(void);
void find_section(const char* file, const char* section_name);
void close_section(const char* section_name);
void surf_parse_float(xbt_maxmin_float_t *value);
void surf_parse_trace(tmgr_trace_t *trace);


/* Should not be called if you use the previous "section" functions */
void  surf_parse_open(const char *file);
void  surf_parse_close(void);

#endif

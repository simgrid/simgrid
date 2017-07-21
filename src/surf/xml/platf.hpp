/* Copyright (c) 2006-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_SURFXML_PARSE_H
#define SURF_SURFXML_PARSE_H

#include <xbt/dict.h>
#include <xbt/function_types.h>
#include <xbt/misc.h>
#include <xbt/signal.hpp>

SG_BEGIN_DECL()

/* Module management functions */
XBT_PUBLIC(void) sg_platf_init();;
XBT_PUBLIC(void) sg_platf_exit();

XBT_PUBLIC(void) surf_parse_open(const char *file);
XBT_PUBLIC(void) surf_parse_close();
XBT_PUBLIC(void) surf_parse_assert(bool cond, const char *fmt, ...) XBT_ATTRIB_PRINTF(2,3);
XBT_PUBLIC(void) XBT_ATTRIB_NORETURN surf_parse_error(const char *msg,...) XBT_ATTRIB_PRINTF(1,2);
XBT_PUBLIC(void) surf_parse_assert_netpoint(char* hostname, const char* pre, const char* post);
XBT_PUBLIC(void) surf_parse_warn(const char *msg,...) XBT_ATTRIB_PRINTF(1,2);

XBT_PUBLIC(double) surf_parse_get_double(const char *string);
XBT_PUBLIC(int) surf_parse_get_int(const char *string);
XBT_PUBLIC(double) surf_parse_get_time(const char *string, const char *entity_kind, const char *name);
XBT_PUBLIC(double) surf_parse_get_size(const char *string, const char *entity_kind, const char *name);
XBT_PUBLIC(double) surf_parse_get_bandwidth(const char *string, const char *entity_kind, const char *name);
XBT_PUBLIC(double) surf_parse_get_speed(const char *string, const char *entity_kind, const char *name);

XBT_PUBLIC(int) surf_parse(); /* Entry-point to the parser */

SG_END_DECL()

#endif

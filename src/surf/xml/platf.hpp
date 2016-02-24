/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_SURFXML_PARSE_H
#define _SURF_SURFXML_PARSE_H

#include "xbt/misc.h"
#include "xbt/function_types.h"
#include "xbt/dict.h"

SG_BEGIN_DECL()

/* Module management functions */
XBT_PUBLIC(void) sg_platf_init(void);
XBT_PUBLIC(void) sg_platf_exit(void);

XBT_PUBLIC(void) surf_parse_open(const char *file);
XBT_PUBLIC(void) surf_parse_close(void);
XBT_PUBLIC(void) surf_parse_assert(bool cond, const char *fmt, ...) XBT_ATTRIB_PRINTF(2,3);
XBT_PUBLIC(void) XBT_ATTRIB_NORETURN surf_parse_error(const char *msg,...) XBT_ATTRIB_PRINTF(1,2);
XBT_PUBLIC(void) surf_parse_warn(const char *msg,...) XBT_ATTRIB_PRINTF(1,2);

XBT_PUBLIC(double) surf_parse_get_double(const char *string);
XBT_PUBLIC(int) surf_parse_get_int(const char *string);
XBT_PUBLIC(double) surf_parse_get_time(const char *string, const char *entity_kind, const char *name);
XBT_PUBLIC(double) surf_parse_get_size(const char *string, const char *entity_kind, const char *name);
XBT_PUBLIC(double) surf_parse_get_bandwidth(const char *string, const char *entity_kind, const char *name);
XBT_PUBLIC(double) surf_parse_get_speed(const char *string, const char *entity_kind, const char *name);

XBT_PUBLIC_DATA(int_f_void_t) surf_parse;       /* Entry-point to the parser. Set this to your function. */


SG_END_DECL()

namespace simgrid {
namespace surf {

extern XBT_PRIVATE xbt::signal<void(void)> on_postparse;

}
}

#endif

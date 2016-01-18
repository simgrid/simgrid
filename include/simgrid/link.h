/* Public interface to the Link datatype                                    */

/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_LINK_H_
#define INCLUDE_SIMGRID_LINK_H_

#include <simgrid/forward.h>

/* C interface */
SG_BEGIN_DECL()
XBT_PUBLIC(const char*) sg_link_name(Link *link);
XBT_PUBLIC(Link *) sg_link_by_name(const char *name);
XBT_PUBLIC(int) sg_link_is_shared(Link *link);
XBT_PUBLIC(double) sg_link_bandwidth(Link *link);
XBT_PUBLIC(double) sg_link_latency(Link *link);
XBT_PUBLIC(void*) sg_link_data(Link *link);
XBT_PUBLIC(void) sg_link_data_set(Link *link,void *data);
XBT_PUBLIC(int) sg_link_count(void);
XBT_PUBLIC(Link**) sg_link_list(void);
XBT_PUBLIC(void) sg_link_exit(void);
SG_END_DECL()

#endif /* INCLUDE_SIMGRID_LINK_H_ */

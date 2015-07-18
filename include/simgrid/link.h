/* Public interface to the Link datatype                                    */

/* Copyright (c) 2015. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_LINK_H_
#define INCLUDE_SIMGRID_LINK_H_

#ifdef __cplusplus
class Link;
#else
typedef struct Link Link;
#endif

/* C interface */
SG_BEGIN_DECL()
XBT_PUBLIC(int) surf_network_link_is_shared(Link *link);
XBT_PUBLIC(double) surf_network_link_get_bandwidth(Link *link);
XBT_PUBLIC(double) surf_network_link_get_latency(Link *link);
XBT_PUBLIC(const char*) surf_network_link_get_name(Link *link);
XBT_PUBLIC(void*) surf_network_link_data(Link *link);
XBT_PUBLIC(void) surf_network_link_data_set(Link *link,void *data);
SG_END_DECL()

#endif /* INCLUDE_SIMGRID_LINK_H_ */

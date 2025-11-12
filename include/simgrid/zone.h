/* Public interface to the Link datatype                                    */

/* Copyright (c) 2018-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_ZONE_H_
#define INCLUDE_SIMGRID_ZONE_H_

#include <simgrid/forward.h>
#include <xbt/base.h>
#include <xbt/dict.h>

/* C interface */
SG_BEGIN_DECL

XBT_PUBLIC sg_netzone_t sg_zone_get_root();
XBT_PUBLIC const char* sg_zone_get_name(const_sg_netzone_t zone);
XBT_PUBLIC sg_netzone_t sg_zone_get_by_name(const char* name);
#ifndef DOXYGEN
XBT_ATTRIB_DEPRECATED_v403("Please use sg_zone_get_childs instead: we want to kill xbt_dict at some point") XBT_PUBLIC
    void sg_zone_get_sons(const_sg_netzone_t zone, xbt_dict_t whereto);
XBT_ATTRIB_DEPRECATED_v403("Please use sg_zone_get_all_hosts instead: we want to kill xbt_dynar at some point")
    XBT_PUBLIC void sg_zone_get_hosts(const_sg_netzone_t zone, xbt_dynar_t whereto);
#endif
/** Returns a NULL-terminated list childs.
 * if @c size is not null, the child count is also stored in it
 */
XBT_PUBLIC sg_netzone_t* sg_zone_get_childs(const_sg_netzone_t netzone, int* size);
XBT_PUBLIC const char** sg_zone_get_property_names(const_sg_netzone_t zone);
XBT_PUBLIC const char* sg_zone_get_property_value(const_sg_netzone_t as, const char* name);
XBT_PUBLIC void sg_zone_set_property_value(sg_netzone_t netzone, const char* name, const char* value);
XBT_PUBLIC const_sg_host_t* sg_zone_get_all_hosts(const_sg_netzone_t zone, int* size);

SG_END_DECL

#endif /* INCLUDE_SIMGRID_ZONE_H_ */

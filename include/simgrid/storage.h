/* Public interface to the Link datatype                                    */

/* Copyright (c) 2018-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_STORAGE_H_
#define INCLUDE_SIMGRID_STORAGE_H_

#include <simgrid/forward.h>
#include <xbt/base.h>

/* C interface */
SG_BEGIN_DECL

XBT_PUBLIC const char* sg_storage_get_name(const_sg_storage_t storage);
XBT_PUBLIC sg_storage_t sg_storage_get_by_name(const char* name);
XBT_PUBLIC xbt_dict_t sg_storage_get_properties(const_sg_storage_t storage);
XBT_PUBLIC void sg_storage_set_property_value(sg_storage_t storage, const char* name, const char* value);
XBT_PUBLIC const char* sg_storage_get_property_value(const_sg_storage_t storage, const char* name);
XBT_PUBLIC xbt_dynar_t sg_storages_as_dynar();
XBT_PUBLIC void sg_storage_set_data(sg_storage_t host, void* data);
XBT_PUBLIC void* sg_storage_get_data(const_sg_storage_t storage);
XBT_PUBLIC const char* sg_storage_get_host(const_sg_storage_t storage);
XBT_PUBLIC sg_size_t sg_storage_read(sg_storage_t storage, sg_size_t size);
XBT_PUBLIC sg_size_t sg_storage_write(sg_storage_t storage, sg_size_t size);

SG_END_DECL

#endif /* INCLUDE_SIMGRID_STORAGE_H_ */

/* Copyright (c) 2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_DISK_H_
#define INCLUDE_SIMGRID_DISK_H_

#include <simgrid/forward.h>
#include <xbt/base.h>

/* C interface */
SG_BEGIN_DECL
XBT_PUBLIC const char* sg_disk_get_name(const_sg_disk_t disk);
XBT_ATTRIB_DEPRECATED_v330("Please use sg_disk_get_name()") XBT_PUBLIC const char* sg_disk_name(const_sg_disk_t disk);
XBT_PUBLIC sg_host_t sg_disk_get_host(const_sg_disk_t disk);
XBT_PUBLIC double sg_disk_read_bandwidth(const_sg_disk_t disk);
XBT_PUBLIC double sg_disk_write_bandwidth(const_sg_disk_t disk);
XBT_PUBLIC sg_size_t sg_disk_write(sg_disk_t disk, sg_size_t size);
XBT_PUBLIC sg_size_t sg_disk_read(sg_disk_t disk, sg_size_t size);
XBT_PUBLIC void* sg_disk_get_data(const_sg_disk_t disk);
XBT_PUBLIC void sg_disk_set_data(sg_disk_t disk, void* data);
XBT_ATTRIB_DEPRECATED_v330("Please use sg_disk_get_data() instead") XBT_PUBLIC void* sg_disk_data(const_sg_disk_t disk);
XBT_ATTRIB_DEPRECATED_v330("Please use sg_disk_set_data() instead") XBT_PUBLIC
    void sg_disk_data_set(sg_disk_t disk, void* data);
SG_END_DECL

#endif /* INCLUDE_SIMGRID_DISK_H_ */

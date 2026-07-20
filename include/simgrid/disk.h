/* Copyright (c) 2020-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_DISK_H_
#define INCLUDE_SIMGRID_DISK_H_

#include <simgrid/forward.h>

/* C interface */
SG_BEGIN_DECL
/** @brief Retrieves the name of that disk */
XBT_PUBLIC const char* sg_disk_get_name(const_sg_disk_t disk);
/** @brief Retrieve the host to which this disk is attached */
XBT_PUBLIC sg_host_t sg_disk_get_host(const_sg_disk_t disk);
/** @brief The read bandwidth of this disk is the max speed at which read operations progress */
XBT_PUBLIC double sg_disk_read_bandwidth(const_sg_disk_t disk);
/** @brief The write bandwidth of this disk is the max speed at which write operations progress */
XBT_PUBLIC double sg_disk_write_bandwidth(const_sg_disk_t disk);
/** @brief Blocking write of the given amount of bytes to this disk. Returns the amount of bytes actually written. */
XBT_PUBLIC sg_size_t sg_disk_write(const_sg_disk_t disk, sg_size_t size);
/** @brief Blocking read of the given amount of bytes from this disk. Returns the amount of bytes actually read. */
XBT_PUBLIC sg_size_t sg_disk_read(const_sg_disk_t disk, sg_size_t size);
/** @brief Retrieve the user data associated to this disk (or nullptr if not set) */
XBT_PUBLIC void* sg_disk_get_data(const_sg_disk_t disk);
/** @brief Set the user data associated to this disk */
XBT_PUBLIC void sg_disk_set_data(sg_disk_t disk, void* data);
SG_END_DECL

#endif /* INCLUDE_SIMGRID_DISK_H_ */

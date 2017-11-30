/* Copyright (c) 2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PLUGINS_FILE_SYSTEM_H_
#define SIMGRID_PLUGINS_FILE_SYSTEM_H_

#include <simgrid/forward.h>
#include <xbt/base.h>

SG_BEGIN_DECL()

XBT_PUBLIC(void) sg_storage_file_system_init();
XBT_PUBLIC(sg_file_t) sg_file_open(const char* fullpath, void* data);
XBT_PUBLIC(sg_size_t) sg_file_read(sg_file_t fd, sg_size_t size);
XBT_PUBLIC(sg_size_t) sg_file_write(sg_file_t fd, sg_size_t size);
XBT_PUBLIC(void) sg_file_close(sg_file_t fd);

XBT_PUBLIC(const char*) sg_file_get_name(sg_file_t fd);
XBT_PUBLIC(sg_size_t) sg_file_get_size(sg_file_t fd);
XBT_PUBLIC(void) sg_file_dump(sg_file_t fd);
XBT_PUBLIC(void*) sg_file_get_data(sg_file_t fd);
XBT_PUBLIC(void) sg_file_set_data(sg_file_t fd, void* data);
XBT_PUBLIC(void) sg_file_seek(sg_file_t fd, sg_offset_t offset, int origin);
XBT_PUBLIC(sg_size_t) sg_file_tell(sg_file_t fd);
XBT_PUBLIC(void) sg_file_move(sg_file_t fd, const char* fullpath);
XBT_PUBLIC(void) sg_file_unlink(sg_file_t fd);
XBT_PUBLIC(int) sg_file_rcopy(sg_file_t file, sg_host_t host, const char* fullpath);
XBT_PUBLIC(int) sg_file_rmove(sg_file_t file, sg_host_t host, const char* fullpath);

XBT_PUBLIC(void*) sg_storage_get_data(sg_storage_t storage);
XBT_PUBLIC(void) sg_storage_set_data(sg_storage_t storage, void* data);
XBT_PUBLIC(sg_size_t) sg_storage_get_size_free(sg_storage_t st);
XBT_PUBLIC(sg_size_t) sg_storage_get_size_used(sg_storage_t st);
XBT_PUBLIC(sg_size_t) sg_storage_get_size(sg_storage_t st);
XBT_PUBLIC(xbt_dict_t) sg_storage_get_content(sg_storage_t storage);

XBT_PUBLIC(xbt_dict_t) sg_host_get_storage_content(sg_host_t host);

#define MSG_file_open(fullpath, data) sg_file_open(fullpath, data)
#define MSG_file_read(fd, size) sg_file_read(fd, size)
#define MSG_file_write(fd, size) sg_file_write(fd, size)
#define MSG_file_close(fd) sg_file_close(fd)
#define MSG_file_get_name(fd) sg_file_get_name(fd)
#define MSG_file_get_size(fd) sg_file_get_size(fd)
#define MSG_file_dump(fd) sg_file_dump(fd)
#define MSG_file_get_data(fd) sg_file_get_data(fd)
#define MSG_file_set_data(fd, data) sg_file_set_data(fd, data)
#define MSG_file_seek(fd, offset, origin) sg_file_seek(fd, offset, origin)
#define MSG_file_tell(fd) sg_file_tell(fd)
#define MSG_file_move(fd, fullpath) sg_file_get_size(fd, fullpath)
#define MSG_file_unlink(fd) sg_file_unlink(fd)
#define MSG_file_rcopy(file, host, fullpath) sg_file_rcopy(file, host, fullpath)
#define MSG_file_rmove(file, host, fullpath) sg_file_rmove(file, host, fullpath)

#define MSG_storage_file_system_init() sg_storage_file_system_init()
#define MSG_storage_get_free_size(st) sg_storage_get_size_free(st)
#define MSG_storage_get_used_size(st) sg_storage_get_size_used(st)
#define MSG_storage_get_size(st) sg_storage_get_size(st)
#define MSG_storage_get_content(st) sg_storage_get_content(st)

#define MSG_host_get_storage_content(st) sg_host_get_storage_content(st)

SG_END_DECL()

#endif

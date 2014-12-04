/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_PROCESS_H
#define MC_PROCESS_H

#include "simgrid_config.h"

#include <sys/types.h>

#include "mc_forward.h"
#include "mc_memory_map.h"

SG_BEGIN_DECL()

typedef enum {
  MC_PROCESS_NO_FLAG = 0,
  MC_PROCESS_SELF_FLAG = 1,
} e_mc_process_flags_t;

/** Representation of a process
 */
struct s_mc_process {
  e_mc_process_flags_t process_flags;
  pid_t pid;
  memory_map_t memory_map;
  void *maestro_stack_start, *maestro_stack_end;
  mc_object_info_t libsimgrid_info;
  mc_object_info_t binary_info;
  mc_object_info_t* object_infos;
  size_t object_infos_size;
};

void MC_process_init(mc_process_t process, pid_t pid);
void MC_process_clear(mc_process_t process);

mc_object_info_t MC_process_find_object_info(mc_process_t process, void* ip);
dw_frame_t MC_process_find_function(mc_process_t process, void* ip);

SG_END_DECL()

#endif

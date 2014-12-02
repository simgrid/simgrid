/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_MEMORY_MAP_H
#define MC_MEMORY_MAP_H

#include <simgrid_config.h>

SG_BEGIN_DECL()

/** \file
 *   These functions and data structures implements a binary interface for
 *   the proc maps ascii interface                                           */

/* Each field is defined as documented in proc's manual page  */
struct s_map_region {

  void *start_addr;             /* Start address of the map */
  void *end_addr;               /* End address of the map */
  int prot;                     /* Memory protection */
  int flags;                    /* Additional memory flags */
  void *offset;                 /* Offset in the file/whatever */
  char dev_major;               /* Major of the device */
  char dev_minor;               /* Minor of the device */
  unsigned long inode;          /* Inode in the device */
  char *pathname;               /* Path name of the mapped file */

};
typedef struct s_map_region s_map_region_t, *map_region_t;

struct s_memory_map {

  s_map_region_t *regions;      /* Pointer to an array of regions */
  int mapsize;                  /* Number of regions in the memory */

};

void MC_init_memory_map_info(void);
memory_map_t MC_get_memory_map(void);
void MC_free_memory_map(memory_map_t map);

SG_END_DECL()

#endif

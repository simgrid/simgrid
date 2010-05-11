/* 	$Id: private.h 5497 2008-05-26 12:19:15Z cristianrosa $	 */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donnassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_PRIVATE_H
#define MC_PRIVATE_H

#include <stdio.h>
#include "mc/mc.h"
#include "mc/datatypes.h"
#include "xbt/fifo.h"
#include "xbt/setset.h"
#include "xbt/config.h"
#include "xbt/function_types.h"
#include "xbt/mmalloc.h"
#include "../simix/private.h"

/****************************** Snapshots ***********************************/

/*typedef struct s_mc_process_checkpoint {
	xbt_checkpoint_t machine_state;  Here we save the cpu + stack 
} s_mc_process_checkpoint_t, *mc_process_checkpoint_t;*/

typedef struct s_mc_snapshot {
  size_t heap_size;
  size_t data_size;
	void* data;
	void* heap;

} s_mc_snapshot_t, *mc_snapshot_t;

extern mc_snapshot_t initial_snapshot;

size_t MC_save_heap(void **);
void MC_restore_heap(void *, size_t);

size_t MC_save_dataseg(void **);
void MC_restore_dataseg(void *, size_t);

void MC_take_snapshot(mc_snapshot_t);
void MC_restore_snapshot(mc_snapshot_t);
void MC_free_snapshot(mc_snapshot_t);

/********************************* MC Global **********************************/

/* Bound of the MC depth-first search algorithm */
#define MAX_DEPTH 1000

extern char mc_replay_mode;

void MC_show_stack(xbt_fifo_t stack);
void MC_dump_stack(xbt_fifo_t stack);
void MC_execute_surf_actions(void);
void MC_replay(xbt_fifo_t stack);
void MC_schedule_enabled_processes(void);
void MC_dfs_init(void);
void MC_dpor_init(void);
void MC_dfs(void);
void MC_dpor(void);

/******************************* Transitions **********************************/
typedef struct s_mc_transition{
  XBT_SETSET_HEADERS;
  char* name;
  unsigned int refcount;
  mc_trans_type_t type;
  smx_process_t process;
  smx_rdv_t rdv;
  smx_comm_t comm;          /* reference to the simix network communication */
  
  /* Used only for random transitions */
  int min;                  /* min random value */ 
  int max;                  /* max random value */
  int current_value;        /* current value */
} s_mc_transition_t;

void MC_random_create(int,int);
void MC_transition_delete(mc_transition_t);
int MC_transition_depend(mc_transition_t, mc_transition_t);

/******************************** States **************************************/
typedef struct mc_state{
  xbt_setset_set_t transitions;
  xbt_setset_set_t enabled_transitions;
  xbt_setset_set_t interleave;
  xbt_setset_set_t done;
  mc_transition_t executed_transition;
} s_mc_state_t, *mc_state_t;

extern xbt_fifo_t mc_stack;
extern xbt_setset_t mc_setset;
extern mc_state_t mc_current_state;

mc_state_t MC_state_new(void);
void MC_state_delete(mc_state_t);

/****************************** Statistics ************************************/
typedef struct mc_stats{
  unsigned long state_size;
  unsigned long visited_states;
  unsigned long expanded_states;
  unsigned long executed_transitions;
}s_mc_stats_t, *mc_stats_t;

extern mc_stats_t mc_stats;

void MC_print_statistics(mc_stats_t);

/********************************** MEMORY ******************************/
/* The possible memory modes for the modelchecker are standard and raw. */
/* Normally the system should operate in std, for switching to raw mode */
/* you must wrap the code between MC_SET_RAW_MODE and MC_UNSET_RAW_MODE */

extern void *std_heap;
extern void *raw_heap;
extern void *libsimgrid_data_addr_start;
extern size_t libsimgrid_data_size;

#define MC_SET_RAW_MEM    mmalloc_set_current_heap(raw_heap)
#define MC_UNSET_RAW_MEM    mmalloc_set_current_heap(std_heap)

/******************************* MEMORY MAPPINGS ***************************/
/* These functions and data structures implements a binary interface for   */
/* the proc maps ascii interface                                           */

#define MAP_READ   1 << 0
#define MAP_WRITE  1 << 1
#define MAP_EXEC   1 << 2
#define MAP_SHARED 1 << 3
#define MAP_PRIV   1 << 4 

/* Each field is defined as documented in proc's manual page  */
typedef struct s_map_region {

  void *start_addr;     /* Start address of the map */
  void *end_addr;       /* End address of the map */
  int perms;            /* Set of permissions */  
  void *offset;         /* Offset in the file/whatever */
  char dev_major;       /* Major of the device */  
  char dev_minor;       /* Minor of the device */
  unsigned long inode;  /* Inode in the device */
  char *pathname;       /* Path name of the mapped file */

} s_map_region;

typedef struct s_memory_map {

  s_map_region *regions;  /* Pointer to an array of regions */
  int mapsize;            /* Number of regions in the memory */

} s_memory_map_t, *memory_map_t;

memory_map_t get_memory_map(void);


#endif

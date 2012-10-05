/* Copyright (c) 2007-2012 Da SimGrid Team. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_PRIVATE_H
#define MC_PRIVATE_H

#include "simgrid_config.h"
#include <stdio.h>
#include <sys/mman.h>
#include "mc/mc.h"
#include "mc/datatypes.h"
#include "xbt/fifo.h"
#include "xbt/config.h"
#include "xbt/function_types.h"
#include "xbt/mmalloc.h"
#include "../simix/smx_private.h"
#include "xbt/automaton.h"
#include "xbt/hash.h"
#include "msg/msg.h"
#include "msg/datatypes.h"
#include "xbt/strbuff.h"

/****************************** Snapshots ***********************************/

typedef struct s_mc_mem_region{
  int type;
  void *start_addr;
  void *data;
  size_t size;
} s_mc_mem_region_t, *mc_mem_region_t;

typedef struct s_mc_snapshot{
  unsigned int num_reg;
  mc_mem_region_t *regions;
  xbt_dynar_t stacks;
} s_mc_snapshot_t, *mc_snapshot_t;

typedef struct s_mc_snapshot_stack{
  xbt_strbuff_t local_variables;
  void *stack_pointer;
}s_mc_snapshot_stack_t, *mc_snapshot_stack_t;

void MC_take_snapshot(mc_snapshot_t);
void MC_take_snapshot_liveness(mc_snapshot_t s);
void MC_restore_snapshot(mc_snapshot_t);
void MC_free_snapshot(mc_snapshot_t);


/********************************* MC Global **********************************/
extern double *mc_time;

/* Bound of the MC depth-first search algorithm */
#define MAX_DEPTH 1000
#define MAX_DEPTH_LIVENESS 500

int MC_deadlock_check(void);
void MC_replay(xbt_fifo_t stack, int start);
void MC_replay_liveness(xbt_fifo_t stack, int all_stack);
void MC_wait_for_requests(void);
void MC_get_enabled_processes();
void MC_show_deadlock(smx_simcall_t req);
void MC_show_stack_safety(xbt_fifo_t stack);
void MC_dump_stack_safety(xbt_fifo_t stack);

/********************************* Requests ***********************************/
int MC_request_depend(smx_simcall_t req1, smx_simcall_t req2);
char* MC_request_to_string(smx_simcall_t req, int value);
unsigned int MC_request_testany_fail(smx_simcall_t req);
/*int MC_waitany_is_enabled_by_comm(smx_req_t req, unsigned int comm);*/
int MC_request_is_visible(smx_simcall_t req);
int MC_request_is_enabled(smx_simcall_t req);
int MC_request_is_enabled_by_idx(smx_simcall_t req, unsigned int idx);
int MC_process_is_enabled(smx_process_t process);


/******************************** States **************************************/
/* Possible exploration status of a process in a state */
typedef enum {
  MC_NOT_INTERLEAVE=0,      /* Do not interleave (do not execute) */
  MC_INTERLEAVE,            /* Interleave the process (one or more request) */
  MC_DONE                   /* Already interleaved */
} e_mc_process_state_t;

/* On every state, each process has an entry of the following type */
typedef struct mc_procstate{
  e_mc_process_state_t state;       /* Exploration control information */
  unsigned int interleave_count;    /* Number of times that the process was
                                       interleaved */
} s_mc_procstate_t, *mc_procstate_t;

/* An exploration state is composed of: */
typedef struct mc_state {
  unsigned long max_pid;            /* Maximum pid at state's creation time */
  mc_procstate_t proc_status;       /* State's exploration status by process */
  s_smx_action_t internal_comm;     /* To be referenced by the internal_req */
  s_smx_simcall_t internal_req;         /* Internal translation of request */
  s_smx_simcall_t executed_req;         /* The executed request of the state */
  int req_num;                      /* The request number (in the case of a
                                       multi-request like waitany ) */
  mc_snapshot_t system_state;      /* Snapshot of system state */
} s_mc_state_t, *mc_state_t;

extern xbt_fifo_t mc_stack_safety_stateless;

mc_state_t MC_state_new(void);
void MC_state_delete(mc_state_t state);
void MC_state_interleave_process(mc_state_t state, smx_process_t process);
unsigned int MC_state_interleave_size(mc_state_t state);
int MC_state_process_is_done(mc_state_t state, smx_process_t process);
void MC_state_set_executed_request(mc_state_t state, smx_simcall_t req, int value);
smx_simcall_t MC_state_get_executed_request(mc_state_t state, int *value);
smx_simcall_t MC_state_get_internal_request(mc_state_t state);
smx_simcall_t MC_state_get_request(mc_state_t state, int *value);

/****************************** Statistics ************************************/
typedef struct mc_stats {
  unsigned long state_size;
  unsigned long visited_states;
  unsigned long expanded_states;
  unsigned long executed_transitions;
} s_mc_stats_t, *mc_stats_t;

typedef struct mc_stats_pair {
  //unsigned long pair_size;
  unsigned long visited_pairs;
  unsigned long expanded_pairs;
  unsigned long executed_transitions;
} s_mc_stats_pair_t, *mc_stats_pair_t;

extern mc_stats_t mc_stats;
extern mc_stats_pair_t mc_stats_pair;

void MC_print_statistics(mc_stats_t);
void MC_print_statistics_pairs(mc_stats_pair_t);

/********************************** MEMORY ******************************/
/* The possible memory modes for the modelchecker are standard and raw. */
/* Normally the system should operate in std, for switching to raw mode */
/* you must wrap the code between MC_SET_RAW_MODE and MC_UNSET_RAW_MODE */

extern void *std_heap;
extern void *raw_heap;


/* FIXME: Horrible hack! because the mmalloc library doesn't provide yet of */
/* an API to query about the status of a heap, we simply call mmstats and */
/* because I now how does structure looks like, then I redefine it here */

/* struct mstats { */
/*   size_t bytes_total;           /\* Total size of the heap. *\/ */
/*   size_t chunks_used;           /\* Chunks allocated by the user. *\/ */
/*   size_t bytes_used;            /\* Byte total of user-allocated chunks. *\/ */
/*   size_t chunks_free;           /\* Chunks in the free list. *\/ */
/*   size_t bytes_free;            /\* Byte total of chunks in the free list. *\/ */
/* }; */

#define MC_SET_RAW_MEM    mmalloc_set_current_heap(raw_heap)
#define MC_UNSET_RAW_MEM  mmalloc_set_current_heap(std_heap)

extern int raw_mem_set;

/******************************* MEMORY MAPPINGS ***************************/
/* These functions and data structures implements a binary interface for   */
/* the proc maps ascii interface                                           */

/* Each field is defined as documented in proc's manual page  */
typedef struct s_map_region {

  void *start_addr;             /* Start address of the map */
  void *end_addr;               /* End address of the map */
  int prot;                     /* Memory protection */
  int flags;                    /* Aditional memory flags */
  void *offset;                 /* Offset in the file/whatever */
  char dev_major;               /* Major of the device */
  char dev_minor;               /* Minor of the device */
  unsigned long inode;          /* Inode in the device */
  char *pathname;               /* Path name of the mapped file */

} s_map_region_t;

typedef struct s_memory_map {

  s_map_region_t *regions;      /* Pointer to an array of regions */
  int mapsize;                  /* Number of regions in the memory */

} s_memory_map_t, *memory_map_t;

memory_map_t get_memory_map(void);
void free_memory_map(memory_map_t map);
void get_plt_section(void);

extern void *start_data_libsimgrid;

/********************************** DPOR for safety  **************************************/
typedef enum {
  e_mc_reduce_unset,
  e_mc_reduce_none,
  e_mc_reduce_dpor
} e_mc_reduce_t;

extern e_mc_reduce_t mc_reduce_kind;

void MC_dpor_init(void);
void MC_dpor(void);
void MC_dpor_exit(void);
void MC_init_safety(void);


/********************************** Double-DFS for liveness property**************************************/

extern xbt_fifo_t mc_stack_liveness;
extern mc_snapshot_t initial_snapshot_liveness;
extern xbt_automaton_t _mc_property_automaton;
extern int compare;
extern void *start_plt;
extern void *end_plt;

typedef struct s_mc_pair{
  mc_snapshot_t system_state;
  mc_state_t graph_state;
  xbt_state_t automaton_state;
}s_mc_pair_t, *mc_pair_t;

typedef struct s_mc_pair_reached{
  int nb;
  xbt_state_t automaton_state;
  xbt_dynar_t prop_ato;
  mc_snapshot_t system_state;
}s_mc_pair_reached_t, *mc_pair_reached_t;

int MC_automaton_evaluate_label(xbt_exp_label_t l);
mc_pair_t new_pair(mc_snapshot_t sn, mc_state_t sg, xbt_state_t st);

int reached(xbt_state_t st);
void set_pair_reached(xbt_state_t st);
int snapshot_compare(mc_snapshot_t s1, mc_snapshot_t s2);
void MC_pair_delete(mc_pair_t pair);
void MC_exit_liveness(void);
mc_state_t MC_state_pair_new(void);

/* **** Double-DFS stateless **** */

typedef struct s_mc_pair_stateless{
  mc_state_t graph_state;
  xbt_state_t automaton_state;
  int requests;
}s_mc_pair_stateless_t, *mc_pair_stateless_t;

mc_pair_stateless_t new_pair_stateless(mc_state_t sg, xbt_state_t st, int r);
void MC_ddfs_init(void);
void MC_ddfs(int search_cycle);
void MC_show_stack_liveness(xbt_fifo_t stack);
void MC_dump_stack_liveness(xbt_fifo_t stack);
void MC_pair_stateless_delete(mc_pair_stateless_t pair);

/********************************** Configuration of MC **************************************/
extern xbt_fifo_t mc_stack_safety;

extern int _surf_mc_checkpoint;
extern char* _surf_mc_property_file;

/****** Core dump ******/

int create_dump(int pair);

/****** Local variables with DWARF ******/

typedef enum {
  e_dw_loclist,
  e_dw_register,
  e_dw_bregister_op,
  e_dw_lit,
  e_dw_fbregister_op,
  e_dw_piece,
  e_dw_arithmetic,
  e_dw_plus_uconst,
  e_dw_compose,
  e_dw_deref,
  e_dw_uconstant,
  e_dw_sconstant,
  e_dw_unsupported
} e_dw_location_type;

typedef struct s_dw_location{
  e_dw_location_type type;
  union{
    
    xbt_dynar_t loclist;
    
    int reg;
    
    struct{
      unsigned int reg;
      int offset;
    }breg_op;

    unsigned int lit;

    int fbreg_op;

    int piece;

    unsigned short int deref_size;

    xbt_dynar_t compose;

    char *arithmetic;

    struct{
      int bytes;
      long unsigned int value;
    }uconstant;

    struct{
      int bytes;
      long signed int value;
    }sconstant;

    unsigned int plus_uconst;

  }location;
}s_dw_location_t, *dw_location_t;

typedef struct s_dw_location_entry{
  long lowpc;
  long highpc;
  dw_location_t location;
}s_dw_location_entry_t, *dw_location_entry_t;

typedef struct s_dw_local_variable{
  char *name;
  dw_location_t location;
}s_dw_local_variable_t, *dw_local_variable_t;

typedef struct s_dw_frame{
  char *name;
  void *low_pc;
  void *high_pc;
  dw_location_t frame_base;
  xbt_dict_t variables;
  unsigned long int start;
  unsigned long int end;
}s_dw_frame_t, *dw_frame_t;

/* FIXME : implement free functions for each structure */

extern xbt_dict_t mc_local_variables;

typedef struct s_variable_value{
  char *type;
  
  union{
    void *address;
    long int res;
  }value;
}s_variable_value_t, *variable_value_t;

void MC_get_local_variables(const char *elf_file, xbt_dict_t location_list, xbt_dict_t *variables);
void print_local_variables(xbt_dict_t list);
char *get_libsimgrid_path(void);
xbt_dict_t MC_get_location_list(const char *elf_file);

#endif

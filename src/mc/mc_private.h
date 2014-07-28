/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_PRIVATE_H
#define MC_PRIVATE_H

#include "simgrid_config.h"
#include <stdio.h>
#include <stdbool.h>
#ifndef WIN32
#include <sys/mman.h>
#endif
#include <elfutils/libdw.h>

#include "mc/mc.h"
#include "mc/datatypes.h"
#include "xbt/fifo.h"
#include "xbt/config.h"
#include "xbt/function_types.h"
#include "xbt/mmalloc.h"
#include "../simix/smx_private.h"
#include "../xbt/mmalloc/mmprivate.h"
#include "xbt/automaton.h"
#include "xbt/hash.h"
#include "msg/msg.h"
#include "msg/datatypes.h"
#include "xbt/strbuff.h"
#include "xbt/parmap.h"
#include "mc_mmu.h"
#include "mc_page_store.h"

SG_BEGIN_DECL()

typedef struct s_dw_frame s_dw_frame_t, *dw_frame_t;
typedef struct s_mc_function_index_item s_mc_function_index_item_t, *mc_function_index_item_t;

/****************************** Snapshots ***********************************/

#define NB_REGIONS 3 /* binary data (data + BSS) (type = 2), libsimgrid data (data + BSS) (type = 1), std_heap (type = 0)*/ 

/** @brief Copy/snapshot of a given memory region
 *
 *  Two types of region snapshots exist:
 *  <ul>
 *    <li>flat/dense snapshots are a simple copy of the region;</li>
 *    <li>sparse/per-page snapshots are snaapshots which shared
 *    identical pages.</li>
 *  </ul>
 */
typedef struct s_mc_mem_region{
  /** @brief  Virtual address of the region in the simulated process */
  void *start_addr;

  /** @brief Permanent virtual address of the region
   *
   * This is usually the same address as the simuilated process address.
   * However, when using SMPI privatization of global variables,
   * each SMPI process has its own set of global variables stored
   * at a different virtual address. The scheduler maps those region
   * on the region of the global variables.
   *
   * */
  void *permanent_addr;

  /** @brief Copy of the snapshot for flat snapshots regions (NULL otherwise) */
  void *data;

  /** @brief Size of the data region in bytes */
  size_t size;

  /** @brief Pages indices in the page store for per-page snapshots (NULL otherwise) */
  size_t* page_numbers;

} s_mc_mem_region_t, *mc_mem_region_t;

static inline  __attribute__ ((always_inline))
bool mc_region_contain(mc_mem_region_t region, void* p)
{
  return p >= region->start_addr &&
    p < (void*)((char*) region->start_addr + region->size);
}

/** Ignored data
 *
 *  Some parts of the snapshot are ignored by zeroing them out: the real
 *  values is stored here.
 * */
typedef struct s_mc_snapshot_ignored_data {
  void* start;
  size_t size;
  void* data;
} s_mc_snapshot_ignored_data_t, *mc_snapshot_ignored_data_t;

typedef struct s_mc_snapshot{
  size_t heap_bytes_used;
  mc_mem_region_t regions[NB_REGIONS];
  xbt_dynar_t enabled_processes;
  mc_mem_region_t* privatization_regions;
  int privatization_index;
  size_t *stack_sizes;
  xbt_dynar_t stacks;
  xbt_dynar_t to_ignore;
  uint64_t hash;
  xbt_dynar_t ignored_data;
} s_mc_snapshot_t, *mc_snapshot_t;

/** @brief Process index used when no process is available
 *
 *  The expected behaviour is that if a process index is needed it will fail.
 * */
#define MC_NO_PROCESS_INDEX -1

/** @brief Process index when any process is suitable
 *
 * We could use a special negative value in the future.
 */
#define MC_ANY_PROCESS_INDEX 0

mc_mem_region_t mc_get_snapshot_region(void* addr, mc_snapshot_t snapshot, int process_index);

static inline __attribute__ ((always_inline))
mc_mem_region_t mc_get_region_hinted(void* addr, mc_snapshot_t snapshot, int process_index, mc_mem_region_t region)
{
  if (mc_region_contain(region, addr))
    return region;
  else
    return mc_get_snapshot_region(addr, snapshot, process_index);
}

/** Information about a given stack frame
 *
 */
typedef struct s_mc_stack_frame {
  /** Instruction pointer */
  unw_word_t ip;
  /** Stack pointer */
  unw_word_t sp;
  unw_word_t frame_base;
  dw_frame_t frame;
  char* frame_name;
  unw_cursor_t unw_cursor;
} s_mc_stack_frame_t, *mc_stack_frame_t;

typedef struct s_mc_snapshot_stack{
  xbt_dynar_t local_variables;
  xbt_dynar_t stack_frames; // mc_stack_frame_t
  int process_index;
}s_mc_snapshot_stack_t, *mc_snapshot_stack_t;

typedef struct s_mc_global_t{
  mc_snapshot_t snapshot;
  int raw_mem_set;
  int prev_pair;
  char *prev_req;
  int initial_communications_pattern_done;
  int comm_deterministic;
  int send_deterministic;
}s_mc_global_t, *mc_global_t;

typedef struct s_mc_checkpoint_ignore_region{
  void *addr;
  size_t size;
}s_mc_checkpoint_ignore_region_t, *mc_checkpoint_ignore_region_t;

static void* mc_snapshot_get_heap_end(mc_snapshot_t snapshot);

mc_snapshot_t SIMIX_pre_mc_snapshot(smx_simcall_t simcall);
mc_snapshot_t MC_take_snapshot(int num_state);
void MC_restore_snapshot(mc_snapshot_t);
void MC_free_snapshot(mc_snapshot_t);

int mc_important_snapshot(mc_snapshot_t snapshot);

size_t* mc_take_page_snapshot_region(void* data, size_t page_count, uint64_t* pagemap, size_t* reference_pages);
void mc_free_page_snapshot_region(size_t* pagenos, size_t page_count);
void mc_restore_page_snapshot_region(void* start_addr, size_t page_count, size_t* pagenos, uint64_t* pagemap, size_t* reference_pagenos);

mc_mem_region_t mc_region_new_sparse(int type, void *start_addr, void* data_addr, size_t size, mc_mem_region_t ref_reg);
void MC_region_destroy(mc_mem_region_t reg);
void mc_region_restore_sparse(mc_mem_region_t reg, mc_mem_region_t ref_reg);
void mc_softdirty_reset();

static inline __attribute__((always_inline))
bool mc_snapshot_region_linear(mc_mem_region_t region) {
  return !region || !region->data;
}

void* mc_snapshot_read_fragmented(void* addr, mc_mem_region_t region, void* target, size_t size);

void* mc_snapshot_read(void* addr, mc_snapshot_t snapshot, int process_index, void* target, size_t size);
int mc_snapshot_region_memcmp(
  void* addr1, mc_mem_region_t region1,
  void* addr2, mc_mem_region_t region2, size_t size);
int mc_snapshot_memcmp(
  void* addr1, mc_snapshot_t snapshot1,
  void* addr2, mc_snapshot_t snapshot2, int process_index, size_t size);

static void* mc_snapshot_read_pointer(void* addr, mc_snapshot_t snapshot, int process_index);

/** @brief State of the model-checker (global variables for the model checker)
 *
 *  Each part of the state of the model chercker represented as a global
 *  variable prevents some sharing between snapshots and must be ignored.
 *  By moving as much state as possible in this structure allocated
 *  on the model-chercker heap, we avoid those issues.
 */
typedef struct s_mc_model_checker {
  // This is the parent snapshot of the current state:
  mc_snapshot_t parent_snapshot;
  mc_pages_store_t pages;
  int fd_clear_refs;
  int fd_pagemap;
} s_mc_model_checker_t, *mc_model_checker_t;

extern mc_model_checker_t mc_model_checker;

extern xbt_dynar_t mc_checkpoint_ignore;

/********************************* MC Global **********************************/

extern double *mc_time;
extern FILE *dot_output;
extern const char* colors[13];
extern xbt_parmap_t parmap;

extern int user_max_depth_reached;

int MC_deadlock_check(void);
void MC_replay(xbt_fifo_t stack, int start);
void MC_replay_liveness(xbt_fifo_t stack, int all_stack);
void MC_wait_for_requests(void);
void MC_show_deadlock(smx_simcall_t req);
void MC_show_stack_safety(xbt_fifo_t stack);
void MC_dump_stack_safety(xbt_fifo_t stack);
int SIMIX_pre_mc_random(smx_simcall_t simcall, int min, int max);

extern xbt_fifo_t mc_stack;
int get_search_interval(xbt_dynar_t list, void *ref, int *min, int *max);


/********************************* Requests ***********************************/

int MC_request_depend(smx_simcall_t req1, smx_simcall_t req2);
char* MC_request_to_string(smx_simcall_t req, int value);
unsigned int MC_request_testany_fail(smx_simcall_t req);
/*int MC_waitany_is_enabled_by_comm(smx_req_t req, unsigned int comm);*/
int MC_request_is_visible(smx_simcall_t req);
int MC_request_is_enabled(smx_simcall_t req);
int MC_request_is_enabled_by_idx(smx_simcall_t req, unsigned int idx);
int MC_process_is_enabled(smx_process_t process);
char *MC_request_get_dot_output(smx_simcall_t req, int value);


/******************************** States **************************************/

extern mc_global_t initial_global_state;

/* Possible exploration status of a process in a state */
typedef enum {
  MC_NOT_INTERLEAVE=0,      /* Do not interleave (do not execute) */
  MC_INTERLEAVE,            /* Interleave the process (one or more request) */
  MC_MORE_INTERLEAVE,       /* Interleave twice the process (for mc_random simcall) */
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
  int num;
} s_mc_state_t, *mc_state_t;

mc_state_t MC_state_new(void);
void MC_state_delete(mc_state_t state);
void MC_state_interleave_process(mc_state_t state, smx_process_t process);
unsigned int MC_state_interleave_size(mc_state_t state);
int MC_state_process_is_done(mc_state_t state, smx_process_t process);
void MC_state_set_executed_request(mc_state_t state, smx_simcall_t req, int value);
smx_simcall_t MC_state_get_executed_request(mc_state_t state, int *value);
smx_simcall_t MC_state_get_internal_request(mc_state_t state);
smx_simcall_t MC_state_get_request(mc_state_t state, int *value);
void MC_state_remove_interleave_process(mc_state_t state, smx_process_t process);


/****************************** Statistics ************************************/

typedef struct mc_stats {
  unsigned long state_size;
  unsigned long visited_states;
  unsigned long visited_pairs;
  unsigned long expanded_states;
  unsigned long expanded_pairs;
  unsigned long executed_transitions;
} s_mc_stats_t, *mc_stats_t;

extern mc_stats_t mc_stats;

void MC_print_statistics(mc_stats_t);


/********************************** MEMORY ******************************/
/* The possible memory modes for the modelchecker are standard and raw. */
/* Normally the system should operate in std, for switching to raw mode */
/* you must wrap the code between MC_SET_RAW_MODE and MC_UNSET_RAW_MODE */

extern void *std_heap;
extern void *mc_heap;


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

#define MC_SET_MC_HEAP    mmalloc_set_current_heap(mc_heap)
#define MC_SET_STD_HEAP  mmalloc_set_current_heap(std_heap)


/******************************* MEMORY MAPPINGS ***************************/
/* These functions and data structures implements a binary interface for   */
/* the proc maps ascii interface                                           */

/* Each field is defined as documented in proc's manual page  */
typedef struct s_map_region {

  void *start_addr;             /* Start address of the map */
  void *end_addr;               /* End address of the map */
  int prot;                     /* Memory protection */
  int flags;                    /* Additional memory flags */
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


void MC_init_memory_map_info(void);
memory_map_t MC_get_memory_map(void);
void MC_free_memory_map(memory_map_t map);

extern char *libsimgrid_path;

/********************************** Snapshot comparison **********************************/

typedef struct s_mc_comparison_times{
  double nb_processes_comparison_time;
  double bytes_used_comparison_time;
  double stacks_sizes_comparison_time;
  double binary_global_variables_comparison_time;
  double libsimgrid_global_variables_comparison_time;
  double heap_comparison_time;
  double stacks_comparison_time;
}s_mc_comparison_times_t, *mc_comparison_times_t;

extern __thread mc_comparison_times_t mc_comp_times;
extern __thread double mc_snapshot_comparison_time;

int snapshot_compare(void *state1, void *state2);
int SIMIX_pre_mc_compare_snapshots(smx_simcall_t simcall, mc_snapshot_t s1, mc_snapshot_t s2);
void print_comparison_times(void);

//#define MC_DEBUG 1
#define MC_VERBOSE 1

/********************************** Safety verification **************************************/

typedef enum {
  e_mc_reduce_unset,
  e_mc_reduce_none,
  e_mc_reduce_dpor
} e_mc_reduce_t;

extern e_mc_reduce_t mc_reduce_kind;
extern xbt_dict_t first_enabled_state;

void MC_pre_modelcheck_safety(void);
void MC_modelcheck_safety(void);

typedef struct s_mc_visited_state{
  mc_snapshot_t system_state;
  size_t heap_bytes_used;
  int nb_processes;
  int num;
  int other_num; // dot_output for
}s_mc_visited_state_t, *mc_visited_state_t;

extern xbt_dynar_t visited_states;
mc_visited_state_t is_visited_state(void);
void visited_state_free(mc_visited_state_t state);
void visited_state_free_voidp(void *s);

/********************************** Liveness verification **************************************/

extern xbt_automaton_t _mc_property_automaton;

typedef struct s_mc_pair{
  int num;
  int search_cycle;
  mc_state_t graph_state; /* System state included */
  xbt_automaton_state_t automaton_state;
  xbt_dynar_t atomic_propositions;
  int requests;
}s_mc_pair_t, *mc_pair_t;

typedef struct s_mc_visited_pair{
  int num;
  int other_num; /* Dot output for */
  int acceptance_pair;
  mc_state_t graph_state; /* System state included */
  xbt_automaton_state_t automaton_state;
  xbt_dynar_t atomic_propositions;
  size_t heap_bytes_used;
  int nb_processes;
  int acceptance_removed;
  int visited_removed;
}s_mc_visited_pair_t, *mc_visited_pair_t;

mc_pair_t MC_pair_new(void);
void MC_pair_delete(mc_pair_t);
void mc_pair_free_voidp(void *p);
mc_visited_pair_t MC_visited_pair_new(int pair_num, xbt_automaton_state_t automaton_state, xbt_dynar_t atomic_propositions);
void MC_visited_pair_delete(mc_visited_pair_t p);

void MC_pre_modelcheck_liveness(void);
void MC_modelcheck_liveness(void);
void MC_show_stack_liveness(xbt_fifo_t stack);
void MC_dump_stack_liveness(xbt_fifo_t stack);

extern xbt_dynar_t visited_pairs;
int is_visited_pair(mc_visited_pair_t pair, int pair_num, xbt_automaton_state_t automaton_state, xbt_dynar_t atomic_propositions);


/********************************** Variables with DWARF **********************************/

#define MC_OBJECT_INFO_EXECUTABLE 1

struct s_mc_object_info {
  size_t flags;
  char* file_name;
  char *start_exec, *end_exec; // Executable segment
  char *start_rw, *end_rw; // Read-write segment
  char *start_ro, *end_ro; // read-only segment
  xbt_dict_t subprograms; // xbt_dict_t<origin as hexadecimal string, dw_frame_t>
  xbt_dynar_t global_variables; // xbt_dynar_t<dw_variable_t>
  xbt_dict_t types; // xbt_dict_t<origin as hexadecimal string, dw_type_t>
  xbt_dict_t full_types_by_name; // xbt_dict_t<name, dw_type_t> (full defined type only)

  // Here we sort the minimal information for an efficient (and cache-efficient)
  // lookup of a function given an instruction pointer.
  // The entries are sorted by low_pc and a binary search can be used to look them up.
  xbt_dynar_t functions_index;
};

mc_object_info_t MC_new_object_info(void);
mc_object_info_t MC_find_object_info(memory_map_t maps, char* name, int executable);
void MC_free_object_info(mc_object_info_t* p);

void MC_dwarf_get_variables(mc_object_info_t info);
void MC_dwarf_get_variables_libdw(mc_object_info_t info);
const char* MC_dwarf_attrname(int attr);
const char* MC_dwarf_tagname(int tag);

dw_frame_t MC_find_function_by_ip(void* ip);
mc_object_info_t MC_ip_find_object_info(void* ip);

extern mc_object_info_t mc_libsimgrid_info;
extern mc_object_info_t mc_binary_info;
extern mc_object_info_t mc_object_infos[2];
extern size_t mc_object_infos_size;

void MC_find_object_address(memory_map_t maps, mc_object_info_t result);
void MC_post_process_types(mc_object_info_t info);
void MC_post_process_object_info(mc_object_info_t info);

// ***** Expressions

/** \brief a DWARF expression with optional validity contraints */
typedef struct s_mc_expression {
  size_t size;
  Dwarf_Op* ops;
  // Optional validity:
  void* lowpc, *highpc;
} s_mc_expression_t, *mc_expression_t;

/** A location list (list of location expressions) */
typedef struct s_mc_location_list {
  size_t size;
  mc_expression_t locations;
} s_mc_location_list_t, *mc_location_list_t;

uintptr_t mc_dwarf_resolve_location(mc_expression_t expression, mc_object_info_t object_info, unw_cursor_t* c, void* frame_pointer_address, mc_snapshot_t snapshot, int process_index);
uintptr_t mc_dwarf_resolve_locations(mc_location_list_t locations, mc_object_info_t object_info, unw_cursor_t* c, void* frame_pointer_address, mc_snapshot_t snapshot, int process_index);

void mc_dwarf_expression_clear(mc_expression_t expression);
void mc_dwarf_expression_init(mc_expression_t expression, size_t len, Dwarf_Op* ops);

void mc_dwarf_location_list_clear(mc_location_list_t list);

void mc_dwarf_location_list_init_from_expression(mc_location_list_t target, size_t len, Dwarf_Op* ops);
void mc_dwarf_location_list_init(mc_location_list_t target, mc_object_info_t info, Dwarf_Die* die, Dwarf_Attribute* attr);

// ***** Variables and functions

struct s_dw_type{
  e_dw_type_type type;
  Dwarf_Off id; /* Offset in the section (in hexadecimal form) */
  char *name; /* Name of the type */
  int byte_size; /* Size in bytes */
  int element_count; /* Number of elements for array type */
  char *dw_type_id; /* DW_AT_type id */
  xbt_dynar_t members; /* if DW_TAG_structure_type, DW_TAG_class_type, DW_TAG_union_type*/
  int is_pointer_type;

  // Location (for members) is either of:
  struct s_mc_expression location;
  int offset;

  dw_type_t subtype; // DW_AT_type
  dw_type_t full_type; // The same (but more complete) type
};

void* mc_member_resolve(const void* base, dw_type_t type, dw_type_t member, mc_snapshot_t snapshot, int process_index);

typedef struct s_dw_variable{
  Dwarf_Off dwarf_offset; /* Global offset of the field. */
  int global;
  char *name;
  char *type_origin;
  dw_type_t type;

  // Use either of:
  s_mc_location_list_t locations;
  void* address;

  size_t start_scope;
  mc_object_info_t object_info;

}s_dw_variable_t, *dw_variable_t;

struct s_dw_frame{
  int tag;
  char *name;
  void *low_pc;
  void *high_pc;
  s_mc_location_list_t frame_base;
  xbt_dynar_t /* <dw_variable_t> */ variables; /* Cannot use dict, there may be several variables with the same name (in different lexical blocks)*/
  unsigned long int id; /* DWARF offset of the subprogram */
  xbt_dynar_t /* <dw_frame_t> */ scopes;
  Dwarf_Off abstract_origin_id;
  mc_object_info_t object_info;
};

struct s_mc_function_index_item {
  void* low_pc, *high_pc;
  dw_frame_t function;
};

void mc_frame_free(dw_frame_t freme);

void dw_type_free(dw_type_t t);
void dw_variable_free(dw_variable_t v);
void dw_variable_free_voidp(void *t);

void MC_dwarf_register_global_variable(mc_object_info_t info, dw_variable_t variable);
void MC_register_variable(mc_object_info_t info, dw_frame_t frame, dw_variable_t variable);
void MC_dwarf_register_non_global_variable(mc_object_info_t info, dw_frame_t frame, dw_variable_t variable);
void MC_dwarf_register_variable(mc_object_info_t info, dw_frame_t frame, dw_variable_t variable);

/** Find the DWARF offset for this ELF object
 *
 *  An offset is applied to address found in DWARF:
 *
 *  <ul>
 *    <li>for an executable obejct, addresses are virtual address
 *        (there is no offset) i.e. \f$\text{virtual address} = \{dwarf address}\f$;</li>
 *    <li>for a shared object, the addreses are offset from the begining
 *        of the shared object (the base address of the mapped shared
 *        object must be used as offset
 *        i.e. \f$\text{virtual address} = \text{shared object base address}
 *             + \text{dwarf address}\f$.</li>
 *
 */
void* MC_object_base_address(mc_object_info_t info);

/********************************** DWARF **********************************/

#define MC_EXPRESSION_STACK_SIZE 64

#define MC_EXPRESSION_OK 0
#define MC_EXPRESSION_E_UNSUPPORTED_OPERATION 1
#define MC_EXPRESSION_E_STACK_OVERFLOW 2
#define MC_EXPRESSION_E_STACK_UNDERFLOW 3
#define MC_EXPRESSION_E_MISSING_STACK_CONTEXT 4
#define MC_EXPRESSION_E_MISSING_FRAME_BASE 5
#define MC_EXPRESSION_E_NO_BASE_ADDRESS 6

typedef struct s_mc_expression_state {
  uintptr_t stack[MC_EXPRESSION_STACK_SIZE];
  size_t stack_size;

  unw_cursor_t* cursor;
  void* frame_base;
  mc_snapshot_t snapshot;
  mc_object_info_t object_info;
  int process_index;
} s_mc_expression_state_t, *mc_expression_state_t;

int mc_dwarf_execute_expression(size_t n, const Dwarf_Op* ops, mc_expression_state_t state);

void* mc_find_frame_base(dw_frame_t frame, mc_object_info_t object_info, unw_cursor_t* unw_cursor);

/********************************** Miscellaneous **********************************/

typedef struct s_local_variable{
  dw_frame_t subprogram;
  unsigned long ip;
  char *name;
  dw_type_t type;
  void *address;
  int region;
}s_local_variable_t, *local_variable_t;

/********************************* Communications pattern ***************************/

typedef struct s_mc_comm_pattern{
  int num;
  smx_action_t comm;
  e_smx_comm_type_t type;
  unsigned long src_proc;
  unsigned long dst_proc;
  const char *src_host;
  const char *dst_host;
  char *rdv;
  ssize_t data_size;
  void *data;
}s_mc_comm_pattern_t, *mc_comm_pattern_t;

extern xbt_dynar_t initial_communications_pattern;
extern xbt_dynar_t communications_pattern;
extern xbt_dynar_t incomplete_communications_pattern;

void get_comm_pattern(xbt_dynar_t communications_pattern, smx_simcall_t request, int call);
void complete_comm_pattern(xbt_dynar_t list, smx_action_t comm);
void MC_pre_modelcheck_comm_determinism(void);
void MC_modelcheck_comm_determinism(void);

/* *********** Sets *********** */

typedef struct s_mc_address_set *mc_address_set_t;

mc_address_set_t mc_address_set_new();
void mc_address_set_free(mc_address_set_t* p);
void mc_address_add(mc_address_set_t p, const void* value);
bool mc_address_test(mc_address_set_t p, const void* value);

/* *********** Hash *********** */

/** \brief Hash the current state
 *  \param num_state number of states
 *  \param stacks stacks (mc_snapshot_stak_t) used fot the stack unwinding informations
 *  \result resulting hash
 * */
uint64_t mc_hash_processes_state(int num_state, xbt_dynar_t stacks);

/* *********** Snapshot *********** */

static inline __attribute__((always_inline))
void* mc_translate_address_region(uintptr_t addr, mc_mem_region_t region)
{
    size_t pageno = mc_page_number(region->start_addr, (void*) addr);
    size_t snapshot_pageno = region->page_numbers[pageno];
    const void* snapshot_page = mc_page_store_get_page(mc_model_checker->pages, snapshot_pageno);
    return (char*) snapshot_page + mc_page_offset((void*) addr);
}

/** \brief Translate a pointer from process address space to snapshot address space
 *
 *  The address space contains snapshot of the main/application memory:
 *  this function finds the address in a given snaphot for a given
 *  real/application address.
 *
 *  For read only memory regions and other regions which are not int the
 *  snapshot, the address is not changed.
 *
 *  \param addr     Application address
 *  \param snapshot The snapshot of interest (if NULL no translation is done)
 *  \return         Translated address in the snapshot address space
 * */
static inline __attribute__((always_inline))
void* mc_translate_address(uintptr_t addr, mc_snapshot_t snapshot, int process_index)
{

  // If not in a process state/clone:
  if (!snapshot) {
    return (uintptr_t *) addr;
  }

  mc_mem_region_t region = mc_get_snapshot_region((void*) addr, snapshot, process_index);

  xbt_assert(mc_region_contain(region, (void*) addr), "Trying to read out of the region boundary.");

  if (!region) {
    return (void *) addr;
  }

  // Flat snapshot:
  else if (region->data) {
    uintptr_t offset = addr - (uintptr_t) region->start_addr;
    return (void *) ((uintptr_t) region->data + offset);
  }

  // Per-page snapshot:
  else if (region->page_numbers) {
    return mc_translate_address_region(addr, region);
  }

  else {
    xbt_die("No data for this memory region");
  }
}

static inline __attribute__ ((always_inline))
  void* mc_snapshot_get_heap_end(mc_snapshot_t snapshot) {
  if(snapshot==NULL)
      xbt_die("snapshot is NULL");
  void** addr = &((xbt_mheap_t)std_heap)->breakval;
  return mc_snapshot_read_pointer(addr, snapshot, MC_ANY_PROCESS_INDEX);
}

static inline __attribute__ ((always_inline))
void* mc_snapshot_read_pointer(void* addr, mc_snapshot_t snapshot, int process_index)
{
  void* res;
  return *(void**) mc_snapshot_read(addr, snapshot, process_index, &res, sizeof(void*));
}

/** @brief Read memory from a snapshot region
 *
 *  @param addr    Process (non-snapshot) address of the data
 *  @param region  Snapshot memory region where the data is located
 *  @param target  Buffer to store the value
 *  @param size    Size of the data to read in bytes
 *  @return Pointer where the data is located (target buffer of original location)
 */
static inline __attribute__((always_inline))
void* mc_snapshot_read_region(void* addr, mc_mem_region_t region, void* target, size_t size)
{
  if (region==NULL)
    return addr;

  uintptr_t offset = (char*) addr - (char*) region->start_addr;

  xbt_assert(mc_region_contain(region, addr),
    "Trying to read out of the region boundary.");

  // Linear memory region:
  if (region->data) {
    return (char*) region->data + offset;
  }

  // Fragmented memory region:
  else if (region->page_numbers) {
    // Last byte of the region:
    void* end = (char*) addr + size - 1;
    if( mc_same_page(addr, end) ) {
      // The memory is contained in a single page:
      return mc_translate_address_region((uintptr_t) addr, region);
    } else {
      // The memory spans several pages:
      return mc_snapshot_read_fragmented(addr, region, target, size);
    }
  }

  else {
    xbt_die("No data available for this region");
  }
}

static inline __attribute__ ((always_inline))
void* mc_snapshot_read_pointer_region(void* addr, mc_mem_region_t region)
{
  void* res;
  return *(void**) mc_snapshot_read_region(addr, region, &res, sizeof(void*));
}

SG_END_DECL()

#endif


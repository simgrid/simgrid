/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <libgen.h>

#include "simgrid/sg_config.h"
#include "../surf/surf_private.h"
#include "../simix/smx_private.h"
#include "../xbt/mmalloc/mmprivate.h"
#include "xbt/fifo.h"
#include "mc_private.h"
#include "xbt/automaton.h"
#include "xbt/dict.h"

XBT_LOG_NEW_CATEGORY(mc, "All MC categories");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_global, mc,
                                "Logging specific to MC (global)");

/* Configuration support */
e_mc_reduce_t mc_reduce_kind=e_mc_reduce_unset;

int _sg_do_model_check = 0;
int _sg_mc_checkpoint=0;
char* _sg_mc_property_file=NULL;
int _sg_mc_timeout=0;
int _sg_mc_hash=0;
int _sg_mc_max_depth=1000;
int _sg_mc_visited=0;
char *_sg_mc_dot_output_file = NULL;
int _sg_mc_comms_determinism=0;
int _sg_mc_send_determinism=0;

int user_max_depth_reached = 0;

void _mc_cfg_cb_reduce(const char *name, int pos) {
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die("You are specifying a reduction strategy after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  char *val= xbt_cfg_get_string(_sg_cfg_set, name);
  if (!strcasecmp(val,"none")) {
    mc_reduce_kind = e_mc_reduce_none;
  } else if (!strcasecmp(val,"dpor")) {
    mc_reduce_kind = e_mc_reduce_dpor;
  } else {
    xbt_die("configuration option %s can only take 'none' or 'dpor' as a value",name);
  }
}

void _mc_cfg_cb_checkpoint(const char *name, int pos) {
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die("You are specifying a checkpointing value after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_checkpoint = xbt_cfg_get_int(_sg_cfg_set, name);
}
void _mc_cfg_cb_property(const char *name, int pos) {
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die("You are specifying a property after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_property_file= xbt_cfg_get_string(_sg_cfg_set, name);
}

void _mc_cfg_cb_timeout(const char *name, int pos) {
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die("You are specifying a value to enable/disable timeout for wait requests after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_timeout= xbt_cfg_get_boolean(_sg_cfg_set, name);
}

void _mc_cfg_cb_hash(const char *name, int pos) {
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die("You are specifying a value to enable/disable the use of global hash to speedup state comparaison, but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_hash= xbt_cfg_get_boolean(_sg_cfg_set, name);
}

void _mc_cfg_cb_max_depth(const char *name, int pos) {
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die("You are specifying a max depth value after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_max_depth= xbt_cfg_get_int(_sg_cfg_set, name);
}

void _mc_cfg_cb_visited(const char *name, int pos) {
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die("You are specifying a number of stored visited states after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_visited= xbt_cfg_get_int(_sg_cfg_set, name);
}

void _mc_cfg_cb_dot_output(const char *name, int pos) {
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die("You are specifying a file name for a dot output of graph state after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_dot_output_file= xbt_cfg_get_string(_sg_cfg_set, name);
}

void _mc_cfg_cb_comms_determinism(const char *name, int pos) {
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die("You are specifying a value to enable/disable the detection of determinism in the communications schemes after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_comms_determinism= xbt_cfg_get_boolean(_sg_cfg_set, name);
}

void _mc_cfg_cb_send_determinism(const char *name, int pos) {
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die("You are specifying a value to enable/disable the detection of send-determinism in the communications schemes after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_send_determinism= xbt_cfg_get_boolean(_sg_cfg_set, name);
}

/* MC global data structures */
mc_state_t mc_current_state = NULL;
char mc_replay_mode = FALSE;
double *mc_time = NULL;
__thread mc_comparison_times_t mc_comp_times = NULL;
__thread double mc_snapshot_comparison_time;
mc_stats_t mc_stats = NULL;

/* Safety */
xbt_fifo_t mc_stack_safety = NULL;
mc_global_t initial_state_safety = NULL;

/* Liveness */
xbt_fifo_t mc_stack_liveness = NULL;
mc_global_t initial_state_liveness = NULL;
int compare;

xbt_automaton_t _mc_property_automaton = NULL;

/* Variables */
mc_object_info_t mc_libsimgrid_info = NULL;
mc_object_info_t mc_binary_info = NULL;

mc_object_info_t mc_object_infos[2] = { NULL, NULL };
size_t mc_object_infos_size = 2;

/* Ignore mechanism */
extern xbt_dynar_t mc_heap_comparison_ignore;
extern xbt_dynar_t stacks_areas;

/* Dot output */
FILE *dot_output = NULL;
const char* colors[13];


/*******************************  DWARF Information *******************************/
/**********************************************************************************/

/************************** Free functions *************************/

void mc_frame_free(dw_frame_t frame){
  xbt_free(frame->name);
  mc_dwarf_location_list_clear(&(frame->frame_base));
  xbt_dynar_free(&(frame->variables));
  xbt_dynar_free(&(frame->scopes));
  xbt_free(frame);
}

void dw_type_free(dw_type_t t){
  xbt_free(t->name);
  xbt_free(t->dw_type_id);
  xbt_dynar_free(&(t->members));
  mc_dwarf_expression_clear(&t->location);
  xbt_free(t);
}

void dw_variable_free(dw_variable_t v){
  if(v){
    xbt_free(v->name);
    xbt_free(v->type_origin);

    if(v->locations.locations)
      mc_dwarf_location_list_clear(&v->locations);
    xbt_free(v);
  }
}

void dw_variable_free_voidp(void *t){
  dw_variable_free((dw_variable_t) * (void **) t);
}

// ***** object_info



mc_object_info_t MC_new_object_info(void) {
  mc_object_info_t res = xbt_new0(s_mc_object_info_t, 1);
  res->subprograms = xbt_dict_new_homogeneous((void (*)(void*))mc_frame_free);
  res->global_variables = xbt_dynar_new(sizeof(dw_variable_t), dw_variable_free_voidp);
  res->types = xbt_dict_new_homogeneous((void (*)(void*))dw_type_free);
  res->full_types_by_name = xbt_dict_new_homogeneous(NULL);
  return res;
}

void MC_free_object_info(mc_object_info_t* info) {
  xbt_free(&(*info)->file_name);
  xbt_dict_free(&(*info)->subprograms);
  xbt_dynar_free(&(*info)->global_variables);
  xbt_dict_free(&(*info)->types);
  xbt_dict_free(&(*info)->full_types_by_name);
  xbt_free(info);
  xbt_dynar_free(&(*info)->functions_index);
  *info = NULL;
}

// ***** Helpers

void* MC_object_base_address(mc_object_info_t info) {
  if(info->flags & MC_OBJECT_INFO_EXECUTABLE)
    return 0;
  void* result = info->start_exec;
  if(info->start_rw!=NULL && result > (void*) info->start_rw) result = info->start_rw;
  if(info->start_ro!=NULL && result > (void*) info->start_ro) result = info->start_ro;
  return result;
}

// ***** Functions index

static int MC_compare_frame_index_items(mc_function_index_item_t a, mc_function_index_item_t b) {
  if(a->low_pc < b->low_pc)
    return -1;
  else if(a->low_pc == b->low_pc)
    return 0;
  else
    return 1;
}

static void MC_make_functions_index(mc_object_info_t info) {
  xbt_dynar_t index = xbt_dynar_new(sizeof(s_mc_function_index_item_t), NULL);

  // Populate the array:
  dw_frame_t frame = NULL;
  xbt_dict_cursor_t cursor;
  char* key;
  xbt_dict_foreach(info->subprograms, cursor, key, frame) {
    if(frame->low_pc==NULL)
      continue;
    s_mc_function_index_item_t entry;
    entry.low_pc = frame->low_pc;
    entry.high_pc = frame->high_pc;
    entry.function = frame;
    xbt_dynar_push(index, &entry);
  }

  mc_function_index_item_t base = (mc_function_index_item_t) xbt_dynar_get_ptr(index, 0);

  // Sort the array by low_pc:
  qsort(base,
    xbt_dynar_length(index),
    sizeof(s_mc_function_index_item_t),
    (int (*)(const void *, const void *))MC_compare_frame_index_items);

  info->functions_index = index;
}

mc_object_info_t MC_ip_find_object_info(void* ip) {
  size_t i;
  for(i=0; i!=mc_object_infos_size; ++i) {
    if(ip >= (void*)mc_object_infos[i]->start_exec && ip <= (void*)mc_object_infos[i]->end_exec) {
      return mc_object_infos[i];
    }
  }
  return NULL;
}

static dw_frame_t MC_find_function_by_ip_and_object(void* ip, mc_object_info_t info) {
  xbt_dynar_t dynar = info->functions_index;
  mc_function_index_item_t base = (mc_function_index_item_t) xbt_dynar_get_ptr(dynar, 0);
  int i = 0;
  int j = xbt_dynar_length(dynar) - 1;
  while(j>=i) {
    int k = i + ((j-i)/2);
    if(ip < base[k].low_pc) {
      j = k-1;
    } else if(ip >= base[k].high_pc) {
      i = k+1;
    } else {
      return base[k].function;
    }
  }
  return NULL;
}

dw_frame_t MC_find_function_by_ip(void* ip) {
  mc_object_info_t info = MC_ip_find_object_info(ip);
  if(info==NULL)
    return NULL;
  else
    return MC_find_function_by_ip_and_object(ip, info);
}

static void MC_post_process_variables(mc_object_info_t info) {
  unsigned cursor = 0;
  dw_variable_t variable = NULL;
  xbt_dynar_foreach(info->global_variables, cursor, variable) {
    if(variable->type_origin) {
      variable->type = xbt_dict_get_or_null(info->types, variable->type_origin);
    }
  }
}

static void mc_post_process_scope(mc_object_info_t info, dw_frame_t scope) {

  if(scope->tag == DW_TAG_inlined_subroutine) {

    // Attach correct namespaced name in inlined subroutine:
    char* key = bprintf("%" PRIx64, (uint64_t) scope->abstract_origin_id);
    dw_frame_t abstract_origin = xbt_dict_get_or_null(info->subprograms, key);
    xbt_assert(abstract_origin, "Could not lookup abstract origin %s", key);
    xbt_free(key);
    scope->name = xbt_strdup(abstract_origin->name);

  }

  // Direct:
  unsigned cursor = 0;
  dw_variable_t variable = NULL;
  xbt_dynar_foreach(scope->variables, cursor, variable) {
    if(variable->type_origin) {
      variable->type = xbt_dict_get_or_null(info->types, variable->type_origin);
    }
  }

  // Recursive post-processing of nested-scopes:
  dw_frame_t nested_scope = NULL;
  xbt_dynar_foreach(scope->scopes, cursor, nested_scope)
    mc_post_process_scope(info, nested_scope);

}

static void MC_post_process_functions(mc_object_info_t info) {
  xbt_dict_cursor_t cursor;
  char* key;
  dw_frame_t subprogram = NULL;
  xbt_dict_foreach(info->subprograms, cursor, key, subprogram) {
    mc_post_process_scope(info, subprogram);
  }
}

/** \brief Finds informations about a given shared object/executable */
mc_object_info_t MC_find_object_info(memory_map_t maps, char* name, int executable) {
  mc_object_info_t result = MC_new_object_info();
  if(executable)
    result->flags |= MC_OBJECT_INFO_EXECUTABLE;
  result->file_name = xbt_strdup(name);
  MC_find_object_address(maps, result);
  MC_dwarf_get_variables(result);
  MC_post_process_types(result);
  MC_post_process_variables(result);
  MC_post_process_functions(result);
  MC_make_functions_index(result);
  return result;
}

/*************************************************************************/

static int MC_dwarf_get_variable_index(xbt_dynar_t variables, char* var, void *address){

  if(xbt_dynar_is_empty(variables))
    return 0;

  unsigned int cursor = 0;
  int start = 0;
  int end = xbt_dynar_length(variables) - 1;
  dw_variable_t var_test = NULL;

  while(start <= end){
    cursor = (start + end) / 2;
    var_test = (dw_variable_t)xbt_dynar_get_as(variables, cursor, dw_variable_t);
    if(strcmp(var_test->name, var) < 0){
      start = cursor + 1;
    }else if(strcmp(var_test->name, var) > 0){
      end = cursor - 1;
    }else{
      if(address){ /* global variable */
        if(var_test->address == address)
          return -1;
        if(var_test->address > address)
          end = cursor - 1;
        else
          start = cursor + 1;
      }else{ /* local variable */
        return -1;
      }
    }
  }

  if(strcmp(var_test->name, var) == 0){
    if(address && var_test->address < address)
      return cursor+1;
    else
      return cursor;
  }else if(strcmp(var_test->name, var) < 0)
    return cursor+1;
  else
    return cursor;

}

void MC_dwarf_register_global_variable(mc_object_info_t info, dw_variable_t variable) {
  int index = MC_dwarf_get_variable_index(info->global_variables, variable->name, variable->address);
  if (index != -1)
    xbt_dynar_insert_at(info->global_variables, index, &variable);
  // TODO, else ?
}

void MC_dwarf_register_non_global_variable(mc_object_info_t info, dw_frame_t frame, dw_variable_t variable) {
  xbt_assert(frame, "Frame is NULL");
  int index = MC_dwarf_get_variable_index(frame->variables, variable->name, NULL);
  if (index != -1)
    xbt_dynar_insert_at(frame->variables, index, &variable);
  // TODO, else ?
}

void MC_dwarf_register_variable(mc_object_info_t info, dw_frame_t frame, dw_variable_t variable) {
  if(variable->global)
    MC_dwarf_register_global_variable(info, variable);
  else if(frame==NULL)
    xbt_die("No frame for this local variable");
  else
    MC_dwarf_register_non_global_variable(info, frame, variable);
}


/*******************************  Ignore mechanism *******************************/
/*********************************************************************************/

xbt_dynar_t mc_checkpoint_ignore;

typedef struct s_mc_stack_ignore_variable{
  char *var_name;
  char *frame;
}s_mc_stack_ignore_variable_t, *mc_stack_ignore_variable_t;

/**************************** Free functions ******************************/

static void stack_ignore_variable_free(mc_stack_ignore_variable_t v){
  xbt_free(v->var_name);
  xbt_free(v->frame);
  xbt_free(v);
}

static void stack_ignore_variable_free_voidp(void *v){
  stack_ignore_variable_free((mc_stack_ignore_variable_t) * (void **) v);
}

void heap_ignore_region_free(mc_heap_ignore_region_t r){
  xbt_free(r);
}

void heap_ignore_region_free_voidp(void *r){
  heap_ignore_region_free((mc_heap_ignore_region_t) * (void **) r);
}

static void checkpoint_ignore_region_free(mc_checkpoint_ignore_region_t r){
  xbt_free(r);
}

static void checkpoint_ignore_region_free_voidp(void *r){
  checkpoint_ignore_region_free((mc_checkpoint_ignore_region_t) * (void **) r);
}

/***********************************************************************/

void MC_ignore_heap(void *address, size_t size){

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  mc_heap_ignore_region_t region = NULL;
  region = xbt_new0(s_mc_heap_ignore_region_t, 1);
  region->address = address;
  region->size = size;
  
  region->block = ((char*)address - (char*)((xbt_mheap_t)std_heap)->heapbase) / BLOCKSIZE + 1;
  
  if(((xbt_mheap_t)std_heap)->heapinfo[region->block].type == 0){
    region->fragment = -1;
    ((xbt_mheap_t)std_heap)->heapinfo[region->block].busy_block.ignore++;
  }else{
    region->fragment = ((uintptr_t) (ADDR2UINT (address) % (BLOCKSIZE))) >> ((xbt_mheap_t)std_heap)->heapinfo[region->block].type;
    ((xbt_mheap_t)std_heap)->heapinfo[region->block].busy_frag.ignore[region->fragment]++;
  }
  
  if(mc_heap_comparison_ignore == NULL){
    mc_heap_comparison_ignore = xbt_dynar_new(sizeof(mc_heap_ignore_region_t), heap_ignore_region_free_voidp);
    xbt_dynar_push(mc_heap_comparison_ignore, &region);
    if(!raw_mem_set)
      MC_UNSET_RAW_MEM;
    return;
  }

  unsigned int cursor = 0;
  mc_heap_ignore_region_t current_region = NULL;
  int start = 0;
  int end = xbt_dynar_length(mc_heap_comparison_ignore) - 1;
  
  while(start <= end){
    cursor = (start + end) / 2;
    current_region = (mc_heap_ignore_region_t)xbt_dynar_get_as(mc_heap_comparison_ignore, cursor, mc_heap_ignore_region_t);
    if(current_region->address == address){
      heap_ignore_region_free(region);
      if(!raw_mem_set)
        MC_UNSET_RAW_MEM;
      return;
    }else if(current_region->address < address){
      start = cursor + 1;
    }else{
      end = cursor - 1;
    }   
  }

  if(current_region->address < address)
    xbt_dynar_insert_at(mc_heap_comparison_ignore, cursor + 1, &region);
  else
    xbt_dynar_insert_at(mc_heap_comparison_ignore, cursor, &region);

  if(!raw_mem_set)
    MC_UNSET_RAW_MEM;
}

void MC_remove_ignore_heap(void *address, size_t size){

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  unsigned int cursor = 0;
  int start = 0;
  int end = xbt_dynar_length(mc_heap_comparison_ignore) - 1;
  mc_heap_ignore_region_t region;
  int ignore_found = 0;

  while(start <= end){
    cursor = (start + end) / 2;
    region = (mc_heap_ignore_region_t)xbt_dynar_get_as(mc_heap_comparison_ignore, cursor, mc_heap_ignore_region_t);
    if(region->address == address){
      ignore_found = 1;
      break;
    }else if(region->address < address){
      start = cursor + 1;
    }else{
      if((char * )region->address <= ((char *)address + size)){
        ignore_found = 1;
        break;
      }else{
        end = cursor - 1;   
      }
    }
  }
  
  if(ignore_found == 1){
    xbt_dynar_remove_at(mc_heap_comparison_ignore, cursor, NULL);
    MC_remove_ignore_heap(address, size);
  }

  if(!raw_mem_set)
    MC_UNSET_RAW_MEM;

}

void MC_ignore_global_variable(const char *name){

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  xbt_assert(mc_libsimgrid_info, "MC subsystem not initialized");

    unsigned int cursor = 0;
    dw_variable_t current_var;
    int start = 0;
    int end = xbt_dynar_length(mc_libsimgrid_info->global_variables) - 1;

    while(start <= end){
      cursor = (start + end) /2;
      current_var = (dw_variable_t)xbt_dynar_get_as(mc_libsimgrid_info->global_variables, cursor, dw_variable_t);
      if(strcmp(current_var->name, name) == 0){
        xbt_dynar_remove_at(mc_libsimgrid_info->global_variables, cursor, NULL);
        start = 0;
        end = xbt_dynar_length(mc_libsimgrid_info->global_variables) - 1;
      }else if(strcmp(current_var->name, name) < 0){
        start = cursor + 1;
      }else{
        end = cursor - 1;
      } 
    }

  if(!raw_mem_set)
    MC_UNSET_RAW_MEM;
}

/** \brief Ignore a local variable in a scope
 *
 *  Ignore all instances of variables with a given name in
 *  any (possibly inlined) subprogram with a given namespaced
 *  name.
 *
 *  \param var_name        Name of the local variable (or parameter to ignore)
 *  \param subprogram_name Name of the subprogram fo ignore (NULL for any)
 *  \param subprogram      (possibly inlined) Subprogram of the scope
 *  \param scope           Current scope
 */
static void mc_ignore_local_variable_in_scope(
  const char *var_name, const char *subprogram_name,
  dw_frame_t subprogram, dw_frame_t scope) {
  // Processing of direct variables:

  // If the current subprogram matche the given name:
  if(subprogram_name==NULL || strcmp(subprogram_name, subprogram->name)==0) {

    // Try to find the variable and remove it:
    int start = 0;
    int end = xbt_dynar_length(scope->variables) - 1;

    // Dichotomic search:
    while(start <= end){
      int cursor = (start + end) / 2;
      dw_variable_t current_var = (dw_variable_t)xbt_dynar_get_as(scope->variables, cursor, dw_variable_t);

      int compare = strcmp(current_var->name, var_name);
      if(compare == 0){
        // Variable found, remove it:
        xbt_dynar_remove_at(scope->variables, cursor, NULL);

        // and start again:
        start = 0;
        end = xbt_dynar_length(scope->variables) - 1;
      }else if(compare < 0){
        start = cursor + 1;
      }else{
        end = cursor - 1;
      }
    }

  }

  // And recursive processing in nested scopes:
  unsigned cursor = 0;
  dw_frame_t nested_scope = NULL;
  xbt_dynar_foreach(scope->scopes, cursor, nested_scope) {
    // The new scope may be an inlined subroutine, in this case we want to use its
    // namespaced name in recursive calls:
    dw_frame_t nested_subprogram = nested_scope->tag == DW_TAG_inlined_subroutine ? nested_scope : subprogram;

    mc_ignore_local_variable_in_scope(var_name, subprogram_name, nested_subprogram, nested_scope);
  }
}

static void MC_ignore_local_variable_in_object(const char *var_name, const char *subprogram_name, mc_object_info_t info) {
  xbt_dict_cursor_t cursor2;
  dw_frame_t frame;
  char* key;
  xbt_dict_foreach(info->subprograms, cursor2, key, frame) {
    mc_ignore_local_variable_in_scope(var_name, subprogram_name, frame, frame);
  }
}

void MC_ignore_local_variable(const char *var_name, const char *frame_name){
  
  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  if(strcmp(frame_name, "*") == 0)
    frame_name = NULL;

  MC_SET_RAW_MEM;

  MC_ignore_local_variable_in_object(var_name, frame_name, mc_libsimgrid_info);
  if(frame_name!=NULL)
    MC_ignore_local_variable_in_object(var_name, frame_name, mc_binary_info);

  if(!raw_mem_set)
    MC_UNSET_RAW_MEM;

}

void MC_new_stack_area(void *stack, char *name, void* context, size_t size){

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  if(stacks_areas == NULL)
    stacks_areas = xbt_dynar_new(sizeof(stack_region_t), NULL);
  
  stack_region_t region = NULL;
  region = xbt_new0(s_stack_region_t, 1);
  region->address = stack;
  region->process_name = strdup(name);
  region->context = context;
  region->size = size;
  region->block = ((char*)stack - (char*)((xbt_mheap_t)std_heap)->heapbase) / BLOCKSIZE + 1;
  xbt_dynar_push(stacks_areas, &region);

  if(!raw_mem_set)
    MC_UNSET_RAW_MEM;
}

void MC_ignore(void *addr, size_t size){

  int raw_mem_set= (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  if(mc_checkpoint_ignore == NULL)
    mc_checkpoint_ignore = xbt_dynar_new(sizeof(mc_checkpoint_ignore_region_t), checkpoint_ignore_region_free_voidp);

  mc_checkpoint_ignore_region_t region = xbt_new0(s_mc_checkpoint_ignore_region_t, 1);
  region->addr = addr;
  region->size = size;

  if(xbt_dynar_is_empty(mc_checkpoint_ignore)){
    xbt_dynar_push(mc_checkpoint_ignore, &region);
  }else{
     
    unsigned int cursor = 0;
    int start = 0;
    int end = xbt_dynar_length(mc_checkpoint_ignore) -1;
    mc_checkpoint_ignore_region_t current_region = NULL;

    while(start <= end){
      cursor = (start + end) / 2;
      current_region = (mc_checkpoint_ignore_region_t)xbt_dynar_get_as(mc_checkpoint_ignore, cursor, mc_checkpoint_ignore_region_t);
      if(current_region->addr == addr){
        if(current_region->size == size){
          checkpoint_ignore_region_free(region);
          if(!raw_mem_set)
            MC_UNSET_RAW_MEM;
          return;
        }else if(current_region->size < size){
          start = cursor + 1;
        }else{
          end = cursor - 1;
        }
      }else if(current_region->addr < addr){
          start = cursor + 1;
      }else{
        end = cursor - 1;
      }
    }

     if(current_region->addr == addr){
       if(current_region->size < size){
        xbt_dynar_insert_at(mc_checkpoint_ignore, cursor + 1, &region);
      }else{
        xbt_dynar_insert_at(mc_checkpoint_ignore, cursor, &region);
      }
    }else if(current_region->addr < addr){
       xbt_dynar_insert_at(mc_checkpoint_ignore, cursor + 1, &region);
    }else{
       xbt_dynar_insert_at(mc_checkpoint_ignore, cursor, &region);
    }
  }

  if(!raw_mem_set)
    MC_UNSET_RAW_MEM;
}

/*******************************  Initialisation of MC *******************************/
/*********************************************************************************/

static void MC_post_process_object_info(mc_object_info_t info) {
  xbt_dict_cursor_t cursor = NULL;
  char* key = NULL;
  dw_type_t type = NULL;
  xbt_dict_foreach(info->types, cursor, key, type){

    // Resolve full_type:
    if(type->name && type->byte_size == 0) {
      for(size_t i=0; i!=mc_object_infos_size; ++i) {
        dw_type_t same_type =  xbt_dict_get_or_null(mc_object_infos[i]->full_types_by_name, type->name);
        if(same_type && same_type->name && same_type->byte_size) {
          type->full_type = same_type;
          break;
        }
      }
    }

  }
}

static void MC_init_debug_info(void) {
  XBT_INFO("Get debug information ...");

  memory_map_t maps = MC_get_memory_map();

  /* Get local variables for state equality detection */
  mc_binary_info = MC_find_object_info(maps, xbt_binary_name, 1);
  mc_object_infos[0] = mc_binary_info;

  mc_libsimgrid_info = MC_find_object_info(maps, libsimgrid_path, 0);
  mc_object_infos[1] = mc_libsimgrid_info;

  // Use information of the other objects:
  MC_post_process_object_info(mc_binary_info);
  MC_post_process_object_info(mc_libsimgrid_info);

  MC_free_memory_map(maps);
  XBT_INFO("Get debug information done !");
}

void MC_init(){

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  compare = 0;

  /* Initialize the data structures that must be persistent across every
     iteration of the model-checker (in RAW memory) */

  MC_SET_RAW_MEM;

  MC_init_memory_map_info();
  MC_init_debug_info();

   /* Init parmap */
  parmap = xbt_parmap_mc_new(xbt_os_get_numcores(), XBT_PARMAP_DEFAULT);

  MC_UNSET_RAW_MEM;

   /* Ignore some variables from xbt/ex.h used by exception e for stacks comparison */
  MC_ignore_local_variable("e", "*");
  MC_ignore_local_variable("__ex_cleanup", "*");
  MC_ignore_local_variable("__ex_mctx_en", "*");
  MC_ignore_local_variable("__ex_mctx_me", "*");
  MC_ignore_local_variable("__xbt_ex_ctx_ptr", "*");
  MC_ignore_local_variable("_log_ev", "*");
  MC_ignore_local_variable("_throw_ctx", "*");
  MC_ignore_local_variable("ctx", "*");

  MC_ignore_local_variable("self", "simcall_BODY_mc_snapshot");
  MC_ignore_local_variable("next_context", "smx_ctx_sysv_suspend_serial");
  MC_ignore_local_variable("i", "smx_ctx_sysv_suspend_serial");

  /* Ignore local variable about time used for tracing */
  MC_ignore_local_variable("start_time", "*"); 

  MC_ignore_global_variable("compared_pointers");
  MC_ignore_global_variable("mc_comp_times");
  MC_ignore_global_variable("mc_snapshot_comparison_time"); 
  MC_ignore_global_variable("mc_time");
  MC_ignore_global_variable("smpi_current_rank");
  MC_ignore_global_variable("counter"); /* Static variable used for tracing */
  MC_ignore_global_variable("maestro_stack_start");
  MC_ignore_global_variable("maestro_stack_end");
  MC_ignore_global_variable("smx_total_comms");

  MC_ignore_heap(&(simix_global->process_to_run), sizeof(simix_global->process_to_run));
  MC_ignore_heap(&(simix_global->process_that_ran), sizeof(simix_global->process_that_ran));
  MC_ignore_heap(simix_global->process_to_run, sizeof(*(simix_global->process_to_run)));
  MC_ignore_heap(simix_global->process_that_ran, sizeof(*(simix_global->process_that_ran)));
  
  smx_process_t process;
  xbt_swag_foreach(process, simix_global->process_list){
    MC_ignore_heap(&(process->process_hookup), sizeof(process->process_hookup));
  }

  if(raw_mem_set)
    MC_SET_RAW_MEM;

}

static void MC_init_dot_output(){ /* FIXME : more colors */

  colors[0] = "blue";
  colors[1] = "red";
  colors[2] = "green3";
  colors[3] = "goldenrod";
  colors[4] = "brown";
  colors[5] = "purple";
  colors[6] = "magenta";
  colors[7] = "turquoise4";
  colors[8] = "gray25";
  colors[9] = "forestgreen";
  colors[10] = "hotpink";
  colors[11] = "lightblue";
  colors[12] = "tan";

  dot_output = fopen(_sg_mc_dot_output_file, "w");
  
  if(dot_output == NULL){
    perror("Error open dot output file");
    xbt_abort();
  }

  fprintf(dot_output, "digraph graphname{\n fixedsize=true; rankdir=TB; ranksep=.25; edge [fontsize=12]; node [fontsize=10, shape=circle,width=.5 ]; graph [resolution=20, fontsize=10];\n");

}

/*******************************  Core of MC *******************************/
/**************************************************************************/

void MC_do_the_modelcheck_for_real() {

  MC_SET_RAW_MEM;
  mc_comp_times = xbt_new0(s_mc_comparison_times_t, 1);
  MC_UNSET_RAW_MEM;
  
  if (!_sg_mc_property_file || _sg_mc_property_file[0]=='\0') {
    if (mc_reduce_kind==e_mc_reduce_unset)
      mc_reduce_kind=e_mc_reduce_dpor;

    XBT_INFO("Check a safety property");
    MC_modelcheck_safety();

  } else  {

    if (mc_reduce_kind==e_mc_reduce_unset)
      mc_reduce_kind=e_mc_reduce_none;

    XBT_INFO("Check the liveness property %s",_sg_mc_property_file);
    MC_automaton_load(_sg_mc_property_file);
    MC_modelcheck_liveness();
  }
}

void MC_modelcheck_safety(void)
{
  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  /* Check if MC is already initialized */
  if (initial_state_safety)
    return;

  mc_time = xbt_new0(double, simix_process_maxpid);

  /* mc_time refers to clock for each process -> ignore it for heap comparison */  
  MC_ignore_heap(mc_time, simix_process_maxpid * sizeof(double));

  /* Initialize the data structures that must be persistent across every
     iteration of the model-checker (in RAW memory) */
  
  MC_SET_RAW_MEM;

  /* Initialize statistics */
  mc_stats = xbt_new0(s_mc_stats_t, 1);
  mc_stats->state_size = 1;

  /* Create exploration stack */
  mc_stack_safety = xbt_fifo_new();

  if((_sg_mc_dot_output_file != NULL) && (_sg_mc_dot_output_file[0]!='\0'))
    MC_init_dot_output();

  MC_UNSET_RAW_MEM;

  if(_sg_mc_visited > 0){
    MC_init();
  }else{
    MC_SET_RAW_MEM;
    MC_init_memory_map_info();
    MC_init_debug_info();
    MC_UNSET_RAW_MEM;
  }

  MC_dpor_init();

  MC_SET_RAW_MEM;
  /* Save the initial state */
  initial_state_safety = xbt_new0(s_mc_global_t, 1);
  initial_state_safety->snapshot = MC_take_snapshot(0);
  initial_state_safety->initial_communications_pattern_done = 0;
  initial_state_safety->comm_deterministic = 1;
  initial_state_safety->send_deterministic = 1;
  MC_UNSET_RAW_MEM;

  MC_dpor();

  if(raw_mem_set)
    MC_SET_RAW_MEM;

  xbt_abort();
  //MC_exit();
}

void MC_modelcheck_liveness(){

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_init();

  mc_time = xbt_new0(double, simix_process_maxpid);

  /* mc_time refers to clock for each process -> ignore it for heap comparison */  
  MC_ignore_heap(mc_time, simix_process_maxpid * sizeof(double));
 
  MC_SET_RAW_MEM;
 
  /* Initialize statistics */
  mc_stats = xbt_new0(s_mc_stats_t, 1);
  mc_stats->state_size = 1;

  /* Create exploration stack */
  mc_stack_liveness = xbt_fifo_new();

  /* Create the initial state */
  initial_state_liveness = xbt_new0(s_mc_global_t, 1);

  if((_sg_mc_dot_output_file != NULL) && (_sg_mc_dot_output_file[0]!='\0'))
    MC_init_dot_output();
  
  MC_UNSET_RAW_MEM;

  MC_ddfs_init();

  /* We're done */
  MC_print_statistics(mc_stats);
  xbt_free(mc_time);

  if(raw_mem_set)
    MC_SET_RAW_MEM;

}


void MC_exit(void)
{
  xbt_free(mc_time);

  MC_memory_exit();
  //xbt_abort();
}

int SIMIX_pre_mc_random(smx_simcall_t simcall, int min, int max){

  return simcall->mc_value;
}


int MC_random(int min, int max)
{
  /*FIXME: return mc_current_state->executed_transition->random.value;*/
  return simcall_mc_random(min, max);
}

/**
 * \brief Schedules all the process that are ready to run
 */
void MC_wait_for_requests(void)
{
  smx_process_t process;
  smx_simcall_t req;
  unsigned int iter;

  while (!xbt_dynar_is_empty(simix_global->process_to_run)) {
    SIMIX_process_runall();
    xbt_dynar_foreach(simix_global->process_that_ran, iter, process) {
      req = &process->simcall;
      if (req->call != SIMCALL_NONE && !MC_request_is_visible(req))
        SIMIX_simcall_pre(req, 0);
    }
  }
}

int MC_deadlock_check()
{
  int deadlock = FALSE;
  smx_process_t process;
  if(xbt_swag_size(simix_global->process_list)){
    deadlock = TRUE;
    xbt_swag_foreach(process, simix_global->process_list){
      if(process->simcall.call != SIMCALL_NONE
         && MC_request_is_enabled(&process->simcall)){
        deadlock = FALSE;
        break;
      }
    }
  }
  return deadlock;
}

/**
 * \brief Re-executes from the state at position start all the transitions indicated by
 *        a given model-checker stack.
 * \param stack The stack with the transitions to execute.
 * \param start Start index to begin the re-execution.
 */
void MC_replay(xbt_fifo_t stack, int start)
{
  int raw_mem = (mmalloc_get_current_heap() == raw_heap);

  int value, i = 1, count = 1;
  char *req_str;
  smx_simcall_t req = NULL, saved_req = NULL;
  xbt_fifo_item_t item, start_item;
  mc_state_t state;
  smx_process_t process = NULL;
  int comm_pattern = 0;

  XBT_DEBUG("**** Begin Replay ****");

  if(start == -1){
    /* Restore the initial state */
    MC_restore_snapshot(initial_state_safety->snapshot);
    /* At the moment of taking the snapshot the raw heap was set, so restoring
     * it will set it back again, we have to unset it to continue  */
    MC_UNSET_RAW_MEM;
  }

  start_item = xbt_fifo_get_last_item(stack);
  if(start != -1){
    while (i != start){
      start_item = xbt_fifo_get_prev_item(start_item);
      i++;
    }
  }

  MC_SET_RAW_MEM;

  if(mc_reduce_kind ==  e_mc_reduce_dpor){
    xbt_dict_reset(first_enabled_state);
    xbt_swag_foreach(process, simix_global->process_list){
      if(MC_process_is_enabled(process)){
      char *key = bprintf("%lu", process->pid);
      char *data = bprintf("%d", count);
      xbt_dict_set(first_enabled_state, key, data, NULL);
      xbt_free(key);
      }
    }
  }

  if(_sg_mc_comms_determinism || _sg_mc_send_determinism){
    xbt_dynar_reset(communications_pattern);
    xbt_dynar_reset(incomplete_communications_pattern);
  }

  MC_UNSET_RAW_MEM;
  

  /* Traverse the stack from the state at position start and re-execute the transitions */
  for (item = start_item;
       item != xbt_fifo_get_first_item(stack);
       item = xbt_fifo_get_prev_item(item)) {

    state = (mc_state_t) xbt_fifo_get_item_content(item);
    saved_req = MC_state_get_executed_request(state, &value);
   
    if(mc_reduce_kind ==  e_mc_reduce_dpor){
      MC_SET_RAW_MEM;
      char *key = bprintf("%lu", saved_req->issuer->pid);
      xbt_dict_remove(first_enabled_state, key); 
      xbt_free(key);
      MC_UNSET_RAW_MEM;
    }
   
    if(saved_req){
      /* because we got a copy of the executed request, we have to fetch the  
         real one, pointed by the request field of the issuer process */
      req = &saved_req->issuer->simcall;

      /* Debug information */
      if(XBT_LOG_ISENABLED(mc_global, xbt_log_priority_debug)){
        req_str = MC_request_to_string(req, value);
        XBT_DEBUG("Replay: %s (%p)", req_str, state);
        xbt_free(req_str);
      }
    }

    if(_sg_mc_comms_determinism || _sg_mc_send_determinism){
      if(req->call == SIMCALL_COMM_ISEND)
        comm_pattern = 1;
      else if(req->call == SIMCALL_COMM_IRECV)
        comm_pattern = 2;
    }

    SIMIX_simcall_pre(req, value);

    if(_sg_mc_comms_determinism || _sg_mc_send_determinism){
      MC_SET_RAW_MEM;
      if(comm_pattern != 0){
        get_comm_pattern(communications_pattern, req, comm_pattern);
      }
      MC_UNSET_RAW_MEM;
      comm_pattern = 0;
    }
    
    MC_wait_for_requests();

    count++;

    if(mc_reduce_kind ==  e_mc_reduce_dpor){
      MC_SET_RAW_MEM;
      /* Insert in dict all enabled processes */
      xbt_swag_foreach(process, simix_global->process_list){
        if(MC_process_is_enabled(process) /*&& !MC_state_process_is_done(state, process)*/){
          char *key = bprintf("%lu", process->pid);
          if(xbt_dict_get_or_null(first_enabled_state, key) == NULL){
            char *data = bprintf("%d", count);
            xbt_dict_set(first_enabled_state, key, data, NULL);
          }
          xbt_free(key);
        }
      }
      MC_UNSET_RAW_MEM;
    }
         
    /* Update statistics */
    mc_stats->visited_states++;
    mc_stats->executed_transitions++;

  }

  XBT_DEBUG("**** End Replay ****");

  if(raw_mem)
    MC_SET_RAW_MEM;
  else
    MC_UNSET_RAW_MEM;
  

}

void MC_replay_liveness(xbt_fifo_t stack, int all_stack)
{

  initial_state_liveness->raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  int value;
  char *req_str;
  smx_simcall_t req = NULL, saved_req = NULL;
  xbt_fifo_item_t item;
  mc_state_t state;
  mc_pair_t pair;
  int depth = 1;

  XBT_DEBUG("**** Begin Replay ****");

  /* Restore the initial state */
  MC_restore_snapshot(initial_state_liveness->snapshot);

  /* At the moment of taking the snapshot the raw heap was set, so restoring
   * it will set it back again, we have to unset it to continue  */
  if(!initial_state_liveness->raw_mem_set)
    MC_UNSET_RAW_MEM;

  if(all_stack){

    item = xbt_fifo_get_last_item(stack);

    while(depth <= xbt_fifo_size(stack)){

      pair = (mc_pair_t) xbt_fifo_get_item_content(item);
      state = (mc_state_t) pair->graph_state;

      if(pair->requests > 0){
   
        saved_req = MC_state_get_executed_request(state, &value);
        //XBT_DEBUG("SavedReq->call %u", saved_req->call);
      
        if(saved_req != NULL){
          /* because we got a copy of the executed request, we have to fetch the  
             real one, pointed by the request field of the issuer process */
          req = &saved_req->issuer->simcall;
          //XBT_DEBUG("Req->call %u", req->call);
  
          /* Debug information */
          if(XBT_LOG_ISENABLED(mc_global, xbt_log_priority_debug)){
            req_str = MC_request_to_string(req, value);
            XBT_DEBUG("Replay (depth = %d) : %s (%p)", depth, req_str, state);
            xbt_free(req_str);
          }
  
        }
 
        SIMIX_simcall_pre(req, value);
        MC_wait_for_requests();
      }

      depth++;
    
      /* Update statistics */
      mc_stats->visited_pairs++;
      mc_stats->executed_transitions++;

      item = xbt_fifo_get_prev_item(item);
    }

  }else{

    /* Traverse the stack from the initial state and re-execute the transitions */
    for (item = xbt_fifo_get_last_item(stack);
         item != xbt_fifo_get_first_item(stack);
         item = xbt_fifo_get_prev_item(item)) {

      pair = (mc_pair_t) xbt_fifo_get_item_content(item);
      state = (mc_state_t) pair->graph_state;

      if(pair->requests > 0){
   
        saved_req = MC_state_get_executed_request(state, &value);
        //XBT_DEBUG("SavedReq->call %u", saved_req->call);
      
        if(saved_req != NULL){
          /* because we got a copy of the executed request, we have to fetch the  
             real one, pointed by the request field of the issuer process */
          req = &saved_req->issuer->simcall;
          //XBT_DEBUG("Req->call %u", req->call);
  
          /* Debug information */
          if(XBT_LOG_ISENABLED(mc_global, xbt_log_priority_debug)){
            req_str = MC_request_to_string(req, value);
            XBT_DEBUG("Replay (depth = %d) : %s (%p)", depth, req_str, state);
            xbt_free(req_str);
          }
  
        }
 
        SIMIX_simcall_pre(req, value);
        MC_wait_for_requests();
      }

      depth++;
    
      /* Update statistics */
      mc_stats->visited_pairs++;
      mc_stats->executed_transitions++;
    }
  }  

  XBT_DEBUG("**** End Replay ****");

  if(initial_state_liveness->raw_mem_set)
    MC_SET_RAW_MEM;
  else
    MC_UNSET_RAW_MEM;
  
}

/**
 * \brief Dumps the contents of a model-checker's stack and shows the actual
 *        execution trace
 * \param stack The stack to dump
 */
void MC_dump_stack_safety(xbt_fifo_t stack)
{
  
  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_show_stack_safety(stack);

  if(!_sg_mc_checkpoint){

    mc_state_t state;

    MC_SET_RAW_MEM;
    while ((state = (mc_state_t) xbt_fifo_pop(stack)) != NULL)
      MC_state_delete(state);
    MC_UNSET_RAW_MEM;

  }

  if(raw_mem_set)
    MC_SET_RAW_MEM;
  else
    MC_UNSET_RAW_MEM;
  
}


void MC_show_stack_safety(xbt_fifo_t stack)
{

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  int value;
  mc_state_t state;
  xbt_fifo_item_t item;
  smx_simcall_t req;
  char *req_str = NULL;
  
  for (item = xbt_fifo_get_last_item(stack);
       (item ? (state = (mc_state_t) (xbt_fifo_get_item_content(item)))
        : (NULL)); item = xbt_fifo_get_prev_item(item)) {
    req = MC_state_get_executed_request(state, &value);
    if(req){
      req_str = MC_request_to_string(req, value);
      XBT_INFO("%s", req_str);
      xbt_free(req_str);
    }
  }

  if(!raw_mem_set)
    MC_UNSET_RAW_MEM;
}

void MC_show_deadlock(smx_simcall_t req)
{
  /*char *req_str = NULL;*/
  XBT_INFO("**************************");
  XBT_INFO("*** DEAD-LOCK DETECTED ***");
  XBT_INFO("**************************");
  XBT_INFO("Locked request:");
  /*req_str = MC_request_to_string(req);
    XBT_INFO("%s", req_str);
    xbt_free(req_str);*/
  XBT_INFO("Counter-example execution trace:");
  MC_dump_stack_safety(mc_stack_safety);
  MC_print_statistics(mc_stats);
}


void MC_show_stack_liveness(xbt_fifo_t stack){
  int value;
  mc_pair_t pair;
  xbt_fifo_item_t item;
  smx_simcall_t req;
  char *req_str = NULL;
  
  for (item = xbt_fifo_get_last_item(stack);
       (item ? (pair = (mc_pair_t) (xbt_fifo_get_item_content(item)))
        : (NULL)); item = xbt_fifo_get_prev_item(item)) {
    req = MC_state_get_executed_request(pair->graph_state, &value);
    if(req){
      if(pair->requests>0){
        req_str = MC_request_to_string(req, value);
        XBT_INFO("%s", req_str);
        xbt_free(req_str);
      }else{
        XBT_INFO("End of system requests but evolution in Büchi automaton");
      }
    }
  }
}

void MC_dump_stack_liveness(xbt_fifo_t stack){

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  mc_pair_t pair;

  MC_SET_RAW_MEM;
  while ((pair = (mc_pair_t) xbt_fifo_pop(stack)) != NULL)
    MC_pair_delete(pair);
  MC_UNSET_RAW_MEM;

  if(raw_mem_set)
    MC_SET_RAW_MEM;

}


void MC_print_statistics(mc_stats_t stats)
{
  if(stats->expanded_pairs == 0){
    XBT_INFO("Expanded states = %lu", stats->expanded_states);
    XBT_INFO("Visited states = %lu", stats->visited_states);
  }else{
    XBT_INFO("Expanded pairs = %lu", stats->expanded_pairs);
    XBT_INFO("Visited pairs = %lu", stats->visited_pairs);
  }
  XBT_INFO("Executed transitions = %lu", stats->executed_transitions);
  MC_SET_RAW_MEM;
  if((_sg_mc_dot_output_file != NULL) && (_sg_mc_dot_output_file[0]!='\0')){
    fprintf(dot_output, "}\n");
    fclose(dot_output);
  }
  if(initial_state_safety != NULL){
    if(_sg_mc_comms_determinism)
      XBT_INFO("Communication-deterministic : %s", !initial_state_safety->comm_deterministic ? "No" : "Yes");
    if (_sg_mc_send_determinism)
      XBT_INFO("Send-deterministic : %s", !initial_state_safety->send_deterministic ? "No" : "Yes");
  }
  MC_UNSET_RAW_MEM;
}

void MC_assert(int prop)
{
  if (MC_is_active() && !prop){
    XBT_INFO("**************************");
    XBT_INFO("*** PROPERTY NOT VALID ***");
    XBT_INFO("**************************");
    XBT_INFO("Counter-example execution trace:");
    MC_dump_stack_safety(mc_stack_safety);
    MC_print_statistics(mc_stats);
    xbt_abort();
  }
}

void MC_cut(void){
  user_max_depth_reached = 1;
}

void MC_process_clock_add(smx_process_t process, double amount)
{
  mc_time[process->pid] += amount;
}

double MC_process_clock_get(smx_process_t process)
{
  if(mc_time){
    if(process != NULL)
      return mc_time[process->pid];
    else 
      return -1;
  }else{
    return 0;
  }
}

void MC_automaton_load(const char *file){

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  if (_mc_property_automaton == NULL)
    _mc_property_automaton = xbt_automaton_new();
  
  xbt_automaton_load(_mc_property_automaton,file);

  MC_UNSET_RAW_MEM;

  if(raw_mem_set)
    MC_SET_RAW_MEM;

}

void MC_automaton_new_propositional_symbol(const char* id, void* fct) {

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  if (_mc_property_automaton == NULL)
    _mc_property_automaton = xbt_automaton_new();

  xbt_automaton_propositional_symbol_new(_mc_property_automaton,id,fct);

  MC_UNSET_RAW_MEM;

  if(raw_mem_set)
    MC_SET_RAW_MEM;
  
}




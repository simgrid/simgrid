/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define _GNU_SOURCE
#define UNW_LOCAL_ONLY

#include <string.h>
#include <link.h>
#include "mc_private.h"
#include "xbt/module.h"
#include <xbt/mmalloc.h>
#include "../smpi/private.h"

#include "xbt/mmalloc/mmprivate.h"

#include "../simix/smx_private.h"

#include <libunwind.h>
#include <libelf.h>

#include "mc_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_checkpoint, mc,
                                "Logging specific to mc_checkpoint");

char *libsimgrid_path;

/************************************  Free functions **************************************/
/*****************************************************************************************/

static void MC_snapshot_stack_free(mc_snapshot_stack_t s){
  if(s){
    xbt_dynar_free(&(s->local_variables));
    xbt_dynar_free(&(s->stack_frames));
    xbt_free(s);
  }
}

static void MC_snapshot_stack_free_voidp(void *s){
  MC_snapshot_stack_free((mc_snapshot_stack_t) * (void **) s);
}

static void local_variable_free(local_variable_t v){
  xbt_free(v->name);
  xbt_free(v);
}

static void local_variable_free_voidp(void *v){
  local_variable_free((local_variable_t) * (void **) v);
}

static void MC_region_destroy(mc_mem_region_t reg)
{
  xbt_free(reg->data);
  xbt_free(reg);
}

void MC_free_snapshot(mc_snapshot_t snapshot){
  unsigned int i;
  for(i=0; i < NB_REGIONS; i++)
    MC_region_destroy(snapshot->regions[i]);

  xbt_free(snapshot->stack_sizes);
  xbt_dynar_free(&(snapshot->stacks));
  xbt_dynar_free(&(snapshot->to_ignore));

  size_t n = snapshot->nb_processes;
  if(snapshot->privatization_regions) {
    for(i=0; i!=n; ++i) {
      MC_region_destroy(snapshot->privatization_regions[i]);
    }
    xbt_free(snapshot->privatization_regions);
  }

  xbt_free(snapshot);
}


/*******************************  Snapshot regions ********************************/
/*********************************************************************************/

static mc_mem_region_t MC_region_new(int type, void *start_addr, size_t size)
{
  mc_mem_region_t new_reg = xbt_new(s_mc_mem_region_t, 1);
  new_reg->start_addr = start_addr;
  new_reg->size = size;
  new_reg->data = xbt_malloc(size);
  memcpy(new_reg->data, start_addr, size);

  XBT_DEBUG("New region : type : %d, data : %p (real addr %p), size : %zu", type, new_reg->data, start_addr, size);
  
  return new_reg;
}

static void MC_region_restore(mc_mem_region_t reg)
{
  /*FIXME: check if start_addr is still mapped, if it is not, then map it
    before copying the data */
 
  memcpy(reg->start_addr, reg->data, reg->size);
  return;
}

static void MC_snapshot_add_region(mc_snapshot_t snapshot, int type, void *start_addr, size_t size)
{
  mc_mem_region_t new_reg = MC_region_new(type, start_addr, size);
  snapshot->regions[type] = new_reg;
  return;
} 

static void MC_get_memory_regions(mc_snapshot_t snapshot){
  size_t i;

  void* start_heap = ((xbt_mheap_t)std_heap)->base;
  void* end_heap   = ((xbt_mheap_t)std_heap)->breakval;
  MC_snapshot_add_region(snapshot, 0, start_heap, (char*) end_heap - (char*) start_heap);
  snapshot->heap_bytes_used = mmalloc_get_bytes_used(std_heap);

  MC_snapshot_add_region(snapshot, 1,  mc_libsimgrid_info->start_rw, mc_libsimgrid_info->end_rw - mc_libsimgrid_info->start_rw);
  if(!smpi_privatize_global_variables) {
    MC_snapshot_add_region(snapshot, 2,  mc_binary_info->start_rw, mc_binary_info->end_rw - mc_binary_info->start_rw);
    snapshot->privatization_regions = NULL;
    snapshot->privatization_index = -1;
  } else {
    snapshot->privatization_regions = xbt_new(mc_mem_region_t, SIMIX_process_count());
    for (i=0; i< SIMIX_process_count(); i++){
      snapshot->privatization_regions[i] = MC_region_new(-1, mappings[i], size_data_exe);
    }
    snapshot->privatization_index = loaded_page;
  }
}

/** @brief Finds the range of the different memory segments and binary paths */
void MC_init_memory_map_info(){
 
  unsigned int i = 0;
  s_map_region_t reg;
  memory_map_t maps = MC_get_memory_map();

  maestro_stack_start = NULL;
  maestro_stack_end = NULL;
  libsimgrid_path = NULL;

  while (i < maps->mapsize) {
    reg = maps->regions[i];
    if (maps->regions[i].pathname == NULL) {
      // Nothing to do
    }
    else if ((reg.prot & PROT_WRITE) && !memcmp(maps->regions[i].pathname, "[stack]", 7)){
          maestro_stack_start = reg.start_addr;
          maestro_stack_end = reg.end_addr;
    } else if ((reg.prot & PROT_READ) && (reg.prot & PROT_EXEC) && !memcmp(basename(maps->regions[i].pathname), "libsimgrid", 10)){
      if(libsimgrid_path == NULL)
          libsimgrid_path = strdup(maps->regions[i].pathname);
    }
    i++;
  }

  xbt_assert(maestro_stack_start, "maestro_stack_start");
  xbt_assert(maestro_stack_end, "maestro_stack_end");
  xbt_assert(libsimgrid_path, "libsimgrid_path&");

  MC_free_memory_map(maps);

}

/** \brief Fill/lookup the "subtype" field.
 */
static void MC_resolve_subtype(mc_object_info_t info, dw_type_t type) {

  if(type->dw_type_id==NULL)
    return;
  type->subtype = xbt_dict_get_or_null(info->types, type->dw_type_id);
  if(type->subtype==NULL)
    return;
  if(type->subtype->byte_size != 0)
    return;
  if(type->subtype->name==NULL)
    return;
  // Try to find a more complete description of the type:
  // We need to fix in order to support C++.

  dw_type_t subtype = xbt_dict_get_or_null(info->full_types_by_name, type->subtype->name);
  if(subtype!=NULL) {
    type->subtype = subtype;
  }

}

void MC_post_process_types(mc_object_info_t info) {
  xbt_dict_cursor_t cursor = NULL;
  char *origin;
  dw_type_t type;

  // Lookup "subtype" field:
  xbt_dict_foreach(info->types, cursor, origin, type){
    MC_resolve_subtype(info, type);

    dw_type_t member;
    unsigned int i = 0;
    if(type->members!=NULL) xbt_dynar_foreach(type->members, i, member) {
      MC_resolve_subtype(info, member);
    }
  }
}

/** \brief Fills the position of the segments (executable, read-only, read/write).
 *
 * TODO, use dl_iterate_phdr to be more robust
 * */
void MC_find_object_address(memory_map_t maps, mc_object_info_t result) {

  unsigned int i = 0;
  s_map_region_t reg;
  const char* name = basename(result->file_name);
  while (i < maps->mapsize) {
    reg = maps->regions[i];
    if (maps->regions[i].pathname == NULL || strcmp(basename(maps->regions[i].pathname),  name)) {
      // Nothing to do
    }
    else if ((reg.prot & PROT_WRITE)){
          xbt_assert(!result->start_rw,
            "Multiple read-write segments for %s, not supported",
            maps->regions[i].pathname);
          result->start_rw = reg.start_addr;
          result->end_rw   = reg.end_addr;
          // .bss is usually after the .data:
          s_map_region_t* next = &(maps->regions[i+1]);
          if(next->pathname == NULL && (next->prot & PROT_WRITE) && next->start_addr == reg.end_addr) {
            result->end_rw = maps->regions[i+1].end_addr;
          }
    } else if ((reg.prot & PROT_READ) && (reg.prot & PROT_EXEC)){
          xbt_assert(!result->start_exec,
            "Multiple executable segments for %s, not supported",
            maps->regions[i].pathname);
          result->start_exec = reg.start_addr;
          result->end_exec   = reg.end_addr;
    }
    else if((reg.prot & PROT_READ) && !(reg.prot & PROT_EXEC)) {
        xbt_assert(!result->start_ro,
          "Multiple read only segments for %s, not supported",
          maps->regions[i].pathname);
        result->start_ro = reg.start_addr;
        result->end_ro   = reg.end_addr;
    }
    i++;
  }

  xbt_assert(result->file_name);
  xbt_assert(result->start_rw);
  xbt_assert(result->start_exec);
}

/************************************* Take Snapshot ************************************/
/****************************************************************************************/

/** \brief Checks whether the variable is in scope for a given IP.
 *
 *  A variable may be defined only from a given value of IP.
 *
 *  \param var   Variable description
 *  \param frame Scope description
 *  \param ip    Instruction pointer
 *  \return      true if the variable is valid
 * */
static bool mc_valid_variable(dw_variable_t var, dw_frame_t frame, const void* ip) {
  // The variable is not yet valid:
  if((const void*)((const char*) frame->low_pc + var->start_scope) > ip)
    return false;
  else
    return true;
}

static void mc_fill_local_variables_values(mc_stack_frame_t stack_frame, dw_frame_t scope, xbt_dynar_t result) {
  void* ip = (void*) stack_frame->ip;
  if(ip < scope->low_pc || ip>= scope->high_pc)
    return;

  unsigned cursor = 0;
  dw_variable_t current_variable;
  xbt_dynar_foreach(scope->variables, cursor, current_variable){

    if(!mc_valid_variable(current_variable, stack_frame->frame, (void*) stack_frame->ip))
      continue;

    int region_type;
    if((long)stack_frame->ip > (long)mc_libsimgrid_info->start_exec)
      region_type = 1;
    else
      region_type = 2;

    local_variable_t new_var = xbt_new0(s_local_variable_t, 1);
    new_var->subprogram = stack_frame->frame;
    new_var->ip = stack_frame->ip;
    new_var->name = xbt_strdup(current_variable->name);
    new_var->type = current_variable->type;
    new_var->region= region_type;

    /* if(current_variable->address!=NULL) {
      new_var->address = current_variable->address;
    } else */
    if(current_variable->locations.size != 0){
      new_var->address = (void*) mc_dwarf_resolve_locations(&current_variable->locations,
        current_variable->object_info,
        &(stack_frame->unw_cursor), (void*)stack_frame->frame_base, NULL);
    }

    xbt_dynar_push(result, &new_var);
  }

  // Recursive processing of nested scopes:
  dw_frame_t nested_scope = NULL;
  xbt_dynar_foreach(scope->scopes, cursor, nested_scope) {
    mc_fill_local_variables_values(stack_frame, nested_scope, result);
  }
}

static xbt_dynar_t MC_get_local_variables_values(xbt_dynar_t stack_frames){

  unsigned cursor1 = 0;
  mc_stack_frame_t stack_frame;
  xbt_dynar_t variables = xbt_dynar_new(sizeof(local_variable_t), local_variable_free_voidp);

  xbt_dynar_foreach(stack_frames,cursor1,stack_frame) {
    mc_fill_local_variables_values(stack_frame, stack_frame->frame, variables);
  }

  return variables;
}

static void MC_stack_frame_free_voipd(void *s){
  mc_stack_frame_t stack_frame = *(mc_stack_frame_t*)s;
  if(stack_frame) {
    xbt_free(stack_frame->frame_name);
    xbt_free(stack_frame);
  }
}

static xbt_dynar_t MC_unwind_stack_frames(void *stack_context) {
  xbt_dynar_t result = xbt_dynar_new(sizeof(mc_stack_frame_t), MC_stack_frame_free_voipd);

  unw_cursor_t c;

  // TODO, check condition check (unw_init_local==0 means end of frame)
  if(unw_init_local(&c, (unw_context_t *)stack_context)!=0) {

    xbt_die("Could not initialize stack unwinding");

  } else while(1) {

    mc_stack_frame_t stack_frame = xbt_new(s_mc_stack_frame_t, 1);
    xbt_dynar_push(result, &stack_frame);

    stack_frame->unw_cursor = c;

    unw_word_t ip, sp;

    unw_get_reg(&c, UNW_REG_IP, &ip);
    unw_get_reg(&c, UNW_REG_SP, &sp);

    stack_frame->ip = ip;
    stack_frame->sp = sp;

    // TODO, use real addresses in frame_t instead of fixing it here

    dw_frame_t frame = MC_find_function_by_ip((void*) ip);
    stack_frame->frame = frame;

    if(frame) {
      stack_frame->frame_name = xbt_strdup(frame->name);
      stack_frame->frame_base = (unw_word_t)mc_find_frame_base(frame, frame->object_info, &c);
    } else {
      stack_frame->frame_base = 0;
      stack_frame->frame_name = NULL;
    }

    /* Stop before context switch with maestro */
    if(frame!=NULL && frame->name!=NULL && !strcmp(frame->name, "smx_ctx_sysv_wrapper"))
      break;

    int ret = ret = unw_step(&c);
    if(ret==0) {
      xbt_die("Unexpected end of stack.");
    } else if(ret<0) {
      xbt_die("Error while unwinding stack.");
    }
  }

  if(xbt_dynar_length(result) == 0){
    XBT_INFO("unw_init_local failed");
    xbt_abort();
  }

  return result;
};

static xbt_dynar_t MC_take_snapshot_stacks(mc_snapshot_t *snapshot, void *heap){

  xbt_dynar_t res = xbt_dynar_new(sizeof(s_mc_snapshot_stack_t), MC_snapshot_stack_free_voidp);

  unsigned int cursor = 0;
  stack_region_t current_stack;
  
  xbt_dynar_foreach(stacks_areas, cursor, current_stack){
    mc_snapshot_stack_t st = xbt_new(s_mc_snapshot_stack_t, 1);
    st->stack_frames = MC_unwind_stack_frames(current_stack->context);
    st->local_variables = MC_get_local_variables_values(st->stack_frames);

    unw_word_t sp = xbt_dynar_get_as(st->stack_frames, 0, mc_stack_frame_t)->sp;
    st->stack_pointer = ((char *)heap + (size_t)(((char *)((long)sp) - (char*)std_heap)));

    st->real_address = current_stack->address;
    xbt_dynar_push(res, &st);
    (*snapshot)->stack_sizes = xbt_realloc((*snapshot)->stack_sizes, (cursor + 1) * sizeof(size_t));
    (*snapshot)->stack_sizes[cursor] = current_stack->size - ((char *)st->stack_pointer - (char *)((char *)heap + ((char *)current_stack->address - (char *)std_heap)));
  }

  return res;

}

static xbt_dynar_t MC_take_snapshot_ignore(){
  
  if(mc_heap_comparison_ignore == NULL)
    return NULL;

  xbt_dynar_t cpy = xbt_dynar_new(sizeof(mc_heap_ignore_region_t), heap_ignore_region_free_voidp);

  unsigned int cursor = 0;
  mc_heap_ignore_region_t current_region;

  xbt_dynar_foreach(mc_heap_comparison_ignore, cursor, current_region){
    mc_heap_ignore_region_t new_region = NULL;
    new_region = xbt_new0(s_mc_heap_ignore_region_t, 1);
    new_region->address = current_region->address;
    new_region->size = current_region->size;
    new_region->block = current_region->block;
    new_region->fragment = current_region->fragment;
    xbt_dynar_push(cpy, &new_region);
  }

  return cpy;

}

static void MC_dump_checkpoint_ignore(mc_snapshot_t snapshot){
  
  unsigned int cursor = 0;
  mc_checkpoint_ignore_region_t region;
  size_t offset;
  
  xbt_dynar_foreach(mc_checkpoint_ignore, cursor, region){
    if(region->addr > snapshot->regions[0]->start_addr && (char *)(region->addr) < (char *)snapshot->regions[0]->start_addr + STD_HEAP_SIZE){
      offset = (char *)region->addr - (char *)snapshot->regions[0]->start_addr;
      memset((char *)snapshot->regions[0]->data + offset, 0, region->size);
    }else if(region->addr > snapshot->regions[2]->start_addr && (char *)(region->addr) < (char*)snapshot->regions[2]->start_addr + snapshot->regions[2]->size){
      offset = (char *)region->addr - (char *)snapshot->regions[2]->start_addr;
      memset((char *)snapshot->regions[2]->data + offset, 0, region->size);
    }else if(region->addr > snapshot->regions[1]->start_addr && (char *)(region->addr) < (char*)snapshot->regions[1]->start_addr + snapshot->regions[1]->size){
      offset = (char *)region->addr - (char *)snapshot->regions[1]->start_addr;
      memset((char *)snapshot->regions[1]->data + offset, 0, region->size);
    }
  }

}


mc_snapshot_t MC_take_snapshot(int num_state){

  mc_snapshot_t snapshot = xbt_new0(s_mc_snapshot_t, 1);
  snapshot->nb_processes = xbt_swag_size(simix_global->process_list);

  /* Save the std heap and the writable mapped pages of libsimgrid and binary */
  MC_get_memory_regions(snapshot);

  snapshot->to_ignore = MC_take_snapshot_ignore();

  if(_sg_mc_visited > 0 || strcmp(_sg_mc_property_file,"")){
    snapshot->stacks = MC_take_snapshot_stacks(&snapshot, snapshot->regions[0]->data);
    if(_sg_mc_hash && snapshot->stacks!=NULL) {
      snapshot->hash = mc_hash_processes_state(num_state, snapshot->stacks);
    } else {
      snapshot->hash = 0;
    }
  }
  else {
    snapshot->hash = 0;
  }

  if(num_state > 0)
    MC_dump_checkpoint_ignore(snapshot);

  return snapshot;

}

void MC_restore_snapshot(mc_snapshot_t snapshot){
  unsigned int i;
  for(i=0; i < NB_REGIONS; i++){
    // For privatized, variables we decided it was not necessary to take the snapshot:
    if(snapshot->regions[i])
      MC_region_restore(snapshot->regions[i]);
  }

  if(snapshot->privatization_regions) {
    for (i=0; i< SIMIX_process_count(); i++){
      if(snapshot->privatization_regions[i]) {
        MC_region_restore(snapshot->privatization_regions[i]);
      }
    }
    switch_data_segment(snapshot->privatization_index);
  }
}

void* mc_translate_address(uintptr_t addr, mc_snapshot_t snapshot) {

  // If not in a process state/clone:
  if(!snapshot) {
    return (uintptr_t*) addr;
  }

  // If it is in a snapshot:
  for(size_t i=0; i!=NB_REGIONS; ++i) {
    mc_mem_region_t region = snapshot->regions[i];
    uintptr_t start = (uintptr_t) region->start_addr;
    uintptr_t end = start + region->size;

    // The address is in this region:
    if(addr >= start && addr < end) {
      uintptr_t offset = addr - start;
      return (void*) ((uintptr_t)region->data + offset);
    }

  }

  // It is not in a snapshot:
  return (void*) addr;
}

uintptr_t mc_untranslate_address(void* addr, mc_snapshot_t snapshot) {
  if(!snapshot) {
    return (uintptr_t) addr;
  }

  for(size_t i=0; i!=NB_REGIONS; ++i) {
    mc_mem_region_t region = snapshot->regions[i];
    if(addr>=region->data && addr<=(void*)(((char*)region->data)+region->size)) {
      size_t offset = (size_t) ((char*) addr - (char*) region->data);
      return ((uintptr_t) region->start_addr) + offset;
    }
  }

  return (uintptr_t) addr;
}

mc_snapshot_t SIMIX_pre_mc_snapshot(smx_simcall_t simcall){
  return MC_take_snapshot(1);
}

void *MC_snapshot(void){
  return simcall_mc_snapshot();
}

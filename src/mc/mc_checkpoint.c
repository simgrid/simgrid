/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define _GNU_SOURCE
#define UNW_LOCAL_ONLY

#include <unistd.h>

#include <string.h>
#include <link.h>
#include <dirent.h>

#include "internal_config.h"
#include "mc_memory_map.h"
#include "mc_private.h"
#include "xbt/module.h"
#include <xbt/mmalloc.h>
#include "../smpi/private.h"
#include <alloca.h>

#include "xbt/mmalloc/mmprivate.h"

#include "../simix/smx_private.h"

#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <libelf.h>

#include "mc_private.h"
#include <mc/mc.h>

#include "mc_snapshot.h"
#include "mc_object_info.h"
#include "mc_mmu.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_checkpoint, mc,
                                "Logging specific to mc_checkpoint");

/************************************  Free functions **************************************/
/*****************************************************************************************/

static void MC_snapshot_stack_free(mc_snapshot_stack_t s)
{
  if (s) {
    xbt_dynar_free(&(s->local_variables));
    xbt_dynar_free(&(s->stack_frames));
    xbt_free(s);
  }
}

static void MC_snapshot_stack_free_voidp(void *s)
{
  MC_snapshot_stack_free((mc_snapshot_stack_t) * (void **) s);
}

static void local_variable_free(local_variable_t v)
{
  xbt_free(v->name);
  xbt_free(v);
}

static void local_variable_free_voidp(void *v)
{
  local_variable_free((local_variable_t) * (void **) v);
}

void MC_region_destroy(mc_mem_region_t region)
{
  if (!region)
    return;
  switch(region->storage_type) {
    case MC_REGION_STORAGE_TYPE_NONE:
      break;
    case MC_REGION_STORAGE_TYPE_FLAT:
      xbt_free(region->flat.data);
      break;
    case MC_REGION_STORAGE_TYPE_CHUNKED:
      mc_free_page_snapshot_region(region->chunked.page_numbers, mc_page_count(region->size));
      xbt_free(region->chunked.page_numbers);
      break;
    case MC_REGION_STORAGE_TYPE_PRIVATIZED:
      {
        size_t regions_count = region->privatized.regions_count;
        for (size_t i=0; i!=regions_count; ++i) {
          MC_region_destroy(region->privatized.regions[i]);
        }
        free(region->privatized.regions);
        break;
      }
  }
  xbt_free(region);
}

void MC_free_snapshot(mc_snapshot_t snapshot)
{
  for (size_t i = 0; i < snapshot->snapshot_regions_count; i++) {
    MC_region_destroy(snapshot->snapshot_regions[i]);
  }
  xbt_free(snapshot->snapshot_regions);
  xbt_free(snapshot->stack_sizes);
  xbt_dynar_free(&(snapshot->stacks));
  xbt_dynar_free(&(snapshot->to_ignore));
  xbt_dynar_free(&snapshot->ignored_data);
  xbt_free(snapshot);
}

/*******************************  Snapshot regions ********************************/
/*********************************************************************************/

static mc_mem_region_t mc_region_new_dense(
  mc_region_type_t region_type,
  void *start_addr, void* permanent_addr, size_t size, mc_mem_region_t ref_reg)
{
  mc_mem_region_t region = xbt_new(s_mc_mem_region_t, 1);
  region->region_type = region_type;
  region->storage_type = MC_REGION_STORAGE_TYPE_FLAT;
  region->start_addr = start_addr;
  region->permanent_addr = permanent_addr;
  region->size = size;
  region->flat.data = xbt_malloc(size);
  MC_process_read(&mc_model_checker->process, region->flat.data, permanent_addr, size);
  XBT_DEBUG("New region : type : %d, data : %p (real addr %p), size : %zu",
            region_type, region->flat.data, permanent_addr, size);
  return region;
}

/** @brief Take a snapshot of a given region
 *
 * @param type
 * @param start_addr   Address of the region in the simulated process
 * @param permanent_addr Permanent address of this data (for privatized variables, this is the virtual address of the privatized mapping)
 * @param size         Size of the data*
 * @param ref_reg      Reference corresponding region
 */
static mc_mem_region_t MC_region_new(mc_region_type_t type, void *start_addr, void* permanent_addr, size_t size, mc_mem_region_t ref_reg)
{
  if (_sg_mc_sparse_checkpoint) {
    return mc_region_new_sparse(type, start_addr, permanent_addr, size, ref_reg);
  } else  {
    return mc_region_new_dense(type, start_addr, permanent_addr, size, ref_reg);
  }
}

/** @brief Restore a region from a snapshot
 *
 *  If we are using per page snapshots, it is possible to use the reference
 *  region in order to do an incremental restoration of the region: the
 *  softclean pages which are shared between the two snapshots do not need
 *  to be restored.
 *
 *  @param reg     Target region
 *  @param reg_reg Current region (if not NULL), used for lazy per page restoration
 */
static void MC_region_restore(mc_mem_region_t region, mc_mem_region_t ref_region)
{
  switch(region->storage_type) {
  case MC_REGION_STORAGE_TYPE_NONE:
  default:
    xbt_die("Storage type not supported");
    break;

  case MC_REGION_STORAGE_TYPE_FLAT:
    MC_process_write(&mc_model_checker->process, region->flat.data,
      region->permanent_addr, region->size);
    break;

  case MC_REGION_STORAGE_TYPE_CHUNKED:
    mc_region_restore_sparse(&mc_model_checker->process, region, ref_region);
    break;

  case MC_REGION_STORAGE_TYPE_PRIVATIZED:
    {
      bool has_ref_regions = ref_region &&
        ref_region->storage_type == MC_REGION_STORAGE_TYPE_PRIVATIZED;
      size_t process_count = region->privatized.regions_count;
      for (size_t i = 0; i < process_count; i++) {
        MC_region_restore(region->privatized.regions[i],
          has_ref_regions ? ref_region->privatized.regions[i] : NULL);
      }
      break;
    }
  }
}

static inline
void* MC_privatization_address(mc_process_t process, int process_index)
{
  xbt_assert(process_index >= 0);
  return smpi_privatisation_regions[process_index].address;
}

static mc_mem_region_t MC_region_new_privatized(
    mc_region_type_t region_type, void *start_addr, void* permanent_addr, size_t size,
    mc_mem_region_t ref_reg)
{
  size_t process_count = smpi_process_count();
  mc_mem_region_t region = xbt_new(s_mc_mem_region_t, 1);
  region->region_type = region_type;
  region->storage_type = MC_REGION_STORAGE_TYPE_PRIVATIZED;
  region->start_addr = start_addr;
  region->permanent_addr = permanent_addr;
  region->size = size;
  region->privatized.regions_count = process_count;
  region->privatized.regions = xbt_new(mc_mem_region_t, process_count);

  for (size_t i = 0; i < process_count; i++) {
    mc_mem_region_t ref_subreg = NULL;
    if (ref_reg && ref_reg->storage_type == MC_REGION_STORAGE_TYPE_PRIVATIZED)
      ref_subreg = ref_reg->privatized.regions[i];
    region->privatized.regions[i] =
      MC_region_new(region_type, start_addr,
        MC_privatization_address(&mc_model_checker->process, i), size,
        ref_subreg);
  }

  return region;
}

static void MC_snapshot_add_region(int index, mc_snapshot_t snapshot, mc_region_type_t type,
                                  mc_object_info_t object_info,
                                  void *start_addr, void* permanent_addr, size_t size)
{
  if (type == MC_REGION_TYPE_DATA)
    xbt_assert(object_info, "Missing object info for object.");
  else if (type == MC_REGION_TYPE_HEAP)
    xbt_assert(!object_info, "Unexpected object info for heap region.");

  mc_mem_region_t ref_reg = NULL;
  if (mc_model_checker->parent_snapshot)
    ref_reg = mc_model_checker->parent_snapshot->snapshot_regions[index];

  mc_mem_region_t region;
  const bool privatization_aware = object_info && MC_object_info_executable(object_info);
  if (privatization_aware && smpi_privatize_global_variables && smpi_process_count())
    region = MC_region_new_privatized(type, start_addr, permanent_addr, size, ref_reg);
  else
    region = MC_region_new(type, start_addr, permanent_addr, size, ref_reg);

  region->object_info = object_info;
  snapshot->snapshot_regions[index] = region;
  return;
}

static void MC_get_memory_regions(mc_process_t process, mc_snapshot_t snapshot)
{
  const size_t n = process->object_infos_size;
  snapshot->snapshot_regions_count = n + 1;
  snapshot->snapshot_regions = xbt_new0(mc_mem_region_t, n + 1);

  for (size_t i = 0; i!=n; ++i) {
    mc_object_info_t object_info = process->object_infos[i];
    MC_snapshot_add_region(i, snapshot, MC_REGION_TYPE_DATA, object_info,
      object_info->start_rw, object_info->start_rw,
      object_info->end_rw - object_info->start_rw);
  }

  xbt_mheap_t heap = MC_process_get_heap(process);
  void *start_heap = heap->base;
  void *end_heap = heap->breakval;

  MC_snapshot_add_region(n, snapshot, MC_REGION_TYPE_HEAP, NULL,
                        start_heap, start_heap,
                        (char *) end_heap - (char *) start_heap);
  snapshot->heap_bytes_used = mmalloc_get_bytes_used_remote(
    heap->heaplimit,
    MC_process_get_malloc_info(process));

#ifdef HAVE_SMPI
  if (smpi_privatize_global_variables && smpi_process_count()) {
    snapshot->privatization_index = smpi_loaded_page;
  } else
#endif
  {
    snapshot->privatization_index = MC_NO_PROCESS_INDEX;
  }
}

/** \brief Fills the position of the segments (executable, read-only, read/write).
 *
 * TODO, use dl_iterate_phdr to be more robust
 * */
void MC_find_object_address(memory_map_t maps, mc_object_info_t result)
{
  unsigned int i = 0;
  s_map_region_t reg;
  const char *name = basename(result->file_name);
  while (i < maps->mapsize) {
    reg = maps->regions[i];
    if (maps->regions[i].pathname == NULL
        || strcmp(basename(maps->regions[i].pathname), name)) {
      // Nothing to do
    } else if ((reg.prot & PROT_WRITE)) {
      xbt_assert(!result->start_rw,
                 "Multiple read-write segments for %s, not supported",
                 maps->regions[i].pathname);
      result->start_rw = reg.start_addr;
      result->end_rw = reg.end_addr;
      // .bss is usually after the .data:
      s_map_region_t *next = &(maps->regions[i + 1]);
      if (next->pathname == NULL && (next->prot & PROT_WRITE)
          && next->start_addr == reg.end_addr) {
        result->end_rw = maps->regions[i + 1].end_addr;
      }
    } else if ((reg.prot & PROT_READ) && (reg.prot & PROT_EXEC)) {
      xbt_assert(!result->start_exec,
                 "Multiple executable segments for %s, not supported",
                 maps->regions[i].pathname);
      result->start_exec = reg.start_addr;
      result->end_exec = reg.end_addr;
    } else if ((reg.prot & PROT_READ) && !(reg.prot & PROT_EXEC)) {
      xbt_assert(!result->start_ro,
                 "Multiple read only segments for %s, not supported",
                 maps->regions[i].pathname);
      result->start_ro = reg.start_addr;
      result->end_ro = reg.end_addr;
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
static bool mc_valid_variable(dw_variable_t var, dw_frame_t scope,
                              const void *ip)
{
  // The variable is not yet valid:
  if ((const void *) ((const char *) scope->low_pc + var->start_scope) > ip)
    return false;
  else
    return true;
}

static void mc_fill_local_variables_values(mc_stack_frame_t stack_frame,
                                           dw_frame_t scope, int process_index, xbt_dynar_t result)
{
  mc_process_t process = &mc_model_checker->process;

  void *ip = (void *) stack_frame->ip;
  if (ip < scope->low_pc || ip >= scope->high_pc)
    return;

  unsigned cursor = 0;
  dw_variable_t current_variable;
  xbt_dynar_foreach(scope->variables, cursor, current_variable) {

    if (!mc_valid_variable(current_variable, scope, (void *) stack_frame->ip))
      continue;

    int region_type;
    if ((long) stack_frame->ip > (long) process->libsimgrid_info->start_exec)
      region_type = 1;
    else
      region_type = 2;

    local_variable_t new_var = xbt_new0(s_local_variable_t, 1);
    new_var->subprogram = stack_frame->frame;
    new_var->ip = stack_frame->ip;
    new_var->name = xbt_strdup(current_variable->name);
    new_var->type = current_variable->type;
    new_var->region = region_type;

    if (current_variable->address != NULL) {
      new_var->address = current_variable->address;
    } else if (current_variable->locations.size != 0) {
      s_mc_location_t location;
      mc_dwarf_resolve_locations(&location, &current_variable->locations,
                                              current_variable->object_info,
                                              &(stack_frame->unw_cursor),
                                              (void *) stack_frame->frame_base,
                                              NULL, process_index);

      switch(mc_get_location_type(&location)) {
      case MC_LOCATION_TYPE_ADDRESS:
        new_var->address = location.memory_location;
        break;
      case MC_LOCATION_TYPE_REGISTER:
      default:
        xbt_die("Cannot handle non-address variable");
      }

    } else {
      xbt_die("No address");
    }

    xbt_dynar_push(result, &new_var);
  }

  // Recursive processing of nested scopes:
  dw_frame_t nested_scope = NULL;
  xbt_dynar_foreach(scope->scopes, cursor, nested_scope) {
    mc_fill_local_variables_values(stack_frame, nested_scope, process_index, result);
  }
}

static xbt_dynar_t MC_get_local_variables_values(xbt_dynar_t stack_frames, int process_index)
{

  unsigned cursor1 = 0;
  mc_stack_frame_t stack_frame;
  xbt_dynar_t variables =
      xbt_dynar_new(sizeof(local_variable_t), local_variable_free_voidp);

  xbt_dynar_foreach(stack_frames, cursor1, stack_frame) {
    mc_fill_local_variables_values(stack_frame, stack_frame->frame, process_index, variables);
  }

  return variables;
}

static void MC_stack_frame_free_voipd(void *s)
{
  mc_stack_frame_t stack_frame = *(mc_stack_frame_t *) s;
  if (stack_frame) {
    xbt_free(stack_frame->frame_name);
    xbt_free(stack_frame);
  }
}

static xbt_dynar_t MC_unwind_stack_frames(void *stack_context)
{
  mc_process_t process = &mc_model_checker->process;
  xbt_dynar_t result =
      xbt_dynar_new(sizeof(mc_stack_frame_t), MC_stack_frame_free_voipd);

  unw_cursor_t c;

  // TODO, check condition check (unw_init_local==0 means end of frame)
  if (unw_init_local(&c, (unw_context_t *) stack_context) != 0) {

    xbt_die("Could not initialize stack unwinding");

  } else
    while (1) {

      mc_stack_frame_t stack_frame = xbt_new(s_mc_stack_frame_t, 1);
      xbt_dynar_push(result, &stack_frame);

      stack_frame->unw_cursor = c;

      unw_word_t ip, sp;

      unw_get_reg(&c, UNW_REG_IP, &ip);
      unw_get_reg(&c, UNW_REG_SP, &sp);

      stack_frame->ip = ip;
      stack_frame->sp = sp;

      // TODO, use real addresses in frame_t instead of fixing it here

      dw_frame_t frame = MC_process_find_function(process, (void *) ip);
      stack_frame->frame = frame;

      if (frame) {
        stack_frame->frame_name = xbt_strdup(frame->name);
        stack_frame->frame_base =
            (unw_word_t) mc_find_frame_base(frame, frame->object_info, &c);
      } else {
        stack_frame->frame_base = 0;
        stack_frame->frame_name = NULL;
      }

      /* Stop before context switch with maestro */
      if (frame != NULL && frame->name != NULL
          && !strcmp(frame->name, "smx_ctx_sysv_wrapper"))
        break;

      int ret = ret = unw_step(&c);
      if (ret == 0) {
        xbt_die("Unexpected end of stack.");
      } else if (ret < 0) {
        xbt_die("Error while unwinding stack.");
      }
    }

  if (xbt_dynar_length(result) == 0) {
    XBT_INFO("unw_init_local failed");
    xbt_abort();
  }

  return result;
};

static xbt_dynar_t MC_take_snapshot_stacks(mc_snapshot_t * snapshot)
{

  xbt_dynar_t res =
      xbt_dynar_new(sizeof(s_mc_snapshot_stack_t),
                    MC_snapshot_stack_free_voidp);

  unsigned int cursor = 0;
  stack_region_t current_stack;

  xbt_dynar_foreach(stacks_areas, cursor, current_stack) {
    mc_snapshot_stack_t st = xbt_new(s_mc_snapshot_stack_t, 1);
    st->stack_frames = MC_unwind_stack_frames(current_stack->context);
    st->local_variables = MC_get_local_variables_values(st->stack_frames, current_stack->process_index);
    st->process_index = current_stack->process_index;

    unw_word_t sp = xbt_dynar_get_as(st->stack_frames, 0, mc_stack_frame_t)->sp;

    xbt_dynar_push(res, &st);
    (*snapshot)->stack_sizes =
        xbt_realloc((*snapshot)->stack_sizes, (cursor + 1) * sizeof(size_t));
    (*snapshot)->stack_sizes[cursor] =
      (char*) current_stack->address + current_stack->size - (char*) sp;
  }

  return res;

}

static xbt_dynar_t MC_take_snapshot_ignore()
{

  if (mc_heap_comparison_ignore == NULL)
    return NULL;

  xbt_dynar_t cpy =
      xbt_dynar_new(sizeof(mc_heap_ignore_region_t),
                    heap_ignore_region_free_voidp);

  unsigned int cursor = 0;
  mc_heap_ignore_region_t current_region;

  xbt_dynar_foreach(mc_heap_comparison_ignore, cursor, current_region) {
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

static void mc_free_snapshot_ignored_data_pvoid(void* data) {
  mc_snapshot_ignored_data_t ignored_data = (mc_snapshot_ignored_data_t) data;
  free(ignored_data->data);
}

static void MC_snapshot_handle_ignore(mc_snapshot_t snapshot)
{
  snapshot->ignored_data = xbt_dynar_new(sizeof(s_mc_snapshot_ignored_data_t), mc_free_snapshot_ignored_data_pvoid);

  // Copy the memory:
  unsigned int cursor = 0;
  mc_checkpoint_ignore_region_t region;
  xbt_dynar_foreach (mc_checkpoint_ignore, cursor, region) {
    s_mc_snapshot_ignored_data_t ignored_data;
    ignored_data.start = region->addr;
    ignored_data.size = region->size;
    ignored_data.data = malloc(region->size);
    memcpy(ignored_data.data, region->addr, region->size);
    xbt_dynar_push(snapshot->ignored_data, &ignored_data);
  }

  // Zero the memory:
  xbt_dynar_foreach (mc_checkpoint_ignore, cursor, region) {
    memset(region->addr, 0, region->size);
  }

}

static void MC_snapshot_ignore_restore(mc_snapshot_t snapshot)
{
  unsigned int cursor = 0;
  s_mc_snapshot_ignored_data_t ignored_data;
  xbt_dynar_foreach (snapshot->ignored_data, cursor, ignored_data) {
    memcpy(ignored_data.start, ignored_data.data, ignored_data.size);
  }
}

/** @brief Can we remove this snapshot?
 *
 * Some snapshots cannot be removed (yet) because we need them
 * at this point.
 *
 * @param snapshot
 */
int mc_important_snapshot(mc_snapshot_t snapshot)
{
  // We need this snapshot in order to know which
  // pages needs to be stored in the next snapshot.
  // This field is only non-NULL when using soft-dirty
  // page tracking.
  if (snapshot == mc_model_checker->parent_snapshot)
    return true;

  return false;
}

static void MC_get_current_fd(mc_snapshot_t snapshot){

  snapshot->total_fd = 0;

  const size_t fd_dir_path_size = 20;
  char fd_dir_path[fd_dir_path_size];
  if (snprintf(fd_dir_path, fd_dir_path_size,
    "/proc/%lli/fd", (long long int) getpid()) > fd_dir_path_size)
    xbt_die("Unexpected buffer is too small for fd_dir_path");

  DIR* fd_dir = opendir (fd_dir_path);
  if (fd_dir == NULL)
    xbt_die("Cannot open directory '/proc/self/fd'\n");

  size_t total_fd = 0;
  struct dirent* fd_number;
  while ((fd_number = readdir(fd_dir))) {

    int fd_value = atoi(fd_number->d_name);

    if(fd_value < 3)
      continue;

    const size_t source_size = 25;
    char source[25];
    if (snprintf(source, source_size, "/proc/self/fd/%s", fd_number->d_name) > source_size)
      xbt_die("Unexpected buffer is too small for fd %s", fd_number->d_name);

    const size_t link_size = 200;
    char link[200];
    int res = readlink(source, link, link_size);
    if (res<0) {
      xbt_die("Could not read link for %s", source);
    }
    if (res==200) {
      xbt_die("Buffer to small for link of %s", source);
    }
    link[res] = '\0';

    if(smpi_is_privatisation_file(link))
      continue;

    // This is (probably) the DIR* we are reading:
    // TODO, read all the file entries at once and close the DIR.*
    if(strcmp(fd_dir_path, link) == 0)
      continue;

    // We don't handle them.
    // It does not mean we should silently ignore them however.
    if (strncmp(link, "pipe:", 5) == 0 || strncmp(link, "socket:", 7) == 0)
      continue;

    // This is probably a shared memory used by lttng-ust:
    if(strncmp("/dev/shm/ust-shm-tmp-", link, 21)==0)
      continue;

    // Add an entry for this FD in the snapshot:
    fd_infos_t fd = xbt_new0(s_fd_infos_t, 1);
    fd->filename = strdup(link);
    fd->number = fd_value;
    fd->flags = fcntl(fd_value, F_GETFL) | fcntl(fd_value, F_GETFD) ;
    fd->current_position = lseek(fd_value, 0, SEEK_CUR);
    snapshot->current_fd = xbt_realloc(snapshot->current_fd, (total_fd + 1) * sizeof(fd_infos_t));
    snapshot->current_fd[total_fd] = fd;
    total_fd++;
  }

  snapshot->total_fd = total_fd;
  closedir (fd_dir);
}

mc_snapshot_t MC_take_snapshot(int num_state)
{
  mc_process_t mc_process = &mc_model_checker->process;
  mc_snapshot_t snapshot = xbt_new0(s_mc_snapshot_t, 1);

  snapshot->enabled_processes = xbt_dynar_new(sizeof(int), NULL);
  smx_process_t process;
  xbt_swag_foreach(process, simix_global->process_list) {
    xbt_dynar_push_as(snapshot->enabled_processes, int, (int)process->pid);
  }

  MC_snapshot_handle_ignore(snapshot);

  MC_get_current_fd(snapshot);

  const bool use_soft_dirty = _sg_mc_sparse_checkpoint
    && _sg_mc_soft_dirty
    && MC_process_is_self(mc_process);

  /* Save the std heap and the writable mapped pages of libsimgrid and binary */
  MC_get_memory_regions(mc_process, snapshot);
  if (use_soft_dirty)
    mc_softdirty_reset();

  snapshot->to_ignore = MC_take_snapshot_ignore();

  if (_sg_mc_visited > 0 || strcmp(_sg_mc_property_file, "")) {
    snapshot->stacks =
        MC_take_snapshot_stacks(&snapshot);
    if (_sg_mc_hash && snapshot->stacks != NULL) {
      snapshot->hash = mc_hash_processes_state(num_state, snapshot->stacks);
    } else {
      snapshot->hash = 0;
    }
  } else {
    snapshot->hash = 0;
  }

  MC_snapshot_ignore_restore(snapshot);
  if (use_soft_dirty)
    mc_model_checker->parent_snapshot = snapshot;
  return snapshot;
}

static inline
void MC_restore_snapshot_regions(mc_snapshot_t snapshot)
{
  mc_snapshot_t parent_snapshot = mc_model_checker->parent_snapshot;

  const size_t n = snapshot->snapshot_regions_count;
  for (size_t i = 0; i < n; i++) {
    // For privatized, variables we decided it was not necessary to take the snapshot:
    if (snapshot->snapshot_regions[i])
      MC_region_restore(snapshot->snapshot_regions[i],
        parent_snapshot ? parent_snapshot->snapshot_regions[i] : NULL);
  }

#ifdef HAVE_SMPI
  if(snapshot->privatization_index >= 0) {
    // We just rewrote the global variables.
    // The privatisation segment SMPI thinks
    // is mapped might be inconsistent with the segment which
    // is really mapped in memory (kernel state).
    // We ask politely SMPI to map the segment anyway,
    // even if it thinks it is the current one:
    smpi_really_switch_data_segment(snapshot->privatization_index);
  }
#endif
}

static inline
void MC_restore_snapshot_fds(mc_snapshot_t snapshot)
{
  int new_fd;
  size_t i;
  for(i=0; i < snapshot->total_fd; i++){
    
    new_fd = open(snapshot->current_fd[i]->filename, snapshot->current_fd[i]->flags);
    if (new_fd <0) {
      xbt_die("Could not reopen the file %s fo restoring the file descriptor",
        snapshot->current_fd[i]->filename);
    }
    if(new_fd != -1 && new_fd != snapshot->current_fd[i]->number){
      dup2(new_fd, snapshot->current_fd[i]->number);
      //fprintf(stderr, "%p\n", fdopen(snapshot->current_fd[i]->number, "rw"));
      close(new_fd);
    };
    lseek(snapshot->current_fd[i]->number, snapshot->current_fd[i]->current_position, SEEK_SET);
  }
}

void MC_restore_snapshot(mc_snapshot_t snapshot)
{
  const bool use_soft_dirty = _sg_mc_sparse_checkpoint
    && _sg_mc_soft_dirty
    && MC_process_is_self(&mc_model_checker->process);

  MC_restore_snapshot_regions(snapshot);
  MC_restore_snapshot_fds(snapshot);
  if (use_soft_dirty) {
    mc_softdirty_reset();
  }
  MC_snapshot_ignore_restore(snapshot);
  if (use_soft_dirty) {
    mc_model_checker->parent_snapshot = snapshot;
  }

  mc_model_checker->process.cache_flags = 0;
}

mc_snapshot_t simcall_HANDLER_mc_snapshot(smx_simcall_t simcall)
{
  return MC_take_snapshot(1);
}

void *MC_snapshot(void)
{
  return simcall_mc_snapshot();
}

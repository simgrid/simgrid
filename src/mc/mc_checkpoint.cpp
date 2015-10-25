/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <unistd.h>

#include <string.h>
#include <link.h>
#include <dirent.h>

#include "src/internal_config.h"
#include "mc_memory_map.h"
#include "mc_private.h"
#include "xbt/module.h"
#include <xbt/mmalloc.h>
#include "src/smpi/private.h"
#include <alloca.h>

#include "src/xbt/mmalloc/mmprivate.h"

#include "src/simix/smx_private.h"

#include <libunwind.h>
#include <libelf.h>

#include "mc_private.h"
#include <mc/mc.h>

#include "mc_snapshot.h"
#include "mc_object_info.h"
#include "mc_mmu.h"
#include "mc_unw.h"
#include "mc_protocol.h"
#include "mc_smx.h"
#include "mc_hash.hpp"

#include "src/mc/ObjectInformation.hpp"
#include "src/mc/Frame.hpp"
#include "src/mc/Variable.hpp"

using simgrid::mc::remote;

extern "C" {

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_checkpoint, mc,
                                "Logging specific to mc_checkpoint");

/************************************  Free functions **************************************/
/*****************************************************************************************/

/** @brief Restore a region from a snapshot
 *
 *  @param reg     Target region
 */
static void MC_region_restore(mc_mem_region_t region)
{
  switch(region->storage_type()) {
  case simgrid::mc::StorageType::NoData:
  default:
    xbt_die("Storage type not supported");
    break;

  case simgrid::mc::StorageType::Flat:
    mc_model_checker->process().write_bytes(region->flat_data(),
      region->size(), region->permanent_address());
    break;

  case simgrid::mc::StorageType::Chunked:
    mc_region_restore_sparse(&mc_model_checker->process(), region);
    break;

  case simgrid::mc::StorageType::Privatized:
    for (auto& p : region->privatized_data())
      MC_region_restore(&p);
    break;
  }
}

}

namespace simgrid {
namespace mc {

#ifdef HAVE_SMPI
simgrid::mc::RegionSnapshot privatized_region(
    RegionType region_type, void *start_addr, void* permanent_addr,
    std::size_t size, const simgrid::mc::RegionSnapshot* ref_region
    )
{
  size_t process_count = MC_smpi_process_count();

  // Read smpi_privatisation_regions from MCed:
  smpi_privatisation_region_t remote_smpi_privatisation_regions;
  mc_model_checker->process().read_variable(
    "smpi_privatisation_regions",
    &remote_smpi_privatisation_regions, sizeof(remote_smpi_privatisation_regions));
  s_smpi_privatisation_region_t privatisation_regions[process_count];
  mc_model_checker->process().read_bytes(
    &privatisation_regions, sizeof(privatisation_regions),
    remote(remote_smpi_privatisation_regions));

  std::vector<simgrid::mc::RegionSnapshot> data;
  data.reserve(process_count);
  for (size_t i = 0; i < process_count; i++) {
    const simgrid::mc::RegionSnapshot* ref_privatized_region = nullptr;
    if (ref_region && ref_region->storage_type() == StorageType::Privatized)
      ref_privatized_region = &ref_region->privatized_data()[i];
    data.push_back(simgrid::mc::region(region_type, start_addr,
      privatisation_regions[i].address, size, ref_privatized_region));
  }

  simgrid::mc::RegionSnapshot region = simgrid::mc::RegionSnapshot(
    region_type, start_addr, permanent_addr, size);
  region.privatized_data(std::move(data));
  return std::move(region);
}
#endif

}
}

extern "C" {

static void MC_snapshot_add_region(int index, mc_snapshot_t snapshot,
                                  simgrid::mc::RegionType type,
                                  simgrid::mc::ObjectInformation* object_info,
                                  void *start_addr, void* permanent_addr,
                                  std::size_t size)
{
  if (type == simgrid::mc::RegionType::Data)
    xbt_assert(object_info, "Missing object info for object.");
  else if (type == simgrid::mc::RegionType::Heap)
    xbt_assert(!object_info, "Unexpected object info for heap region.");

  simgrid::mc::RegionSnapshot const* ref_region = nullptr;
  if (mc_model_checker->parent_snapshot_)
    ref_region = mc_model_checker->parent_snapshot_->snapshot_regions[index].get();

  simgrid::mc::RegionSnapshot region;
#ifdef HAVE_SMPI
  const bool privatization_aware = object_info && object_info->privatized();
  if (privatization_aware && MC_smpi_process_count())
    region = simgrid::mc::privatized_region(
      type, start_addr, permanent_addr, size, ref_region);
  else
#endif
    region = simgrid::mc::region(type, start_addr, permanent_addr, size, ref_region);

  region.object_info(object_info);
  snapshot->snapshot_regions[index]
    = std::unique_ptr<simgrid::mc::RegionSnapshot>(
      new simgrid::mc::RegionSnapshot(std::move(region)));
  return;
}

static void MC_get_memory_regions(simgrid::mc::Process* process, mc_snapshot_t snapshot)
{
  const size_t n = process->object_infos.size();
  snapshot->snapshot_regions.resize(n + 1);
  int i = 0;
  for (auto const& object_info : process->object_infos) {
    MC_snapshot_add_region(i, snapshot, simgrid::mc::RegionType::Data,
      object_info.get(),
      object_info->start_rw, object_info->start_rw,
      object_info->end_rw - object_info->start_rw);
    ++i;
  }

  xbt_mheap_t heap = process->get_heap();
  void *start_heap = heap->base;
  void *end_heap = heap->breakval;

  MC_snapshot_add_region(n, snapshot, simgrid::mc::RegionType::Heap, NULL,
                        start_heap, start_heap,
                        (char *) end_heap - (char *) start_heap);
  snapshot->heap_bytes_used = mmalloc_get_bytes_used_remote(
    heap->heaplimit,
    process->get_malloc_info());

#ifdef HAVE_SMPI
  if (smpi_privatize_global_variables && MC_smpi_process_count()) {
    // snapshot->privatization_index = smpi_loaded_page
    mc_model_checker->process().read_variable(
      "smpi_loaded_page", &snapshot->privatization_index,
      sizeof(snapshot->privatization_index));
  } else
#endif
  {
    snapshot->privatization_index = simgrid::mc::ProcessIndexMissing;
  }
}

/** \brief Fills the position of the segments (executable, read-only, read/write).
 *
 *  `dl_iterate_phdr` would be more robust but would not work in cross-process.
 * */
void MC_find_object_address(
  std::vector<simgrid::mc::VmMap> const& maps,
  simgrid::mc::ObjectInformation* result)
{
  char* file_name = xbt_strdup(result->file_name.c_str());
  const char *name = basename(file_name);
  for (size_t i = 0; i < maps.size(); ++i) {
    simgrid::mc::VmMap const& reg = maps[i];
    if (maps[i].pathname.empty()
        || strcmp(basename(maps[i].pathname.c_str()), name)) {
      // Nothing to do
    } else if ((reg.prot & PROT_WRITE)) {
      xbt_assert(!result->start_rw,
                 "Multiple read-write segments for %s, not supported",
                 maps[i].pathname.c_str());
      result->start_rw = (char*) reg.start_addr;
      result->end_rw = (char*) reg.end_addr;
      // .bss is usually after the .data:
      simgrid::mc::VmMap const& next = maps[i + 1];
      if (next.pathname.empty() && (next.prot & PROT_WRITE)
          && next.start_addr == reg.end_addr) {
        result->end_rw = (char*) maps[i + 1].end_addr;
      }
    } else if ((reg.prot & PROT_READ) && (reg.prot & PROT_EXEC)) {
      xbt_assert(!result->start_exec,
                 "Multiple executable segments for %s, not supported",
                 maps[i].pathname.c_str());
      result->start_exec = (char*) reg.start_addr;
      result->end_exec = (char*) reg.end_addr;
    } else if ((reg.prot & PROT_READ) && !(reg.prot & PROT_EXEC)) {
      xbt_assert(!result->start_ro,
                 "Multiple read only segments for %s, not supported",
                 maps[i].pathname.c_str());
      result->start_ro = (char*) reg.start_addr;
      result->end_ro = (char*) reg.end_addr;
    }
  }

  result->start = result->start_rw;
  if ((const void*) result->start_ro > result->start)
    result->start = result->start_ro;
  if ((const void*) result->start_exec > result->start)
    result->start = result->start_exec;

  result->end = result->end_rw;
  if (result->end_ro && (const void*) result->end_ro < result->end)
    result->end = result->end_ro;
  if (result->end_exec && (const void*) result->end_exec > result->end)
    result->end = result->end_exec;

  xbt_assert(result->start_rw);
  xbt_assert(result->start_exec);
  free(file_name);
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
static bool mc_valid_variable(simgrid::mc::Variable* var,
                              simgrid::mc::Frame* scope,
                              const void *ip)
{
  // The variable is not yet valid:
  if ((const void *) ((const char *) scope->low_pc + var->start_scope) > ip)
    return false;
  else
    return true;
}

static void mc_fill_local_variables_values(mc_stack_frame_t stack_frame,
                                           simgrid::mc::Frame* scope,
                                           int process_index,
                                           std::vector<s_local_variable>& result)
{
  simgrid::mc::Process* process = &mc_model_checker->process();

  void *ip = (void *) stack_frame->ip;
  if (ip < scope->low_pc || ip >= scope->high_pc)
    return;

  for(simgrid::mc::Variable& current_variable :
      scope->variables) {

    if (!mc_valid_variable(&current_variable, scope, (void *) stack_frame->ip))
      continue;

    int region_type;
    // FIXME, get rid of `region_type`
    if ((long) stack_frame->ip > (long) process->libsimgrid_info->start_exec)
      region_type = 1;
    else
      region_type = 2;

    s_local_variable_t new_var;
    new_var.subprogram = stack_frame->frame;
    new_var.ip = stack_frame->ip;
    new_var.name = current_variable.name;
    new_var.type = current_variable.type;
    new_var.region = region_type;
    new_var.address = nullptr;

    if (current_variable.address != NULL) {
      new_var.address = current_variable.address;
    } else if (!current_variable.location_list.empty()) {
      simgrid::dwarf::Location location =
        simgrid::dwarf::resolve(
          current_variable.location_list,
          current_variable.object_info,
          &(stack_frame->unw_cursor),
          (void *) stack_frame->frame_base,
          &mc_model_checker->process(), process_index);

      if (!location.in_memory())
        xbt_die("Cannot handle non-address variable");
      new_var.address = location.address();

    } else {
      xbt_die("No address");
    }

    result.push_back(std::move(new_var));
  }

  // Recursive processing of nested scopes:
  for(simgrid::mc::Frame& nested_scope : scope->scopes)
    mc_fill_local_variables_values(
      stack_frame, &nested_scope, process_index, result);
}

static std::vector<s_local_variable> MC_get_local_variables_values(
  std::vector<s_mc_stack_frame_t>& stack_frames, int process_index)
{
  std::vector<s_local_variable> variables;
  for (s_mc_stack_frame_t& stack_frame : stack_frames)
    mc_fill_local_variables_values(&stack_frame, stack_frame.frame, process_index, variables);
  return std::move(variables);
}

static void MC_stack_frame_free_voipd(void *s)
{
  mc_stack_frame_t stack_frame = *(mc_stack_frame_t *) s;
  delete(stack_frame);
}

static std::vector<s_mc_stack_frame_t> MC_unwind_stack_frames(mc_unw_context_t stack_context)
{
  simgrid::mc::Process* process = &mc_model_checker->process();
  std::vector<s_mc_stack_frame_t> result;

  unw_cursor_t c;

  // TODO, check condition check (unw_init_local==0 means end of frame)
  if (mc_unw_init_cursor(&c, stack_context) != 0) {

    xbt_die("Could not initialize stack unwinding");

  } else
    while (1) {

      s_mc_stack_frame_t stack_frame;

      stack_frame.unw_cursor = c;

      unw_word_t ip, sp;

      unw_get_reg(&c, UNW_REG_IP, &ip);
      unw_get_reg(&c, UNW_REG_SP, &sp);

      stack_frame.ip = ip;
      stack_frame.sp = sp;

      // TODO, use real addresses in frame_t instead of fixing it here

      simgrid::mc::Frame* frame = process->find_function(remote(ip));
      stack_frame.frame = frame;

      if (frame) {
        stack_frame.frame_name = frame->name;
        stack_frame.frame_base =
            (unw_word_t) frame->frame_base(c);
      } else {
        stack_frame.frame_base = 0;
        stack_frame.frame_name = std::string();
      }

      result.push_back(std::move(stack_frame));

      /* Stop before context switch with maestro */
      if (frame != nullptr &&
          frame->name == "smx_ctx_sysv_wrapper")
        break;

      int ret = unw_step(&c);
      if (ret == 0) {
        xbt_die("Unexpected end of stack.");
      } else if (ret < 0) {
        xbt_die("Error while unwinding stack");
      }
    }

  if (result.empty()) {
    XBT_INFO("unw_init_local failed");
    xbt_abort();
  }

  return std::move(result);
};

static std::vector<s_mc_snapshot_stack_t> MC_take_snapshot_stacks(mc_snapshot_t * snapshot)
{
  std::vector<s_mc_snapshot_stack_t> res;

  unsigned int cursor = 0;
  stack_region_t current_stack;

  // FIXME, cross-process support (stack_areas)
  xbt_dynar_foreach(stacks_areas, cursor, current_stack) {
    s_mc_snapshot_stack_t st;

    // Read the context from remote process:
    unw_context_t context;
    mc_model_checker->process().read_bytes(
      &context, sizeof(context), remote(current_stack->context));

    if (mc_unw_init_context(&st.context, &mc_model_checker->process(),
      &context) < 0) {
      xbt_die("Could not initialise the libunwind context.");
    }
    st.stack_frames = MC_unwind_stack_frames(&st.context);
    st.local_variables = MC_get_local_variables_values(st.stack_frames, current_stack->process_index);
    st.process_index = current_stack->process_index;

    unw_word_t sp = st.stack_frames[0].sp;

    res.push_back(std::move(st));

    size_t stack_size =
      (char*) current_stack->address + current_stack->size - (char*) sp;
    (*snapshot)->stack_sizes.push_back(stack_size);
  }

  return std::move(res);

}

static std::vector<s_mc_heap_ignore_region_t> MC_take_snapshot_ignore()
{
  std::vector<s_mc_heap_ignore_region_t> res;

  if (mc_heap_comparison_ignore == NULL)
    return std::move(res);

  unsigned int cursor = 0;
  mc_heap_ignore_region_t current_region;

  xbt_dynar_foreach(mc_heap_comparison_ignore, cursor, current_region) {
    s_mc_heap_ignore_region_t new_region;
    new_region.address = current_region->address;
    new_region.size = current_region->size;
    new_region.block = current_region->block;
    new_region.fragment = current_region->fragment;
    res.push_back(std::move(new_region));
  }

  return std::move(res);
}

static void MC_snapshot_handle_ignore(mc_snapshot_t snapshot)
{
  xbt_assert(snapshot->process());
  
  // Copy the memory:
  for (auto const& region : mc_model_checker->process().ignored_regions()) {
    s_mc_snapshot_ignored_data_t ignored_data;
    ignored_data.start = (void*)region.addr;
    ignored_data.data.resize(region.size);
    // TODO, we should do this once per privatization segment:
    snapshot->process()->read_bytes(
      ignored_data.data.data(), region.size, remote(region.addr),
      simgrid::mc::ProcessIndexDisabled);
    snapshot->ignored_data.push_back(std::move(ignored_data));
  }

  // Zero the memory:
  for(auto const& region : mc_model_checker->process().ignored_regions()) {
    snapshot->process()->clear_bytes(remote(region.addr), region.size);
  }

}

static void MC_snapshot_ignore_restore(mc_snapshot_t snapshot)
{
  for (auto const& ignored_data : snapshot->ignored_data)
    snapshot->process()->write_bytes(
      ignored_data.data.data(), ignored_data.data.size(),
      remote(ignored_data.start));
}

static std::vector<s_fd_infos_t> MC_get_current_fds(pid_t pid)
{
  const size_t fd_dir_path_size = 20;
  char fd_dir_path[fd_dir_path_size];
  int res = snprintf(fd_dir_path, fd_dir_path_size,
    "/proc/%lli/fd", (long long int) pid);
  xbt_assert(res >= 0);
  if ((size_t) res > fd_dir_path_size)
    xbt_die("Unexpected buffer is too small for fd_dir_path");

  DIR* fd_dir = opendir(fd_dir_path);
  if (fd_dir == NULL)
    xbt_die("Cannot open directory '/proc/self/fd'\n");

  std::vector<s_fd_infos_t> fds;

  struct dirent* fd_number;
  while ((fd_number = readdir(fd_dir))) {

    int fd_value = atoi(fd_number->d_name);

    if(fd_value < 3)
      continue;

    const size_t source_size = 25;
    char source[25];
    int res = snprintf(source, source_size, "/proc/%lli/fd/%s",
        (long long int) pid, fd_number->d_name);
    xbt_assert(res >= 0);
    if ((size_t) res > source_size)
      xbt_die("Unexpected buffer is too small for fd %s", fd_number->d_name);

    const size_t link_size = 200;
    char link[200];
    res = readlink(source, link, link_size);
    if (res<0) {
      xbt_die("Could not read link for %s", source);
    }
    if (res==200) {
      xbt_die("Buffer to small for link of %s", source);
    }
    link[res] = '\0';

#ifdef HAVE_SMPI
    if(smpi_is_privatisation_file(link))
      continue;
#endif

    // This is (probably) the DIR* we are reading:
    // TODO, read all the file entries at once and close the DIR.*
    if(strcmp(fd_dir_path, link) == 0)
      continue;

    // We don't handle them.
    // It does not mean we should silently ignore them however.
    if (strncmp(link, "pipe:", 5) == 0 || strncmp(link, "socket:", 7) == 0)
      continue;

    // If dot_output enabled, do not handle the corresponding file
    if (dot_output !=  NULL && strcmp(basename(link), _sg_mc_dot_output_file) == 0)
      continue;

    // This is probably a shared memory used by lttng-ust:
    if(strncmp("/dev/shm/ust-shm-tmp-", link, 21)==0)
      continue;

    // Add an entry for this FD in the snapshot:
    s_fd_infos_t fd;
    fd.filename = std::string(link);
    fd.number = fd_value;
    fd.flags = fcntl(fd_value, F_GETFL) | fcntl(fd_value, F_GETFD) ;
    fd.current_position = lseek(fd_value, 0, SEEK_CUR);
    fds.push_back(std::move(fd));
  }

  closedir (fd_dir);
  return std::move(fds);
}

mc_snapshot_t MC_take_snapshot(int num_state)
{
  XBT_DEBUG("Taking snapshot %i", num_state);

  simgrid::mc::Process* mc_process = &mc_model_checker->process();

  mc_snapshot_t snapshot = new simgrid::mc::Snapshot(mc_process);

  snapshot->num_state = num_state;

  smx_process_t process;
  MC_EACH_SIMIX_PROCESS(process,
    snapshot->enabled_processes.insert(process->pid));

  MC_snapshot_handle_ignore(snapshot);

  if (_sg_mc_snapshot_fds)
    snapshot->current_fds = MC_get_current_fds(process->pid);

  const bool use_soft_dirty = _sg_mc_sparse_checkpoint && _sg_mc_soft_dirty;

  /* Save the std heap and the writable mapped pages of libsimgrid and binary */
  MC_get_memory_regions(mc_process, snapshot);
  if (use_soft_dirty)
    mc_process->reset_soft_dirty();

  snapshot->to_ignore = MC_take_snapshot_ignore();

  if (_sg_mc_visited > 0 || strcmp(_sg_mc_property_file, "")) {
    snapshot->stacks =
        MC_take_snapshot_stacks(&snapshot);
    if (_sg_mc_hash) {
      snapshot->hash = simgrid::mc::hash(*snapshot);
    } else {
      snapshot->hash = 0;
    }
  } else {
    snapshot->hash = 0;
  }

  MC_snapshot_ignore_restore(snapshot);
  if (use_soft_dirty)
    mc_model_checker->parent_snapshot_ = snapshot;
  return snapshot;
}

static inline
void MC_restore_snapshot_regions(mc_snapshot_t snapshot)
{
  for(std::unique_ptr<s_mc_mem_region_t> const& region : snapshot->snapshot_regions) {
    // For privatized, variables we decided it was not necessary to take the snapshot:
    if (region)
      MC_region_restore(region.get());
  }

#ifdef HAVE_SMPI
  // TODO, send a message to implement this in the MCed process
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
  if (mc_mode == MC_MODE_SERVER)
    xbt_die("FD snapshot not implemented in client/server mode.");

  for (auto const& fd : snapshot->current_fds) {
    
    int new_fd = open(fd.filename.c_str(), fd.flags);
    if (new_fd < 0) {
      xbt_die("Could not reopen the file %s fo restoring the file descriptor",
        fd.filename.c_str());
    }
    if (new_fd != fd.number) {
      dup2(new_fd, fd.number);
      close(new_fd);
    };
    lseek(fd.number, fd.current_position, SEEK_SET);
  }
}

void MC_restore_snapshot(mc_snapshot_t snapshot)
{
  XBT_DEBUG("Restore snapshot %i", snapshot->num_state);
  const bool use_soft_dirty = _sg_mc_sparse_checkpoint && _sg_mc_soft_dirty;
  MC_restore_snapshot_regions(snapshot);
  if (_sg_mc_snapshot_fds)
    MC_restore_snapshot_fds(snapshot);
  if (use_soft_dirty)
    mc_model_checker->process().reset_soft_dirty();
  MC_snapshot_ignore_restore(snapshot);
  mc_model_checker->process().cache_flags = 0;
  if (use_soft_dirty)
    mc_model_checker->parent_snapshot_ = snapshot;
}

mc_snapshot_t simcall_HANDLER_mc_snapshot(smx_simcall_t simcall)
{
  return MC_take_snapshot(1);
}

}

/* Copyright (c) 2008-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef WIN32
#include <sys/mman.h>
#endif

#include "xbt/file.hpp"

#include "src/mc/mc_config.hpp"
#include "src/mc/mc_hash.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_smx.hpp"
#include "src/mc/sosp/mc_snapshot.hpp"

using simgrid::mc::remote;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_checkpoint, mc, "Logging specific to mc_checkpoint");

#define PROT_RWX (PROT_READ | PROT_WRITE | PROT_EXEC)
#define PROT_RW (PROT_READ | PROT_WRITE)
#define PROT_RX (PROT_READ | PROT_EXEC)

namespace simgrid {
namespace mc {

/************************************  Free functions **************************************/
/*****************************************************************************************/

/** @brief Restore a region from a snapshot
 *
 *  @param region     Target region
 */
static void restore(RegionSnapshot* region)
{
  switch (region->storage_type()) {
    case simgrid::mc::StorageType::Flat:
      mc_model_checker->process().write_bytes(region->flat_data().get(), region->size(), region->permanent_address());
      break;

    case simgrid::mc::StorageType::Chunked:
      xbt_assert(((region->permanent_address().address()) & (xbt_pagesize - 1)) == 0, "Not at the beginning of a page");
      xbt_assert(simgrid::mc::mmu::chunk_count(region->size()) == region->page_data().page_count());

      for (size_t i = 0; i != region->page_data().page_count(); ++i) {
        void* target_page =
            (void*)simgrid::mc::mmu::join(i, (std::uintptr_t)(void*)region->permanent_address().address());
        const void* source_page = region->page_data().page(i);
        mc_model_checker->process().write_bytes(source_page, xbt_pagesize, remote(target_page));
      }

      break;

    case simgrid::mc::StorageType::Privatized:
      for (auto& p : region->privatized_data())
        restore(p.get());
      break;

    default: // includes StorageType::NoData
      xbt_die("Storage type not supported");
      break;
  }
}

static void get_memory_regions(simgrid::mc::RemoteClient* process, simgrid::mc::Snapshot* snapshot)
{
  snapshot->snapshot_regions_.clear();

  for (auto const& object_info : process->object_infos)
    snapshot->add_region(simgrid::mc::RegionType::Data, object_info.get(), object_info->start_rw, object_info->start_rw,
                         object_info->end_rw - object_info->start_rw);

  xbt_mheap_t heap = process->get_heap();
  void* start_heap = heap->base;
  void* end_heap   = heap->breakval;

  snapshot->add_region(simgrid::mc::RegionType::Heap, nullptr, start_heap, start_heap,
                       (char*)end_heap - (char*)start_heap);
  snapshot->heap_bytes_used_ = mmalloc_get_bytes_used_remote(heap->heaplimit, process->get_malloc_info());
  snapshot->privatization_index_ = simgrid::mc::ProcessIndexMissing;

#if HAVE_SMPI
  if (mc_model_checker->process().privatized() && MC_smpi_process_count())
    // snapshot->privatization_index = smpi_loaded_page
    mc_model_checker->process().read_variable("smpi_loaded_page", &snapshot->privatization_index_,
                                              sizeof(snapshot->privatization_index_));
#endif
}

/** @brief Fills the position of the segments (executable, read-only, read/write).
 * */
// TODO, use the ELF segment information for more robustness
void find_object_address(std::vector<simgrid::xbt::VmMap> const& maps, simgrid::mc::ObjectInformation* result)
{
  std::string name = simgrid::xbt::Path(result->file_name).get_base_name();

  for (size_t i = 0; i < maps.size(); ++i) {
    simgrid::xbt::VmMap const& reg = maps[i];
    if (maps[i].pathname.empty())
      continue;
    std::string map_basename = simgrid::xbt::Path(maps[i].pathname).get_base_name();
    if (map_basename != name)
      continue;

    // This is the non-GNU_RELRO-part of the data segment:
    if (reg.prot == PROT_RW) {
      xbt_assert(not result->start_rw, "Multiple read-write segments for %s, not supported", maps[i].pathname.c_str());
      result->start_rw = (char*)reg.start_addr;
      result->end_rw   = (char*)reg.end_addr;

      // The next VMA might be end of the data segment:
      if (i + 1 < maps.size() && maps[i + 1].pathname.empty() && maps[i + 1].prot == PROT_RW &&
          maps[i + 1].start_addr == reg.end_addr)
        result->end_rw = (char*)maps[i + 1].end_addr;
    }

    // This is the text segment:
    else if (reg.prot == PROT_RX) {
      xbt_assert(not result->start_exec, "Multiple executable segments for %s, not supported",
                 maps[i].pathname.c_str());
      result->start_exec = (char*)reg.start_addr;
      result->end_exec   = (char*)reg.end_addr;

      // The next VMA might be end of the data segment:
      if (i + 1 < maps.size() && maps[i + 1].pathname.empty() && maps[i + 1].prot == PROT_RW &&
          maps[i + 1].start_addr == reg.end_addr) {
        result->start_rw = (char*)maps[i + 1].start_addr;
        result->end_rw   = (char*)maps[i + 1].end_addr;
      }
    }

    // This is the GNU_RELRO-part of the data segment:
    else if (reg.prot == PROT_READ) {
      xbt_assert(not result->start_ro, "Multiple read only segments for %s, not supported", maps[i].pathname.c_str());
      result->start_ro = (char*)reg.start_addr;
      result->end_ro   = (char*)reg.end_addr;
    }
  }

  result->start = result->start_rw;
  if ((const void*)result->start_ro < result->start)
    result->start = result->start_ro;
  if ((const void*)result->start_exec < result->start)
    result->start = result->start_exec;

  result->end = result->end_rw;
  if (result->end_ro && (const void*)result->end_ro > result->end)
    result->end = result->end_ro;
  if (result->end_exec && (const void*)result->end_exec > result->end)
    result->end = result->end_exec;

  xbt_assert(result->start_exec || result->start_rw || result->start_ro);
}

/************************************* Take Snapshot ************************************/
/****************************************************************************************/

/** @brief Checks whether the variable is in scope for a given IP.
 *
 *  A variable may be defined only from a given value of IP.
 *
 *  @param var   Variable description
 *  @param scope Scope description
 *  @param ip    Instruction pointer
 *  @return      true if the variable is valid
 * */
static bool valid_variable(simgrid::mc::Variable* var, simgrid::mc::Frame* scope, const void* ip)
{
  // The variable is not yet valid:
  if (scope->range.begin() + var->start_scope > (std::uint64_t)ip)
    return false;
  else
    return true;
}

static void fill_local_variables_values(mc_stack_frame_t stack_frame, simgrid::mc::Frame* scope, int process_index,
                                        std::vector<s_local_variable_t>& result)
{
  if (not scope || not scope->range.contain(stack_frame->ip))
    return;

  for (simgrid::mc::Variable& current_variable : scope->variables) {

    if (not valid_variable(&current_variable, scope, (void*)stack_frame->ip))
      continue;

    s_local_variable_t new_var;
    new_var.subprogram = stack_frame->frame;
    new_var.ip         = stack_frame->ip;
    new_var.name       = current_variable.name;
    new_var.type       = current_variable.type;
    new_var.address    = nullptr;

    if (current_variable.address != nullptr)
      new_var.address = current_variable.address;
    else if (not current_variable.location_list.empty()) {
      simgrid::dwarf::Location location = simgrid::dwarf::resolve(
          current_variable.location_list, current_variable.object_info, &(stack_frame->unw_cursor),
          (void*)stack_frame->frame_base, &mc_model_checker->process(), process_index);

      if (not location.in_memory())
        xbt_die("Cannot handle non-address variable");
      new_var.address = location.address();

    } else
      xbt_die("No address");

    result.push_back(std::move(new_var));
  }

  // Recursive processing of nested scopes:
  for (simgrid::mc::Frame& nested_scope : scope->scopes)
    fill_local_variables_values(stack_frame, &nested_scope, process_index, result);
}

static std::vector<s_local_variable_t> get_local_variables_values(std::vector<s_mc_stack_frame_t>& stack_frames,
                                                                  int process_index)
{
  std::vector<s_local_variable_t> variables;
  for (s_mc_stack_frame_t& stack_frame : stack_frames)
    fill_local_variables_values(&stack_frame, stack_frame.frame, process_index, variables);
  return variables;
}

static std::vector<s_mc_stack_frame_t> unwind_stack_frames(simgrid::mc::UnwindContext* stack_context)
{
  simgrid::mc::RemoteClient* process = &mc_model_checker->process();
  std::vector<s_mc_stack_frame_t> result;

  unw_cursor_t c = stack_context->cursor();

  // TODO, check condition check (unw_init_local==0 means end of frame)

  while (1) {

    s_mc_stack_frame_t stack_frame;

    stack_frame.unw_cursor = c;

    unw_word_t ip;
    unw_word_t sp;

    unw_get_reg(&c, UNW_REG_IP, &ip);
    unw_get_reg(&c, UNW_REG_SP, &sp);

    stack_frame.ip = ip;
    stack_frame.sp = sp;

    // TODO, use real addresses in frame_t instead of fixing it here

    simgrid::mc::Frame* frame = process->find_function(remote(ip));
    stack_frame.frame         = frame;

    if (frame) {
      stack_frame.frame_name = frame->name;
      stack_frame.frame_base = (unw_word_t)frame->frame_base(c);
    } else {
      stack_frame.frame_base = 0;
      stack_frame.frame_name = std::string();
    }

    result.push_back(std::move(stack_frame));

    /* Stop before context switch with maestro */
    if (frame != nullptr && frame->name == "smx_ctx_wrapper")
      break;

    int ret = unw_step(&c);
    if (ret == 0)
      xbt_die("Unexpected end of stack.");
    else if (ret < 0)
      xbt_die("Error while unwinding stack");
  }

  xbt_assert(not result.empty(), "unw_init_local failed");

  return result;
}

static void take_snapshot_stacks(simgrid::mc::Snapshot* snapshot)
{
  for (auto const& stack : mc_model_checker->process().stack_areas()) {
    s_mc_snapshot_stack_t st;

    // Read the context from remote process:
    unw_context_t context;
    mc_model_checker->process().read_bytes(&context, sizeof(context), remote(stack.context));

    st.context.initialize(&mc_model_checker->process(), &context);

    st.stack_frames    = unwind_stack_frames(&st.context);
    st.local_variables = get_local_variables_values(st.stack_frames, stack.process_index);
    st.process_index   = stack.process_index;

    unw_word_t sp = st.stack_frames[0].sp;

    snapshot->stacks_.push_back(std::move(st));

    size_t stack_size = (char*)stack.address + stack.size - (char*)sp;
    snapshot->stack_sizes_.push_back(stack_size);
  }
}

static void snapshot_handle_ignore(simgrid::mc::Snapshot* snapshot)
{
  xbt_assert(snapshot->process());

  // Copy the memory:
  for (auto const& region : mc_model_checker->process().ignored_regions()) {
    s_mc_snapshot_ignored_data_t ignored_data;
    ignored_data.start = (void*)region.addr;
    ignored_data.data.resize(region.size);
    // TODO, we should do this once per privatization segment:
    snapshot->process()->read_bytes(ignored_data.data.data(), region.size, remote(region.addr),
                                    simgrid::mc::ProcessIndexDisabled);
    snapshot->ignored_data_.push_back(std::move(ignored_data));
  }

  // Zero the memory:
  for (auto const& region : mc_model_checker->process().ignored_regions())
    snapshot->process()->clear_bytes(remote(region.addr), region.size);
}

static void snapshot_ignore_restore(simgrid::mc::Snapshot* snapshot)
{
  for (auto const& ignored_data : snapshot->ignored_data_)
    snapshot->process()->write_bytes(ignored_data.data.data(), ignored_data.data.size(), remote(ignored_data.start));
}

std::shared_ptr<simgrid::mc::Snapshot> take_snapshot(int num_state)
{
  XBT_DEBUG("Taking snapshot %i", num_state);

  simgrid::mc::RemoteClient* mc_process = &mc_model_checker->process();

  std::shared_ptr<simgrid::mc::Snapshot> snapshot = std::make_shared<simgrid::mc::Snapshot>(mc_process, num_state);

  for (auto const& p : mc_model_checker->process().actors())
    snapshot->enabled_processes_.insert(p.copy.getBuffer()->get_pid());

  snapshot_handle_ignore(snapshot.get());

  /* Save the std heap and the writable mapped pages of libsimgrid and binary */
  get_memory_regions(mc_process, snapshot.get());

  snapshot->to_ignore_ = mc_model_checker->process().ignored_heap();

  if (_sg_mc_max_visited_states > 0 || not _sg_mc_property_file.get().empty()) {
    take_snapshot_stacks(snapshot.get());
    if (_sg_mc_hash)
      snapshot->hash_ = simgrid::mc::hash(*snapshot);
  }

  snapshot_ignore_restore(snapshot.get());
  return snapshot;
}

static inline void restore_snapshot_regions(simgrid::mc::Snapshot* snapshot)
{
  for (std::unique_ptr<simgrid::mc::RegionSnapshot> const& region : snapshot->snapshot_regions_) {
    // For privatized, variables we decided it was not necessary to take the snapshot:
    if (region)
      restore(region.get());
  }

#if HAVE_SMPI
  if (snapshot->privatization_index_ >= 0) {
    // Fix the privatization mmap:
    s_mc_message_restore_t message{MC_MESSAGE_RESTORE, snapshot->privatization_index_};
    mc_model_checker->process().getChannel().send(message);
  }
#endif
}

void restore_snapshot(std::shared_ptr<simgrid::mc::Snapshot> snapshot)
{
  XBT_DEBUG("Restore snapshot %i", snapshot->num_state_);
  restore_snapshot_regions(snapshot.get());
  snapshot_ignore_restore(snapshot.get());
  mc_model_checker->process().clear_cache();
}

} // namespace mc
} // namespace simgrid

/* Copyright (c) 2016-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_utils.hpp"

#include "private.hpp"
#include "smpi_config.hpp"
#include "src/kernel/xml/platf.hpp"
#include "xbt/ex.h"
#include "xbt/file.hpp"
#include "xbt/log.h"
#include "xbt/parse_units.hpp"
#include "xbt/str.h"
#include "xbt/sysdep.h"
#include <algorithm>
#include <boost/tokenizer.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_utils, smpi, "Logging specific to SMPI (utils)");

namespace {

double total_benched_time=0;
unsigned long total_malloc_size=0;
unsigned long total_shared_size=0;
unsigned int total_shared_calls=0;
struct alloc_metadata_t {
  size_t size          = 0;
  unsigned int numcall = 0;
  int line             = 0;
  std::string file;
};

struct current_buffer_metadata_t {
  alloc_metadata_t alloc;
  std::string name;
};

alloc_metadata_t max_malloc;
simgrid::smpi::F2C* current_handle = nullptr;
current_buffer_metadata_t current_buffer1;
current_buffer_metadata_t current_buffer2;

std::unordered_map<const void*, alloc_metadata_t> allocs;

std::unordered_map<int, std::vector<std::string>> collective_calls;

} // namespace

namespace simgrid::smpi::utils {

void add_benched_time(double time){
  total_benched_time += time;
}

void account_malloc_size(size_t size, std::string_view file, int line, const void* ptr)
{
  if (smpi_cfg_display_alloc()) {
    alloc_metadata_t metadata;
    metadata.size = size;
    metadata.line = line;
    metadata.numcall = 1;
    metadata.file    = file;
    allocs.try_emplace(ptr, metadata);

    total_malloc_size += size;
    if(size > max_malloc.size){
      max_malloc.size = size;
      max_malloc.line = line;
      max_malloc.numcall = 1;
      max_malloc.file    = file;
    } else if (size == max_malloc.size && max_malloc.line == line && max_malloc.file == file) {
      max_malloc.numcall++;
    }
  }
}

void account_shared_size(size_t size){
  if (smpi_cfg_display_alloc()) {
    total_shared_size += size;
    total_shared_calls++;
  }
}

void print_time_analysis(double global_time){
  if (simgrid::config::get_value<bool>("smpi/display-timing")) {
    XBT_INFO("Simulated time: %g seconds. \n\n"
             "The simulation took %g seconds (after parsing and platform setup)\n"
             "%g seconds were actual computation of the application",
             simgrid_get_clock(), global_time, total_benched_time);
    if (total_benched_time/global_time>=0.75)
      XBT_INFO("More than 75%% of the time was spent inside the application code.\n"
    "You may want to use sampling functions or trace replay to reduce this.");
  }
}

static void print_leaked_handles()
{
  // Put the leaked non-default handles in a vector to sort them by id
  std::vector<std::pair<unsigned int, smpi::F2C*>> handles;
  if (simgrid::smpi::F2C::lookup() != nullptr)
    std::copy_if(simgrid::smpi::F2C::lookup()->begin(), simgrid::smpi::F2C::lookup()->end(),
                 std::back_inserter(handles),
                 [](auto const& entry) { return entry.first >= simgrid::smpi::F2C::get_num_default_handles(); });
  if (handles.empty())
    return;

  auto max            = static_cast<unsigned long>(simgrid::config::get_value<int>("smpi/list-leaks"));
  std::string message = "Probable memory leaks in your code: SMPI detected %zu unfreed MPI handles:";
  if (max == 0)
    message += "\nHINT: Display types and addresses (n max) with --cfg=smpi/list-leaks:n.\n"
               "Running smpirun with -wrapper \"valgrind --leak-check=full\" can provide more information";
  XBT_INFO(message.c_str(), handles.size());
  if (max == 0)
    return;

  // we cannot trust F2C::lookup()->size() > F2C::get_num_default_handles() because some default handles are already
  // freed at this point
  bool display_advice = false;
  std::map<std::string, int, std::less<>> count;
  for (const auto& [_, elem] : handles) {
    std::string key = elem->name();
    if ((not xbt_log_no_loc) && (not elem->call_location().empty()))
      key += " at " + elem->call_location();
    else
      display_advice = true;
    auto& result = count.try_emplace(key, 0).first->second;
    result++;
  }
  if (display_advice)
    XBT_WARN("To get more information (location of allocations), compile your code with -trace-call-location flag of "
             "smpicc/f90");
  unsigned int i = 0;
  for (const auto& [key, value] : count) {
    if (value == 1)
      XBT_INFO("leaked handle of type %s", key.c_str());
    else
      XBT_INFO("%d leaked handles of type %s", value, key.c_str());
    i++;
    if (i == max)
      break;
  }
  if (max < count.size())
    XBT_INFO("(%lu more handle leaks hidden as you wanted to see only %lu of them)", count.size() - max, max);
}

static void print_leaked_buffers()
{
  if (allocs.empty())
    return;

  auto max            = static_cast<unsigned long>(simgrid::config::get_value<int>("smpi/list-leaks"));
  std::string message = "Probable memory leaks in your code: SMPI detected %zu unfreed buffers:";
  if (max == 0)
    message += "display types and addresses (n max) with --cfg=smpi/list-leaks:n.\nRunning smpirun with -wrapper "
               "\"valgrind --leak-check=full\" can provide more information";
  XBT_INFO(message.c_str(), allocs.size());

  if (max == 0)
    return;

  // gather by allocation origin (only one group reported in case of no-loc or if trace-call-location is not used)
  struct buff_leak {
    int count;
    size_t total_size;
    size_t min_size;
    size_t max_size;
  };
  std::map<std::string, struct buff_leak, std::less<>> leaks_aggreg;
  for (const auto& [_, elem] : allocs) {
    std::string key = "leaked allocations";
    if (not xbt_log_no_loc)
      key = elem.file + ":" + std::to_string(elem.line) + ": " + key;
    auto& result = leaks_aggreg.try_emplace(key, buff_leak{0, 0, elem.size, elem.size}).first->second;
    result.count++;
    result.total_size += elem.size;
    if (elem.size > result.max_size)
      result.max_size = elem.size;
    else if (elem.size < result.min_size)
      result.min_size = elem.size;
  }
  // now we can order by total size.
  std::vector<std::pair<std::string, buff_leak>> leaks(leaks_aggreg.begin(), leaks_aggreg.end());
  std::sort(leaks.begin(), leaks.end(),
            [](auto const& a, auto const& b) { return a.second.total_size > b.second.total_size; });

  unsigned int i = 0;
  for (const auto& [key, value] : leaks) {
    if (value.min_size == value.max_size)
      XBT_INFO("%s of total size %zu, called %d times, each with size %zu", key.c_str(), value.total_size, value.count,
               value.min_size);
    else
      XBT_INFO("%s of total size %zu, called %d times, with minimum size %zu and maximum size %zu", key.c_str(),
               value.total_size, value.count, value.min_size, value.max_size);
    i++;
    if (i == max)
      break;
  }
  if (max < leaks_aggreg.size())
    XBT_INFO("(more buffer leaks hidden as you wanted to see only %lu of them)", max);
}

void print_memory_analysis()
{
  if (smpi_cfg_display_alloc()) {
    print_leaked_handles();
    print_leaked_buffers();

    if(total_malloc_size != 0)
      XBT_INFO("Memory Usage: Simulated application allocated %lu bytes during its lifetime through malloc/calloc calls.\n"
             "Largest allocation at once from a single process was %zu bytes, at %s:%d. It was called %u times during the whole simulation.\n"
             "If this is too much, consider sharing allocations for computation buffers.\n"
             "This can be done automatically by setting --cfg=smpi/auto-shared-malloc-thresh to the minimum size wanted size (this can alter execution if data content is necessary)\n",
             total_malloc_size, max_malloc.size, simgrid::xbt::Path(max_malloc.file).get_base_name().c_str(), max_malloc.line, max_malloc.numcall
      );
    else
      XBT_INFO(
          "Allocations analysis asked, but 0 bytes were allocated through malloc/calloc calls intercepted by SMPI.\n"
          "The code may not use malloc() to allocate memory, or it was built with SMPI_NO_OVERRIDE_MALLOC");
    if(total_shared_size != 0)
      XBT_INFO("%lu bytes were automatically shared between processes, in %u calls\n", total_shared_size, total_shared_calls);
  }
}

void set_current_handle(F2C* handle){
  current_handle=handle;
}

void print_current_handle(){
  if(current_handle){
    if(current_handle->call_location().empty())
      XBT_INFO("To get handle location information, pass -trace-call-location flag to smpicc/f90 as well");
    else
      XBT_INFO("Handle %s was allocated by a call at %s", current_handle->name().c_str(),
               (char*)(current_handle->call_location().c_str()));
  }
}

void set_current_buffer(int i, const char* name, const void* buf){
  //clear previous one
  if(i==1){
    if(not current_buffer1.name.empty()){
      current_buffer1.name="";
    }
    if(not current_buffer2.name.empty()){
      current_buffer2.name="";
    }
  }
  auto meta = allocs.find(buf);
  if (meta == allocs.end()) {
    XBT_DEBUG("Buffer %p was not allocated with malloc/calloc", buf);
    return;
  }
  if(i==1){
    current_buffer1.alloc = meta->second;
    current_buffer1.name = name;
  }else{
    current_buffer2.alloc=meta->second;
    current_buffer2.name=name;
  }
}

void print_buffer_info()
{
  if (not current_buffer1.name.empty())
    XBT_INFO("Buffer %s was allocated from %s line %d, with size %zu", current_buffer1.name.c_str(),
             current_buffer1.alloc.file.c_str(), current_buffer1.alloc.line, current_buffer1.alloc.size);
  if (not current_buffer2.name.empty())
    XBT_INFO("Buffer %s was allocated from %s line %d, with size %zu", current_buffer2.name.c_str(),
             current_buffer2.alloc.file.c_str(), current_buffer2.alloc.line, current_buffer2.alloc.size);
}

size_t get_buffer_size(const void* buf){
  auto meta = allocs.find(buf);
  if (meta == allocs.end()) {
    //we don't know this buffer (on stack or feature disabled), assume it's fine.
    return  std::numeric_limits<std::size_t>::max();
  }
  return meta->second.size;
}

void account_free(const void* ptr){
  if (smpi_cfg_display_alloc()) {
    allocs.erase(ptr);
  }
}

int check_collectives_ordering(MPI_Comm comm, const std::string& call)
{
  unsigned int count = comm->get_collectives_count();
  comm->increment_collectives_count();
  if (auto vec = collective_calls.find(comm->id()); vec == collective_calls.end()) {
    collective_calls.try_emplace(comm->id(), std::vector<std::string>{call});
  } else {
    // are we the first ? add the call
    if (vec->second.size() == count) {
      vec->second.emplace_back(call);
    } else if (vec->second.size() > count) {
      if (vec->second[count] != call) {
        XBT_WARN("Collective operation mismatch. For process %ld, expected %s, got %s",
                 simgrid::s4u::this_actor::get_pid(), vec->second[count].c_str(), call.c_str());
        return MPI_ERR_OTHER;
      }
    } else {
      THROW_IMPOSSIBLE;
    }
  }
  return MPI_SUCCESS;
}
} // namespace simgrid::smpi::utils

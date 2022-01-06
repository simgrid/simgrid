/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include <boost/algorithm/string.hpp>
#include <simgrid/s4u/Actor.hpp>
#include <cctype>
#include <cstdarg>
#include <cwchar>
#include <deque>
#include <simgrid/sg_config.hpp>
#include <simgrid/s4u/Host.hpp>
#include <string>
#include <vector>

#include "src/smpi/include/smpi_actor.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_smpi, instr, "Tracing SMPI");

static std::unordered_map<std::string, std::deque<std::string>> keys;

static const std::map<std::string, std::string, std::less<>> smpi_colors = {{"recv", "1 0 0"},
                                                                            {"irecv", "1 0.52 0.52"},
                                                                            {"send", "0 0 1"},
                                                                            {"isend", "0.52 0.52 1"},
                                                                            {"sendrecv", "0 1 1"},
                                                                            {"wait", "1 1 0"},
                                                                            {"waitall", "0.78 0.78 0"},
                                                                            {"waitany", "0.78 0.78 0.58"},
                                                                            {"test", "0.52 0.52 0"},

                                                                            {"allgather", "1 0 0"},
                                                                            {"allgatherv", "1 0.52 0.52"},
                                                                            {"allreduce", "1 0 1"},
                                                                            {"alltoall", "0.52 0 1"},
                                                                            {"alltoallv", "0.78 0.52 1"},
                                                                            {"barrier", "0 0.39 0.78"},
                                                                            {"bcast", "0 0.78 0.39"},
                                                                            {"gather", "1 1 0"},
                                                                            {"gatherv", "1 1 0.52"},
                                                                            {"reduce", "0 1 0"},
                                                                            {"reducescatter", "0.52 1 0.52"},
                                                                            {"scan", "1 0.58 0.23"},
                                                                            {"exscan", "1 0.54 0.25"},
                                                                            {"scatterv", "0.52 0 0.52"},
                                                                            {"scatter", "1 0.74 0.54"},

                                                                            {"computing", "0 1 1"},
                                                                            {"sleeping", "0 0.5 0.5"},

                                                                            {"init", "0 1 0"},
                                                                            {"finalize", "0 1 0"},

                                                                            {"put", "0.3 1 0"},
                                                                            {"get", "0 1 0.3"},
                                                                            {"accumulate", "1 0.3 0"},
                                                                            {"rput", "0.3 1 0"},
                                                                            {"rget", "0 1 0.3"},
                                                                            {"raccumulate", "1 0.3 0"},
                                                                            {"compare_and_swap", "0.3 1 0"},
                                                                            {"get_accumulate", "0 1 0.3"},
                                                                            {"rget_accumulate", "1 0.3 0"},
                                                                            {"win_fence", "1 0 0.3"},
                                                                            {"win_post", "1 0 0.8"},
                                                                            {"win_wait", "1 0.8 0"},
                                                                            {"win_start", "0.8 0 1"},
                                                                            {"win_complete", "0.8 1 0"},
                                                                            {"win_lock", "1 0 0.3"},
                                                                            {"win_unlock", "1 0 0.3"},
                                                                            {"win_lock_all", "1 0 0.8"},
                                                                            {"win_unlock_all", "1 0.8 0"},
                                                                            {"win_flush", "1 0 0.3"},
                                                                            {"win_flush_local", "1 0 0.8"},
                                                                            {"win_flush_all", "1 0.8 0"},
                                                                            {"win_flush_local_all", "1 0 0.3"},

                                                                            {"file_read", "1 1 0.3"}};

static const char* instr_find_color(const char* c_state)
{
  std::string state(c_state);
  boost::algorithm::to_lower(state);
  if (state.substr(0, 5) == "pmpi_")
    state = state.substr(5, std::string::npos); // Remove pmpi_ to allow for exact matches

  if (smpi_colors.find(state) != smpi_colors.end()) { // Exact match in the map?
    return smpi_colors.find(state)->second.c_str();
  }
  for (const auto& pair : smpi_colors) { // Is an entry of our map a substring of this state name?
    if (std::strstr(state.c_str(), pair.first.c_str()) != nullptr)
      return pair.second.c_str();
  }

  return "0.5 0.5 0.5"; // Just in case we find nothing in the map ...
}

XBT_PRIVATE simgrid::instr::Container* smpi_container(aid_t pid)
{
  return simgrid::instr::Container::by_name(std::string("rank-") + std::to_string(pid));
}

static std::string TRACE_smpi_put_key(aid_t src, aid_t dst, int tag, int send)
{
  //generate the key
  static unsigned long long counter = 0;
  counter++;
  std::string key =
      std::to_string(src) + "_" + std::to_string(dst) + "_" + std::to_string(tag) + "_" + std::to_string(counter);

  //push it
  std::string aux =
      std::to_string(src) + "#" + std::to_string(dst) + "#" + std::to_string(tag) + "#" + std::to_string(send);
  keys[aux].push_back(key);

  return key;
}

static std::string TRACE_smpi_get_key(aid_t src, aid_t dst, int tag, int send)
{
  std::string key;
  std::string aux = std::to_string(src) + "#" + std::to_string(dst) + "#" + std::to_string(tag) + "#" +
                    std::to_string(send == 1 ? 0 : 1);
  auto it = keys.find(aux);
  if (it == keys.end()) {
    // first posted
    key = TRACE_smpi_put_key(src, dst, tag, send);
  } else {
    key = it->second.front();
    it->second.pop_front();
    if (it->second.empty())
      keys.erase(it);
  }
  return key;
}

void TRACE_smpi_setup_container(aid_t pid, const_sg_host_t host)
{
  auto* parent = simgrid::instr::Container::get_root();
  if (TRACE_smpi_is_grouped()) {
    parent = simgrid::instr::Container::by_name_or_null(host->get_name());
    xbt_assert(parent != nullptr, "Could not find a parent for mpi rank 'rank-%ld' at function %s", pid, __func__);
  }
  parent->create_child(std::string("rank-") + std::to_string(pid), "MPI"); // This container is of type MPI
}

void TRACE_smpi_init(aid_t pid, const std::string& calling_func)
{
  if (not TRACE_smpi_is_enabled())
    return;

  auto self = simgrid::s4u::Actor::self();

  TRACE_smpi_setup_container(pid, sg_host_self());
  simgrid::s4u::this_actor::on_exit([self](bool) { smpi_container(self->get_pid())->remove_from_parent(); });

  simgrid::instr::StateType* state = smpi_container(pid)->get_state("MPI_STATE");

  state->add_entity_value(calling_func, instr_find_color(calling_func.c_str()));
  state->push_event(calling_func, new simgrid::instr::NoOpTIData("init"));
  state->pop_event();
  if (TRACE_smpi_is_computing())
    state->add_entity_value("computing", instr_find_color("computing"));
  if (TRACE_smpi_is_sleeping())
    state->add_entity_value("sleeping", instr_find_color("sleeping"));

#if HAVE_PAPI
  const simgrid::instr::Container* container = smpi_container(pid);
  papi_counter_t counters = smpi_process()->papi_counters();

  for (auto const& it : counters) {
    /**
     * Check whether this variable already exists or not. Otherwise, it will be created
     * multiple times but only the last one would be used...
     */
    container->get_type()->by_name_or_create(it.first, "");
  }
#endif
}

void TRACE_smpi_sleeping_in(aid_t pid, double duration)
{
  if (TRACE_smpi_is_enabled() && TRACE_smpi_is_sleeping())
    smpi_container(pid)
        ->get_state("MPI_STATE")
        ->push_event("sleeping", new simgrid::instr::CpuTIData("sleep", duration));
}

void TRACE_smpi_sleeping_out(aid_t pid)
{
  if (TRACE_smpi_is_enabled() && TRACE_smpi_is_sleeping())
    smpi_container(pid)->get_state("MPI_STATE")->pop_event();
}

void TRACE_smpi_comm_in(aid_t pid, const char* operation, simgrid::instr::TIData* extra)
{
  if (not TRACE_smpi_is_enabled()) {
    delete extra;
    return;
  }

  simgrid::instr::StateType* state = smpi_container(pid)->get_state("MPI_STATE");
  state->add_entity_value(operation, instr_find_color(operation));
  state->push_event(operation, extra);
}

void TRACE_smpi_comm_out(aid_t pid)
{
  if (TRACE_smpi_is_enabled())
    smpi_container(pid)->get_state("MPI_STATE")->pop_event();
}

void TRACE_smpi_send(aid_t rank, aid_t src, aid_t dst, int tag, size_t size)
{
  if (not TRACE_smpi_is_enabled())
    return;

  std::string key = TRACE_smpi_get_key(src, dst, tag, 1);

  XBT_DEBUG("Send tracing from %ld to %ld, tag %d, with key %s", src, dst, tag, key.c_str());
  simgrid::instr::Container::get_root()->get_link("MPI_LINK")->start_event(smpi_container(rank), "PTP", key, size);
}

void TRACE_smpi_recv(aid_t src, aid_t dst, int tag)
{
  if (not TRACE_smpi_is_enabled())
    return;

  std::string key = TRACE_smpi_get_key(src, dst, tag, 0);

  XBT_DEBUG("Recv tracing from %ld to %ld, tag %d, with key %s", src, dst, tag, key.c_str());
  simgrid::instr::Container::get_root()->get_link("MPI_LINK")->end_event(smpi_container(dst), "PTP", key);
}

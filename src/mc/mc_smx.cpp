/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cassert>
#include <cstdlib>

#include <vector>

#include <xbt/log.h>
#include <xbt/str.h>
#include <xbt/swag.h>

#include "src/simix/smx_private.h"

#include "src/mc/mc_smx.h"
#include "src/mc/ModelChecker.hpp"

using simgrid::mc::remote;

extern "C" {

static inline
bool is_in_vector(smx_process_t p, std::vector<simgrid::mc::SimixProcessInformation>& ps)
{
  return (uintptr_t) p >= (uintptr_t) &ps[0]
    && (uintptr_t) p < (uintptr_t) &ps[ps.size()];
}

static inline
simgrid::mc::SimixProcessInformation* MC_smx_process_get_info(smx_process_t p)
{
  assert(is_in_vector(p, mc_model_checker->process().smx_process_infos)
    || is_in_vector(p, mc_model_checker->process().smx_old_process_infos));
  simgrid::mc::SimixProcessInformation* process_info =
    (simgrid::mc::SimixProcessInformation*)
      ((char*) p - offsetof(simgrid::mc::SimixProcessInformation, copy));
  return process_info;
}

/** Load the remote swag of processes into a dynar
 *
 *  @param process     MCed process
 *  @param target      Local dynar (to be filled with copies of `s_smx_process_t`)
 *  @param remote_swag Address of the process SWAG in the remote list
 */
static void MC_process_refresh_simix_process_list(
  simgrid::mc::Process* process,
  std::vector<simgrid::mc::SimixProcessInformation>& target, xbt_swag_t remote_swag)
{
  target.clear();

  // swag = REMOTE(*simix_global->process_list)
  s_xbt_swag_t swag;
  process->read_bytes(&swag, sizeof(swag), remote(remote_swag));

  // Load each element of the dynar from the MCed process:
  int i = 0;
  for (smx_process_t p = (smx_process_t) swag.head; p; ++i) {

    simgrid::mc::SimixProcessInformation info;
    info.address = p;
    info.hostname = nullptr;
    process->read_bytes(&info.copy, sizeof(info.copy), remote(p));
    target.push_back(std::move(info));

    // Lookup next process address:
    p = (smx_process_t) xbt_swag_getNext(&info.copy, swag.offset);
  }
  assert(i == swag.count);
}

namespace simgrid {
namespace mc {

void Process::refresh_simix()
{
  if (this->cache_flags_ & Process::cache_simix_processes)
    return;

  // TODO, avoid to reload `&simix_global`, `simix_global`, `*simix_global`

  // simix_global_p = REMOTE(simix_global);
  smx_global_t simix_global_p;
  this->read_variable("simix_global", &simix_global_p, sizeof(simix_global_p));

  // simix_global = REMOTE(*simix_global)
  s_smx_global_t simix_global;
  this->read_bytes(&simix_global, sizeof(simix_global),
    remote(simix_global_p));

  MC_process_refresh_simix_process_list(
    this, this->smx_process_infos, simix_global.process_list);
  MC_process_refresh_simix_process_list(
    this, this->smx_old_process_infos, simix_global.process_to_destroy);

  this->cache_flags_ |= Process::cache_simix_processes;
}

}
}

/** Get the issuer of a simcall (`req->issuer`)
 *
 *  In split-process mode, it does the black magic necessary to get an address
 *  of a (shallow) copy of the data structure the issuer SIMIX process in the local
 *  address space.
 *
 *  @param process the MCed process
 *  @param req     the simcall (copied in the local process)
 */
smx_process_t MC_smx_simcall_get_issuer(smx_simcall_t req)
{
  if (mc_mode == MC_MODE_CLIENT)
    return req->issuer;

  // This is the address of the smx_process in the MCed process:
  void* address = req->issuer;

  // Lookup by address:
  for (auto& p : mc_model_checker->process().simix_processes())
    if (p.address == address)
      return &p.copy;
  for (auto& p : mc_model_checker->process().old_simix_processes())
    if (p.address == address)
      return &p.copy;

  xbt_die("Issuer not found");
}

smx_process_t MC_smx_resolve_process(smx_process_t process_remote_address)
{
  if (!process_remote_address)
    return nullptr;
  if (mc_mode == MC_MODE_CLIENT)
    return process_remote_address;

  simgrid::mc::SimixProcessInformation* process_info = MC_smx_resolve_process_info(process_remote_address);
  if (process_info)
    return &process_info->copy;
  else
    return nullptr;
}

simgrid::mc::SimixProcessInformation* MC_smx_resolve_process_info(smx_process_t process_remote_address)
{
  if (mc_mode == MC_MODE_CLIENT)
    xbt_die("No process_info for local process is not enabled.");
  for (auto& process_info : mc_model_checker->process().smx_process_infos)
    if (process_info.address == process_remote_address)
      return &process_info;
  for (auto& process_info : mc_model_checker->process().smx_old_process_infos)
    if (process_info.address == process_remote_address)
      return &process_info;
  xbt_die("Process info not found");
}

const char* MC_smx_process_get_host_name(smx_process_t p)
{
  if (mc_mode == MC_MODE_CLIENT)
    return sg_host_get_name(p->host);

  simgrid::mc::Process* process = &mc_model_checker->process();

  /* Horrible hack to find the offset of the id in the simgrid::s4u::Host.

     Offsetof is not supported for non-POD types but this should
     work in pratice for the targets currently supported by the MC
     as long as we do not add funny features to the Host class
     (such as virtual base).

     We are using a (C++11) unrestricted union in order to avoid
     any construction/destruction of the simgrid::s4u::Host.
  */
  union fake_host {
    simgrid::s4u::Host host;
    fake_host() {}
    ~fake_host() {}
  };
  fake_host foo;
  const size_t offset = (char*) &foo.host.name() - (char*) &foo.host;

  // Read the simgrid::xbt::string in the MCed process:
  simgrid::mc::SimixProcessInformation* info = MC_smx_process_get_info(p);
  simgrid::xbt::string_data remote_string;
  auto remote_string_address = remote(
    (simgrid::xbt::string_data*) ((char*) p->host + offset));
  process->read_bytes(&remote_string, sizeof(remote_string), remote_string_address);
  char hostname[remote_string.len];
  process->read_bytes(hostname, remote_string.len + 1, remote(remote_string.data));
  info->hostname = mc_model_checker->get_host_name(hostname);
  return info->hostname;
}

const char* MC_smx_process_get_name(smx_process_t p)
{
  simgrid::mc::Process* process = &mc_model_checker->process();
  if (mc_mode == MC_MODE_CLIENT)
    return p->name;
  if (!p->name)
    return nullptr;

  simgrid::mc::SimixProcessInformation* info = MC_smx_process_get_info(p);
  if (info->name.empty()) {
    char* name = process->read_string(p->name);
    info->name = name;
    free(name);
  }
  return info->name.c_str();
}

#if HAVE_SMPI
int MC_smpi_process_count(void)
{
  if (mc_mode == MC_MODE_CLIENT)
    return smpi_process_count();
  int res;
  mc_model_checker->process().read_variable("process_count",
    &res, sizeof(res));
  return res;
}
#endif

unsigned long MC_smx_get_maxpid(void)
{
  unsigned long maxpid;
  mc_model_checker->process().read_variable("simix_process_maxpid",
    &maxpid, sizeof(maxpid));
  return maxpid;
}

}

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

/** Statically "upcast" a s_smx_process_t into a SimixProcessInformation
 *
 *  This gets 'processInfo' from '&processInfo->copy'. It upcasts in the
 *  sense that we could achieve the same thing by having SimixProcessInformation
 *  inherit from s_smx_process_t but we don't really want to do that.
 */
static inline
simgrid::mc::SimixProcessInformation* process_info_cast(smx_process_t p)
{
  simgrid::mc::SimixProcessInformation* process_info =
    (simgrid::mc::SimixProcessInformation*)
      ((char*) p - offsetof(simgrid::mc::SimixProcessInformation, copy));
  return process_info;
}

/** Load the remote swag of processes into a vector
 *
 *  @param process     MCed process
 *  @param target      Local vector (to be filled with copies of `s_smx_process_t`)
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

  // Load each element of the vector from the MCed process:
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
smx_process_t MC_smx_simcall_get_issuer(s_smx_simcall_t const* req)
{
  xbt_assert(mc_model_checker != nullptr);

  // This is the address of the smx_process in the MCed process:
  auto address = simgrid::mc::remote(req->issuer);

  // Lookup by address:
  for (auto& p : mc_model_checker->process().simix_processes())
    if (p.address == address)
      return &p.copy;
  for (auto& p : mc_model_checker->process().old_simix_processes())
    if (p.address == address)
      return &p.copy;

  xbt_die("Issuer not found");
}

const char* MC_smx_process_get_host_name(smx_process_t p)
{
  if (mc_model_checker == nullptr)
    return sg_host_get_name(p->host);

  simgrid::mc::Process* process = &mc_model_checker->process();

  /* HACK, Horrible hack to find the offset of the id in the simgrid::s4u::Host.

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
  simgrid::mc::SimixProcessInformation* info = process_info_cast(p);
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
  if (mc_model_checker == nullptr)
    return p->name;
  if (!p->name)
    return nullptr;

  simgrid::mc::SimixProcessInformation* info = process_info_cast(p);
  if (info->name.empty())
    info->name = process->read_string(p->name);
  return info->name.c_str();
}

#if HAVE_SMPI
int MC_smpi_process_count(void)
{
  if (mc_model_checker == nullptr)
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

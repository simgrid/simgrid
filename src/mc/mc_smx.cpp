/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cassert>
#include <cstddef>
#include <cstdlib>

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include <xbt/log.h>
#include <xbt/str.h>
#include <xbt/swag.h>

#include <simgrid/s4u/host.hpp>

#include "src/simix/smx_private.h"
#include "src/mc/mc_smx.h"
#include "src/mc/ModelChecker.hpp"

using simgrid::mc::remote;

/** HACK, Statically "upcast" a s_smx_process_t into a SimixProcessInformation
 *
 *  This gets 'processInfo' from '&processInfo->copy'. It upcasts in the
 *  sense that we could achieve the same thing by having SimixProcessInformation
 *  inherit from s_smx_process_t but we don't really want to do that.
 */
static inline
simgrid::mc::SimixProcessInformation* process_info_cast(smx_process_t p)
{
  simgrid::mc::SimixProcessInformation temp;
  std::size_t offset = (char*) temp.copy.getBuffer() - (char*)&temp;

  simgrid::mc::SimixProcessInformation* process_info =
    (simgrid::mc::SimixProcessInformation*) ((char*) p - offset);
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
  std::vector<simgrid::mc::SimixProcessInformation>& target,
  simgrid::mc::RemotePtr<s_xbt_swag_t> remote_swag)
{
  target.clear();

  // swag = REMOTE(*simix_global->process_list)
  s_xbt_swag_t swag;
  process->read_bytes(&swag, sizeof(swag), remote_swag);

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

  static_assert(std::is_same<
      std::unique_ptr<simgrid::simix::Global>,
      decltype(simix_global)
    >::value, "Unexpected type for simix_global");
  static_assert(sizeof(simix_global) == sizeof(simgrid::simix::Global*),
    "Bad size for simix_global");

  // simix_global_p = REMOTE(simix_global.get());
  RemotePtr<simgrid::simix::Global> simix_global_p =
    this->read_variable<simgrid::simix::Global*>("simix_global");

  // simix_global = REMOTE(*simix_global)
  Remote<simgrid::simix::Global> simix_global =
    this->read<simgrid::simix::Global>(simix_global_p);

  MC_process_refresh_simix_process_list(
    this, this->smx_process_infos,
    remote(simix_global.getBuffer()->process_list));
  MC_process_refresh_simix_process_list(
    this, this->smx_old_process_infos,
    remote(simix_global.getBuffer()->process_to_destroy));

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
      return p.copy.getBuffer();
  for (auto& p : mc_model_checker->process().old_simix_processes())
    if (p.address == address)
      return p.copy.getBuffer();

  xbt_die("Issuer not found");
}

const char* MC_smx_process_get_host_name(smx_process_t p)
{
  if (mc_model_checker == nullptr)
    return sg_host_get_name(p->host);

  simgrid::mc::Process* process = &mc_model_checker->process();

  /* HACK, Horrible hack to find the offset of the id in the simgrid::s4u::Host.

     Offsetof is not supported for non-POD types but this should
     work in practice for the targets currently supported by the MC
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
  auto remote_string_address = remote(
    (simgrid::xbt::string_data*) ((char*) p->host + offset));
  simgrid::xbt::string_data remote_string = process->read(remote_string_address);
  char hostname[remote_string.len];
  process->read_bytes(hostname, remote_string.len + 1, remote(remote_string.data));
  info->hostname = mc_model_checker->get_host_name(hostname);
  return info->hostname;
}

const char* MC_smx_process_get_name(smx_process_t p)
{
  simgrid::mc::Process* process = &mc_model_checker->process();
  if (mc_model_checker == nullptr)
    return p->name.c_str();

  simgrid::mc::SimixProcessInformation* info = process_info_cast(p);
  if (info->name.empty()) {
    simgrid::xbt::string_data string_data = (simgrid::xbt::string_data&)p->name;
    info->name = process->read_string(remote(string_data.data), string_data.len);
  }
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

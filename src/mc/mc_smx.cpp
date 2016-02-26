/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cassert>

#include <xbt/log.h>
#include <xbt/string.hpp>

#include "src/simix/smx_private.h"

#include "src/mc/mc_smx.h"
#include "ModelChecker.hpp"

using simgrid::mc::remote;

extern "C" {

static
void MC_smx_process_info_clear(mc_smx_process_info_t p)
{
  p->hostname = nullptr;
  free(p->name);
  p->name = nullptr;
}

xbt_dynar_t MC_smx_process_info_list_new(void)
{
  return xbt_dynar_new(
    sizeof(s_mc_smx_process_info_t),
    ( void_f_pvoid_t) &MC_smx_process_info_clear);
}

static inline
bool is_in_dynar(smx_process_t p, xbt_dynar_t dynar)
{
  return (uintptr_t) p >= (uintptr_t) dynar->data
    && (uintptr_t) p < ((uintptr_t) dynar->data + dynar->used * dynar->elmsize);
}

static inline
mc_smx_process_info_t MC_smx_process_get_info(smx_process_t p)
{
  assert(is_in_dynar(p, mc_model_checker->process().smx_process_infos)
    || is_in_dynar(p, mc_model_checker->process().smx_old_process_infos));
  mc_smx_process_info_t process_info =
    (mc_smx_process_info_t)
      ((char*) p - offsetof(s_mc_smx_process_info_t, copy));
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
  xbt_dynar_t target, xbt_swag_t remote_swag)
{
  // swag = REMOTE(*simix_global->process_list)
  s_xbt_swag_t swag;
  process->read_bytes(&swag, sizeof(swag), remote(remote_swag));

  smx_process_t p;
  xbt_dynar_reset(target);

  // Load each element of the dynar from the MCed process:
  int i = 0;
  for (p = (smx_process_t) swag.head; p; ++i) {

    s_mc_smx_process_info_t info;
    info.address = p;
    info.name = nullptr;
    info.hostname = nullptr;
    process->read_bytes(&info.copy, sizeof(info.copy), remote(p));
    xbt_dynar_push(target, &info);

    // Lookup next process address:
    p = (smx_process_t) xbt_swag_getNext(&info.copy, swag.offset);
  }
  assert(i == swag.count);
}

void MC_process_smx_refresh(simgrid::mc::Process* process)
{
  xbt_assert(mc_mode == MC_MODE_SERVER);
  if (process->cache_flags & MC_PROCESS_CACHE_FLAG_SIMIX_PROCESSES)
    return;

  // TODO, avoid to reload `&simix_global`, `simix_global`, `*simix_global`

  // simix_global_p = REMOTE(simix_global);
  smx_global_t simix_global_p;
  process->read_variable("simix_global", &simix_global_p, sizeof(simix_global_p));

  // simix_global = REMOTE(*simix_global)
  s_smx_global_t simix_global;
  process->read_bytes(&simix_global, sizeof(simix_global),
    remote(simix_global_p));

  MC_process_refresh_simix_process_list(
    process, process->smx_process_infos, simix_global.process_list);
  MC_process_refresh_simix_process_list(
    process, process->smx_old_process_infos, simix_global.process_to_destroy);

  process->cache_flags |= MC_PROCESS_CACHE_FLAG_SIMIX_PROCESSES;
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

  MC_process_smx_refresh(&mc_model_checker->process());

  // This is the address of the smx_process in the MCed process:
  void* address = req->issuer;

  unsigned i;
  mc_smx_process_info_t p;

  // Lookup by address:
  xbt_dynar_foreach_ptr(mc_model_checker->process().smx_process_infos, i, p)
    if (p->address == address)
      return &p->copy;
  xbt_dynar_foreach_ptr(mc_model_checker->process().smx_old_process_infos, i, p)
    if (p->address == address)
      return &p->copy;

  xbt_die("Issuer not found");
}

smx_process_t MC_smx_resolve_process(smx_process_t process_remote_address)
{
  if (!process_remote_address)
    return nullptr;
  if (mc_mode == MC_MODE_CLIENT)
    return process_remote_address;

  mc_smx_process_info_t process_info = MC_smx_resolve_process_info(process_remote_address);
  if (process_info)
    return &process_info->copy;
  else
    return nullptr;
}

mc_smx_process_info_t MC_smx_resolve_process_info(smx_process_t process_remote_address)
{
  if (mc_mode == MC_MODE_CLIENT)
    xbt_die("No process_info for local process is not enabled.");

  unsigned index;
  mc_smx_process_info_t process_info;
  xbt_dynar_foreach_ptr(mc_model_checker->process().smx_process_infos, index, process_info)
    if (process_info->address == process_remote_address)
      return process_info;
  xbt_dynar_foreach_ptr(mc_model_checker->process().smx_old_process_infos, index, process_info)
    if (process_info->address == process_remote_address)
      return process_info;
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
  mc_smx_process_info_t info = MC_smx_process_get_info(p);
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

  mc_smx_process_info_t info = MC_smx_process_get_info(p);
  if (!info->name) {
    info->name = process->read_string(p->name);
  }
  return info->name;
}

#ifdef HAVE_SMPI
int MC_smpi_process_count(void)
{
  if (mc_mode == MC_MODE_CLIENT)
    return smpi_process_count();
  else {
    int res;
    mc_model_checker->process().read_variable("process_count",
      &res, sizeof(res));
    return res;
  }
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

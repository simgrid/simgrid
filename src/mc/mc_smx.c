/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <assert.h>

#include <xbt/log.h>

#include "simix/smx_private.h"

#include "mc_smx.h"
#include "mc_model_checker.h"

static
void MC_smx_process_info_clear(mc_smx_process_info_t p)
{
  p->hostname = NULL;

  xbt_mheap_t heap = mmalloc_set_current_heap(mc_heap);
  free(p->name);
  mmalloc_set_current_heap(heap);

  p->name = NULL;
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
  assert(is_in_dynar(p, mc_model_checker->process.smx_process_infos)
    || is_in_dynar(p, mc_model_checker->process.smx_old_process_infos));
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
  mc_process_t process,
  xbt_dynar_t target, xbt_swag_t remote_swag)
{
  // swag = REMOTE(*simix_global->process_list)
  s_xbt_swag_t swag;
  MC_process_read(process, MC_PROCESS_NO_FLAG, &swag, remote_swag, sizeof(swag),
    MC_PROCESS_INDEX_ANY);

  smx_process_t p;
  xbt_dynar_reset(target);

  // Load each element of the dynar from the MCed process:
  int i = 0;
  for (p = swag.head; p; ++i) {

    s_mc_smx_process_info_t info;
    info.address = p;
    info.name = NULL;
    info.hostname = NULL;
    MC_process_read(process, MC_PROCESS_NO_FLAG,
      &info.copy, p, sizeof(info.copy), MC_PROCESS_INDEX_ANY);
    xbt_dynar_push(target, &info);

    // Lookup next process address:
    p = xbt_swag_getNext(&info.copy, swag.offset);
  }
  assert(i == swag.count);
}

void MC_process_smx_refresh(mc_process_t process)
{
  if (process->cache_flags & MC_PROCESS_CACHE_FLAG_SIMIX_PROCESSES)
    return;

  xbt_mheap_t heap = mmalloc_set_current_heap(mc_heap);

  // TODO, avoid to reload `&simix_global`, `simix_global`, `*simix_global`

  // simix_global_p = REMOTE(simix_global);
  smx_global_t simix_global_p;
  MC_process_read_variable(process, "simix_global", &simix_global_p, sizeof(simix_global_p));

  // simix_global = REMOTE(*simix_global)
  s_smx_global_t simix_global;
  MC_process_read(process, MC_PROCESS_NO_FLAG, &simix_global, simix_global_p, sizeof(simix_global),
    MC_PROCESS_INDEX_ANY);

  MC_process_refresh_simix_process_list(
    process, process->smx_process_infos, simix_global.process_list);
  MC_process_refresh_simix_process_list(
    process, process->smx_old_process_infos, simix_global.process_to_destroy);

  process->cache_flags |= MC_PROCESS_CACHE_FLAG_SIMIX_PROCESSES;
  mmalloc_set_current_heap(heap);
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
  if (MC_process_is_self(&mc_model_checker->process))
    return req->issuer;

  MC_process_smx_refresh(&mc_model_checker->process);

  // This is the address of the smx_process in the MCed process:
  void* address = req->issuer;

  unsigned i;
  mc_smx_process_info_t p;

  // Lookup by address:
  xbt_dynar_foreach_ptr(mc_model_checker->process.smx_process_infos, i, p)
    if (p->address == address)
      return &p->copy;
  xbt_dynar_foreach_ptr(mc_model_checker->process.smx_old_process_infos, i, p)
    if (p->address == address)
      return &p->copy;

  xbt_die("Issuer not found");
}

smx_process_t MC_smx_resolve_process(smx_process_t process_remote_address)
{
  if (!process_remote_address)
    return NULL;
  if (MC_process_is_self(&mc_model_checker->process))
    return process_remote_address;

  mc_smx_process_info_t process_info = MC_smx_resolve_process_info(process_remote_address);
  if (process_info)
    return &process_info->copy;
  else
    return NULL;
}

mc_smx_process_info_t MC_smx_resolve_process_info(smx_process_t process_remote_address)
{
  if (MC_process_is_self(&mc_model_checker->process))
    xbt_die("No process_info for local process is not enabled.");

  unsigned index;
  mc_smx_process_info_t process_info;
  xbt_dynar_foreach_ptr(mc_model_checker->process.smx_process_infos, index, process_info)
    if (process_info->address == process_remote_address)
      return process_info;
  xbt_dynar_foreach_ptr(mc_model_checker->process.smx_old_process_infos, index, process_info)
    if (process_info->address == process_remote_address)
      return process_info;
  xbt_die("Process info not found");
}

const char* MC_smx_process_get_host_name(smx_process_t p)
{
  if (MC_process_is_self(&mc_model_checker->process))
    return SIMIX_host_get_name(p->smx_host);

  mc_process_t process = &mc_model_checker->process;

  // Currently, smx_host_t = xbt_dictelm_t.
  // TODO, add an static_assert on this if switching to C++
  // The host name is host->key and the host->key_len==strlen(host->key).
  s_xbt_dictelm_t host_copy;
  mc_smx_process_info_t info = MC_smx_process_get_info(p);
  if (!info->hostname) {

    // Read the hostname from the MCed process:
    MC_process_read_simple(process, &host_copy, p->smx_host, sizeof(host_copy));
    int len = host_copy.key_len + 1;
    char hostname[len];
    MC_process_read_simple(process, hostname, host_copy.key, len);

    // Lookup the host name in the dictionary (or create it):
    xbt_dictelm_t elt = xbt_dict_get_elm_or_null(mc_model_checker->hosts, hostname);
    if (!elt) {
      xbt_mheap_t heap = mmalloc_set_current_heap(mc_heap);
      xbt_dict_set(mc_model_checker->hosts, hostname, NULL, NULL);
      elt = xbt_dict_get_elm_or_null(mc_model_checker->hosts, hostname);
      assert(elt);
      mmalloc_set_current_heap(heap);
    }

    info->hostname = elt->key;
  }
  return info->hostname;
}

const char* MC_smx_process_get_name(smx_process_t p)
{
  mc_process_t process = &mc_model_checker->process;
  if (MC_process_is_self(process))
    return p->name;
  if (!p->name)
    return NULL;

  mc_smx_process_info_t info = MC_smx_process_get_info(p);
  if (!info->name) {
    size_t size = 128;
    char buffer[size];

    size_t off = 0;
    while (1) {
      ssize_t n = pread(process->memory_file, buffer+off, size-off, (off_t)p->name + off);
      if (n==-1) {
        if (errno == EINTR)
          continue;
        else {
          perror("MC_smx_process_get_name");
          xbt_die("Could not read memory");
        }
      }
      if (n==0)
        return "?";
      void* end = memchr(buffer+off, '\0', n);
      if (end)
        break;
      off += n;
      if (off == size)
        return "?";
    }
    xbt_mheap_t heap = mmalloc_set_current_heap(mc_heap);
    info->name = strdup(buffer);
    mmalloc_set_current_heap(heap);
  }
  return info->name;
}

int MC_smpi_process_count(void)
{
  if (MC_process_is_self(&mc_model_checker->process))
    return smpi_process_count();
  else {
    int res;
    MC_process_read_variable(&mc_model_checker->process, "process_count",
      &res, sizeof(res));
    return res;
  }
}

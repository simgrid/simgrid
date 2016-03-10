/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <xbt/fifo.h>
#include <xbt/log.h>
#include <xbt/sysdep.h>

#include "simgrid/simix.h"

#include "src/simix/smx_private.h"
#include "src/simix/smx_process_private.h"

#include "src/mc/mc_replay.h"
#include "src/mc/mc_record.h"
#include "src/mc/mc_base.h"

#if HAVE_MC
#include "src/mc/mc_request.h"
#include "src/mc/mc_private.h"
#include "src/mc/mc_state.h"
#include "src/mc/mc_smx.h"
#include "src/mc/mc_liveness.h"
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_record, mc,
  " Logging specific to MC record/replay facility");

extern "C" {

char* MC_record_path = nullptr;

void MC_record_replay(mc_record_item_t start, std::size_t len)
{
  simgrid::mc::wait_for_requests();
  mc_record_item_t end = start + len;

  // Choose the recorded simcall and execute it:
  for (mc_record_item_t item=start;item!=end; ++item) {

    XBT_DEBUG("Executing %i$%i", item->pid, item->value);
/*
    if (xbt_dynar_is_empty(simix_global->process_to_run))
      xbt_die("Unexpected end of application.");
*/

    // Choose a request:
    smx_process_t process = SIMIX_process_from_PID(item->pid);
    if (!process)
      xbt_die("Unexpected process.");
    smx_simcall_t simcall = &(process->simcall);
    if(!simcall || simcall->call == SIMCALL_NONE)
      xbt_die("No simcall for this process.");
    if (!simgrid::mc::request_is_visible(simcall)
        || !simgrid::mc::request_is_enabled(simcall))
      xbt_die("Unexpected simcall.");

    // Execute the request:
    SIMIX_simcall_handle(simcall, item->value);
    simgrid::mc::wait_for_requests();
  }
}

xbt_dynar_t MC_record_from_string(const char* data)
{
  XBT_INFO("path=%s", data);
  if (!data || !data[0])
    return nullptr;

  xbt_dynar_t dynar = xbt_dynar_new(sizeof(s_mc_record_item_t), nullptr);

  const char* current = data;
  while (*current) {

    s_mc_record_item_t item = { 0, 0 };
    int count = sscanf(current, "%u/%u", &item.pid, &item.value);
    if(count != 2 && count != 1)
      goto fail;
    xbt_dynar_push(dynar, &item);

    // Find next chunk:
    const char* end = std::strchr(current, ';');
    if(end == nullptr)
      break;
    else
      current = end + 1;
  }

  return dynar;

fail:
  xbt_dynar_free(&dynar);
  return nullptr;
}

#if HAVE_MC
static char* MC_record_stack_to_string_liveness(xbt_fifo_t stack)
{
  char* buffer;
  std::size_t size;
  std::FILE* file = open_memstream(&buffer, &size);

  xbt_fifo_item_t item;
  xbt_fifo_item_t start = xbt_fifo_get_last_item(stack);
  for (item = start; item; item = xbt_fifo_get_prev_item(item)) {
    simgrid::mc::Pair* pair = (simgrid::mc::Pair*) xbt_fifo_get_item_content(item);
    int value;
    smx_simcall_t req = MC_state_get_executed_request(pair->graph_state, &value);
    if (req && req->call != SIMCALL_NONE) {
      smx_process_t issuer = MC_smx_simcall_get_issuer(req);
      const int pid = issuer->pid;

      // Serialization the (pid, value) pair:
      const char* sep = (item!=start) ? ";" : "";
      if (value)
        std::fprintf(file, "%s%u/%u", sep, pid, value);
      else
        std::fprintf(file, "%s%u", sep, pid);
    }
  }

  std::fclose(file);
  return buffer;
}

char* MC_record_stack_to_string(xbt_fifo_t stack)
{
  if (_sg_mc_liveness)
    return MC_record_stack_to_string_liveness(stack);

  xbt_fifo_item_t start = xbt_fifo_get_last_item(stack);

  if (!start) {
    char* res = (char*) malloc(1 * sizeof(char));
    res[0] = '\0';
    return res;
  }

  char* buffer;
  std::size_t size;
  std::FILE* file = open_memstream(&buffer, &size);

  xbt_fifo_item_t item;
  for (item = start; item; item = xbt_fifo_get_prev_item(item)) {

    // Find (pid, value):
    mc_state_t state = (mc_state_t) xbt_fifo_get_item_content(item);
    int value = 0;
    smx_simcall_t saved_req = MC_state_get_executed_request(state, &value);
    const smx_process_t issuer = MC_smx_simcall_get_issuer(saved_req);
    const int pid = issuer->pid;

    // Serialization the (pid, value) pair:
    const char* sep = (item!=start) ? ";" : "";
    if (value)
      std::fprintf(file, "%s%u/%u", sep, pid, value);
    else
      std::fprintf(file, "%s%u", sep, pid);
  }

  std::fclose(file);
  return buffer;
}

void MC_record_dump_path(xbt_fifo_t stack)
{
  if (MC_record_is_active()) {
    char* path = MC_record_stack_to_string(stack);
    XBT_INFO("Path = %s", path);
    std::free(path);
  }
}
#endif

void MC_record_replay_from_string(const char* path_string)
{
  xbt_dynar_t path = MC_record_from_string(path_string);
  mc_record_item_t start = &xbt_dynar_get_as(path, 0, s_mc_record_item_t);
  MC_record_replay(start, xbt_dynar_length(path));
  xbt_dynar_free(&path);
}

void MC_record_replay_init()
{
  simgrid::mc::processes_time.resize(simix_process_maxpid);
}

}

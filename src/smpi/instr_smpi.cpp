/* Copyright (c) 2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "private.hpp"
#include <ctype.h>
#include <simgrid/sg_config.h>
#include <stdarg.h>
#include <wchar.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_smpi, instr, "Tracing SMPI");

static xbt_dict_t keys;

static const char *smpi_colors[] ={
    "recv",     "1 0 0",
    "irecv",    "1 0.52 0.52",
    "send",     "0 0 1",
    "isend",    "0.52 0.52 1",
    "sendrecv", "0 1 1",
    "wait",     "1 1 0",
    "waitall",  "0.78 0.78 0",
    "waitany",  "0.78 0.78 0.58",
    "test",     "0.52 0.52 0",

    "allgather",     "1 0 0",
    "allgatherv",    "1 0.52 0.52",
    "allreduce",     "1 0 1",
    "alltoall",      "0.52 0 1",
    "alltoallv",     "0.78 0.52 1",
    "barrier",       "0 0.78 0.78",
    "bcast",         "0 0.78 0.39",
    "gather",        "1 1 0",
    "gatherv",       "1 1 0.52",
    "reduce",        "0 1 0",
    "reducescatter", "0.52 1 0.52",
    "scan",          "1 0.58 0.23",
    "exscan",          "1 0.54 0.25",
    "scatterv",      "0.52 0 0.52",
    "scatter",       "1 0.74 0.54",

    "computing",     "0 1 1",
    "sleeping",      "0 0.5 0.5",

    "init",       "0 1 0",
    "finalize",     "0 1 0",

    "put",       "0.3 1 0",
    "get",       "0 1 0.3",
    "accumulate",       "1 0.3 0",
    "win_fence",       "1 0 0.3",
    "win_post",       "1 0 0.8",
    "win_wait",       "1 0.8 0",
    "win_start",       "0.8 0 1",
    "win_complete",       "0.8 1 0",
    nullptr, nullptr,
};

static char *str_tolower (const char *str)
{
  char *ret = xbt_strdup (str);
  int i, n = strlen (ret);
  for (i = 0; i < n; i++)
    ret[i] = tolower (str[i]);
  return ret;
}

static const char *instr_find_color (const char *state)
{
  char *target = str_tolower (state);
  const char *ret = nullptr;
  unsigned int i = 0;
  const char *current = smpi_colors[i];
  while ((current != nullptr)){
    if (strcmp (state, current) == 0 //exact match
        || strstr(target, current) != 0 ){//as substring
         ret = smpi_colors[i+1]; 
         break; 
    }
    i+=2;
    current = smpi_colors[i];
  }
  xbt_free(target);
  return ret;
}

XBT_PRIVATE char *smpi_container(int rank, char *container, int n)
{
  snprintf(container, n, "rank-%d", rank);
  return container;
}

static char *TRACE_smpi_get_key(int src, int dst, char *key, int n);

static char *TRACE_smpi_put_key(int src, int dst, char *key, int n)
{
  //get the dynar for src#dst
  char aux[INSTR_DEFAULT_STR_SIZE];
  snprintf(aux, INSTR_DEFAULT_STR_SIZE, "r%d#%d", src, dst);
  xbt_dynar_t d = static_cast<xbt_dynar_t>(xbt_dict_get_or_null(keys, aux));

  if(xbt_dynar_is_empty(d) == 0){
    //receive was already pushed, perform a get instead
    TRACE_smpi_get_key(src , dst, key ,n);
    return key;
  }

  if (d == nullptr) {
    d = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
    snprintf(aux, INSTR_DEFAULT_STR_SIZE, "s%d#%d", src, dst);
    xbt_dict_set(keys, aux, d, nullptr);
  }

  //generate the key
  static unsigned long long counter = 0;
  counter++;
  snprintf(key, n, "%d_%d_%llu", src, dst, counter);

  //push it
  char *a = static_cast<char*> (xbt_strdup(key));
  xbt_dynar_push_as(d, char *, a);

  return key;
}

static char *TRACE_smpi_get_key(int src, int dst, char *key, int n)
{
  char aux[INSTR_DEFAULT_STR_SIZE];
  snprintf(aux, INSTR_DEFAULT_STR_SIZE, "s%d#%d", src, dst);
  xbt_dynar_t d = static_cast<xbt_dynar_t>(xbt_dict_get_or_null(keys, aux));

  // sometimes the receive may be posted before the send
  if(xbt_dynar_is_empty(d)){
      TRACE_smpi_put_key(src, dst, key, n);
      return key;
  }

  char *s = xbt_dynar_get_as (d, 0, char *);
  snprintf (key, n, "%s", s);
  xbt_dynar_remove_at (d, 0, nullptr);
  return key;
}

static xbt_dict_t process_category;

static void cleanup_extra_data (instr_extra_data extra){
  if(extra!=nullptr){
    if(extra->sendcounts!=nullptr)
      xbt_free(extra->sendcounts);
    if(extra->recvcounts!=nullptr)
      xbt_free(extra->recvcounts);
    xbt_free(extra);
  }
}

void TRACE_internal_smpi_set_category (const char *category)
{
  if (!TRACE_smpi_is_enabled())
    return;

  //declare category
  TRACE_category (category);

  char processid[INSTR_DEFAULT_STR_SIZE];
  snprintf (processid, INSTR_DEFAULT_STR_SIZE, "%p", SIMIX_process_self());
  if (xbt_dict_get_or_null (process_category, processid))
    xbt_dict_remove (process_category, processid);
  if (category != nullptr)
    xbt_dict_set (process_category, processid, xbt_strdup(category), nullptr);
}

const char *TRACE_internal_smpi_get_category ()
{
  if (!TRACE_smpi_is_enabled())
    return nullptr;

  char processid[INSTR_DEFAULT_STR_SIZE];
  snprintf (processid, INSTR_DEFAULT_STR_SIZE, "%p", SIMIX_process_self());
  return static_cast<char*>(xbt_dict_get_or_null (process_category, processid));
}

void TRACE_smpi_alloc()
{
  keys = xbt_dict_new_homogeneous(xbt_dynar_free_voidp);
  process_category = xbt_dict_new_homogeneous(xbt_free_f);
}

void TRACE_smpi_release()
{
  xbt_dict_free(&keys);
  xbt_dict_free(&process_category);
}

void TRACE_smpi_init(int rank)
{
  if (!TRACE_smpi_is_enabled())
    return;

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);

  container_t father;
  if (TRACE_smpi_is_grouped()){
    father = PJ_container_get (SIMIX_host_self_get_name());
  }else{
    father = PJ_container_get_root ();
  }
  xbt_assert(father!=nullptr,
      "Could not find a parent for mpi rank %s at function %s", str, __FUNCTION__);
#if HAVE_PAPI
  container_t container =
#endif
      PJ_container_new(str, INSTR_SMPI, father);
#if HAVE_PAPI
  papi_counter_t counters = smpi_process_papi_counters();

  for (auto& it : counters) {
    /**
     * Check whether this variable already exists or not. Otherwise, it will be created
     * multiple times but only the last one would be used...
     */
    if (PJ_type_get_or_null(it.first.c_str(), container->type) == nullptr) {
      PJ_type_variable_new(it.first.c_str(), nullptr, container->type);
    }
  }
#endif
}

void TRACE_smpi_finalize(int rank)
{
  if (!TRACE_smpi_is_enabled())
    return;

  char str[INSTR_DEFAULT_STR_SIZE];
  container_t container = PJ_container_get(smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE));
  PJ_container_remove_from_parent (container);
  PJ_container_free (container);
}

void TRACE_smpi_collective_in(int rank, int root, const char *operation, instr_extra_data extra)
{
  if (!TRACE_smpi_is_enabled()) {
      cleanup_extra_data(extra);
      return;
  }

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = PJ_container_get (str);
  type_t type = PJ_type_get ("MPI_STATE", container->type);
  const char *color = instr_find_color (operation);
  val_t value = PJ_value_get_or_new (operation, color, type);
   new_pajePushStateWithExtra (SIMIX_get_clock(), container, type, value, static_cast<void*>(extra));
}

void TRACE_smpi_collective_out(int rank, int root, const char *operation)
{
  if (!TRACE_smpi_is_enabled()) 
    return;

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = PJ_container_get (str);
  type_t type = PJ_type_get ("MPI_STATE", container->type);

  new_pajePopState (SIMIX_get_clock(), container, type);
}

void TRACE_smpi_computing_init(int rank)
{
 //first use, initialize the color in the trace
  if (!TRACE_smpi_is_enabled() || !TRACE_smpi_is_computing()) 
    return;

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = PJ_container_get (str);
  type_t type = PJ_type_get ("MPI_STATE", container->type);
  const char *color = instr_find_color ("computing");
  val_t value = PJ_value_get_or_new ("computing", color, type);
  new_pajePushState (SIMIX_get_clock(), container, type, value);
}

void TRACE_smpi_computing_in(int rank, instr_extra_data extra)
{
  //do not forget to set the color first, otherwise this will explode
  if (!TRACE_smpi_is_enabled()|| !TRACE_smpi_is_computing()) {
      cleanup_extra_data(extra);
      return;
  }

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = PJ_container_get (str);
  type_t type = PJ_type_get ("MPI_STATE", container->type);
  val_t value = PJ_value_get_or_new ("computing", nullptr, type);
  new_pajePushStateWithExtra  (SIMIX_get_clock(), container, type, value, static_cast<void*>(extra));
}

void TRACE_smpi_computing_out(int rank)
{
  if (!TRACE_smpi_is_enabled()|| !TRACE_smpi_is_computing())
    return;
  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = PJ_container_get (str);
  type_t type = PJ_type_get ("MPI_STATE", container->type);
  new_pajePopState (SIMIX_get_clock(), container, type);
}

void TRACE_smpi_sleeping_init(int rank)
{
  //first use, initialize the color in the trace
  if (!TRACE_smpi_is_enabled() || !TRACE_smpi_is_sleeping())
    return;

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = PJ_container_get (str);
  type_t type = PJ_type_get ("MPI_STATE", container->type);
  const char *color = instr_find_color ("sleeping");
  val_t value = PJ_value_get_or_new ("sleeping", color, type);
  new_pajePushState (SIMIX_get_clock(), container, type, value);
}

void TRACE_smpi_sleeping_in(int rank, instr_extra_data extra)
{
  //do not forget to set the color first, otherwise this will explode
  if (!TRACE_smpi_is_enabled()|| !TRACE_smpi_is_sleeping()) {
      cleanup_extra_data(extra);
      return;
  }

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = PJ_container_get (str);
  type_t type = PJ_type_get ("MPI_STATE", container->type);
  val_t value = PJ_value_get_or_new ("sleeping", nullptr, type);
  new_pajePushStateWithExtra  (SIMIX_get_clock(), container, type, value, static_cast<void*>(extra));
}

void TRACE_smpi_sleeping_out(int rank)
{
  if (!TRACE_smpi_is_enabled()|| !TRACE_smpi_is_sleeping())
    return;
  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = PJ_container_get (str);
  type_t type = PJ_type_get ("MPI_STATE", container->type);
  new_pajePopState (SIMIX_get_clock(), container, type);
}

void TRACE_smpi_testing_in(int rank, instr_extra_data extra)
{
  //do not forget to set the color first, otherwise this will explode
  if (!TRACE_smpi_is_enabled()) {
      cleanup_extra_data(extra);
      return;
  }

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = PJ_container_get (str);
  type_t type = PJ_type_get ("MPI_STATE", container->type);
  val_t value = PJ_value_get_or_new ("test", nullptr, type);
  new_pajePushStateWithExtra  (SIMIX_get_clock(), container, type, value, static_cast<void*>(extra));
}

void TRACE_smpi_testing_out(int rank)
{
  if (!TRACE_smpi_is_enabled())
    return;
  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = PJ_container_get (str);
  type_t type = PJ_type_get ("MPI_STATE", container->type);
  new_pajePopState (SIMIX_get_clock(), container, type);
}

void TRACE_smpi_ptp_in(int rank, int src, int dst, const char *operation, instr_extra_data extra)
{
  if (!TRACE_smpi_is_enabled()) {
      cleanup_extra_data(extra);
      return;
  }

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = PJ_container_get (str);
  type_t type = PJ_type_get ("MPI_STATE", container->type);
  const char *color = instr_find_color (operation);
  val_t value = PJ_value_get_or_new (operation, color, type);
  new_pajePushStateWithExtra (SIMIX_get_clock(), container, type, value, static_cast<void*>(extra));
}

void TRACE_smpi_ptp_out(int rank, int src, int dst, const char *operation)
{
  if (!TRACE_smpi_is_enabled())
    return;

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = PJ_container_get (str);
  type_t type = PJ_type_get ("MPI_STATE", container->type);

  new_pajePopState (SIMIX_get_clock(), container, type);
}

void TRACE_smpi_send(int rank, int src, int dst, int size)
{
  if (!TRACE_smpi_is_enabled())
    return;

  char key[INSTR_DEFAULT_STR_SIZE] = {0};
  TRACE_smpi_put_key(src, dst, key, INSTR_DEFAULT_STR_SIZE);

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(src, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = PJ_container_get (str);
  type_t type = PJ_type_get ("MPI_LINK", PJ_type_get_root());
  XBT_DEBUG("Send tracing from %d to %d, with key %s", src, dst, key);
  new_pajeStartLinkWithSize (SIMIX_get_clock(), PJ_container_get_root(), type, container, "PTP", key, size);
}

void TRACE_smpi_recv(int rank, int src, int dst)
{
  if (!TRACE_smpi_is_enabled())
    return;

  char key[INSTR_DEFAULT_STR_SIZE] = {0};
  TRACE_smpi_get_key(src, dst, key, INSTR_DEFAULT_STR_SIZE);

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(dst, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = PJ_container_get (str);
  type_t type = PJ_type_get ("MPI_LINK", PJ_type_get_root());
  XBT_DEBUG("Recv tracing from %d to %d, with key %s", src, dst, key);
  new_pajeEndLink (SIMIX_get_clock(), PJ_container_get_root(), type, container, "PTP", key);
}

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
    
    "migration",	"0.2 0.5 0.2",
    
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

char *smpi_container(int rank, char *container, int n)
{
  if(smpi_process_get_replaying()){
    snprintf(container, n, "%s-rank-%d", SIMIX_host_self_get_name(), rank);
  /*TODO We may need a configuration flag to enable or disable the tracing of
   * the migration of processes/tasks.*/
  }else{
    snprintf(container, n, "rank-%d", rank);
  }
  return container;
}

static char *TRACE_smpi_get_key(int src, int dst, int tag, char *key, int n, int send);

static char *TRACE_smpi_put_key(int src, int dst, int tag, char *key, int n, int send)
{
  //get the dynar for src#dst
  char aux[INSTR_DEFAULT_STR_SIZE];
  snprintf(aux, INSTR_DEFAULT_STR_SIZE, "%d#%d#%d#%d", src, dst, tag, send);
  xbt_dynar_t d = static_cast<xbt_dynar_t>(xbt_dict_get_or_null(keys, aux));

  if (d == nullptr) {
    d = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
    xbt_dict_set(keys, aux, d, nullptr);
  }

  //generate the key
  static unsigned long long counter = 0;
  counter++;
  snprintf(key, n, "%d_%d_%d_%llu", src, dst, tag, counter);

  //push it
  char *a = static_cast<char*> (xbt_strdup(key));
  xbt_dynar_push_as(d, char *, a);

  return key;
}

static char *TRACE_smpi_get_key(int src, int dst, int tag, char *key, int n, int send)
{
  char aux[INSTR_DEFAULT_STR_SIZE];
  snprintf(aux, INSTR_DEFAULT_STR_SIZE, "%d#%d#%d#%d", src, dst, tag, send==1?0:1);
  xbt_dynar_t d = static_cast<xbt_dynar_t>(xbt_dict_get_or_null(keys, aux));

  // first posted
  if(xbt_dynar_is_empty(d)){
      TRACE_smpi_put_key(src, dst, tag, key, n, send);
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
      "Could not find a parent for mpi rank %s", str);
  container_t me = PJ_container_new(str, INSTR_SMPI, father);
  type_t eventype = PJ_type_event_new("MPI_Migrate", me->type);

  val_t val = PJ_value_new ("migrate", "0 0 0", eventype);
#if HAVE_PAPI
  container_t container = me;

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

void TRACE_smpi_computing_init(int rank, instr_extra_data extra)
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
  if(extra != NULL){
    new_pajePushStateWithExtra(SIMIX_get_clock(), container, type, value,
      (void *)extra);//we need the extra data to keep track of states.
  }else{
    new_pajePushState(SIMIX_get_clock(), container, type, value);
  }
  new_pajePopState (SIMIX_get_clock(), container, type);
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

void TRACE_smpi_send(int rank, int src, int dst, int tag, int size)
{
  if (!TRACE_smpi_is_enabled())
    return;

  char key[INSTR_DEFAULT_STR_SIZE] = {0};
  TRACE_smpi_get_key(src, dst, tag, key, INSTR_DEFAULT_STR_SIZE,1);

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(src, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = PJ_container_get (str);
  type_t type = PJ_type_get ("MPI_LINK", PJ_type_get_root());
  XBT_DEBUG("Send tracing from %d to %d, tag %d, with key %s", src, dst, tag, key);
  new_pajeStartLinkWithSize (SIMIX_get_clock(), PJ_container_get_root(), type, container, "PTP", key, size);
}

void TRACE_smpi_recv(int rank, int src, int dst, int tag)
{
  if (!TRACE_smpi_is_enabled())
    return;

  char key[INSTR_DEFAULT_STR_SIZE] = {0};
  TRACE_smpi_get_key(src, dst, tag, key, INSTR_DEFAULT_STR_SIZE,0);

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(dst, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = PJ_container_get (str);
  type_t type = PJ_type_get ("MPI_LINK", PJ_type_get_root());
  XBT_DEBUG("Recv tracing from %d to %d, tag %d, with key %s", src, dst, tag, key);
  new_pajeEndLink (SIMIX_get_clock(), PJ_container_get_root(), type, container, "PTP", key);
}

/**************** Functions to trace the migration of tasks. *****************/

void TRACE_smpi_task_migrate(int rank, sg_host_t host)
{
  if (!TRACE_smpi_is_enabled()) return;
  
  char str[INSTR_DEFAULT_STR_SIZE];
  snprintf(str, INSTR_DEFAULT_STR_SIZE, "%s-rank-%d", sg_host_get_name(host), rank);
  
  if(PJ_container_get_or_null(str)){
    return;
  }

  container_t father;
  if (TRACE_smpi_is_grouped()){
    father = PJ_container_get(sg_host_get_name(host));
  }else{
    father = PJ_container_get_root();
  }
  xbt_assert(father!=NULL,
	      "Could not find a parent for mpi rank %s at function %s", str,
	      __FUNCTION__);
  PJ_container_new(str, INSTR_SMPI, father);
}

long long int get_migration_counter()
{

  static long long int counter = -1;
  static unsigned phase = 0;
  if(phase == 0){ // Data migration phase (smpi_replay_send_process_data)
    phase = 1;
    counter++;
  }else{ // Process remapping phase (smpi_replay_process_migrate)
    phase = 0;
  }
  return counter;
}

void TRACE_smpi_send_process_data_in(int rank)
{
  if (!TRACE_smpi_is_enabled()) {
      return;
  }


  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = PJ_container_get(str);
  
  /* Now we change the container's state to indicate that the process data is
   * being transferred. */ 
  type_t type = PJ_type_get ("MIGRATE_STATE", container->type);
  const char *color = instr_find_color ("migration");
  val_t value = PJ_value_get_or_new ("migration", color, type);
  new_pajePushState(SIMIX_get_clock(), container, type, value);
}

void TRACE_smpi_send_process_data_out(int rank)
{
  if (!TRACE_smpi_is_enabled()) {
      return;
  }

  char str[INSTR_DEFAULT_STR_SIZE];
  char key[INSTR_DEFAULT_STR_SIZE];
  long long int counter = get_migration_counter();
 
  /* Clean the process state. */ 
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = PJ_container_get(str);
  type_t type = PJ_type_get ("MIGRATE_STATE", container->type);
  new_pajePopState(SIMIX_get_clock(), container, type);
  
  /* This should start a link from the original process's container to the new
   * one. */
  snprintf(key, INSTR_DEFAULT_STR_SIZE, "%lld", counter);
  type = PJ_type_get ("MIGRATE_LINK", PJ_type_get_root());
  new_pajeStartLink(SIMIX_get_clock(), PJ_container_get_root(),
  		      type, container, "MIG", key);

}

void TRACE_smpi_process_change_host(int rank, sg_host_t host,
				    sg_host_t new_host, int size)
{
  if (!TRACE_smpi_is_enabled()) return;
  
  long long int counter = get_migration_counter();

  char key[INSTR_DEFAULT_STR_SIZE];
  char str[INSTR_DEFAULT_STR_SIZE];

  //Create new container on the new_host location, if it doesn't already exist.
  TRACE_smpi_task_migrate(rank, new_host);

  //This should end the link from the original to the new container.
  snprintf (key, INSTR_DEFAULT_STR_SIZE, "%lld", counter);
  //We can't call smpi_cointainer() because we need the link to end on the new
  //host.
  snprintf(str, INSTR_DEFAULT_STR_SIZE, "%s-rank-%d",
	    sg_host_get_name(new_host), rank);
  container_t container = PJ_container_get(str);
  type_t type = PJ_type_get("MIGRATE_LINK", PJ_type_get_root());
  new_pajeEndLink(SIMIX_get_clock(), PJ_container_get_root(),
		    type, container, "MIG", key);

}


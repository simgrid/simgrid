/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_smpi, instr, "Tracing SMPI");

static xbt_dict_t keys;

static char *smpi_container(int rank, char *container, int n)
{
  snprintf(container, n, "rank-%d", rank);
  return container;
}

static char *TRACE_smpi_put_key(int src, int dst, char *key, int n)
{
  //get the dynar for src#dst
  char aux[INSTR_DEFAULT_STR_SIZE];
  snprintf(aux, INSTR_DEFAULT_STR_SIZE, "%d#%d", src, dst);
  xbt_dynar_t d = xbt_dict_get_or_null(keys, aux);
  if (d == NULL) {
    d = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
    xbt_dict_set(keys, aux, d, xbt_free);
  }
  //generate the key
  static unsigned long long counter = 0;
  snprintf(key, n, "%d%d%lld", src, dst, counter++);

  //push it
  char *a = (char*)xbt_strdup(key);
  xbt_dynar_push_as(d, char *, a);

  return key;
}

static char *TRACE_smpi_get_key(int src, int dst, char *key, int n)
{
  char aux[INSTR_DEFAULT_STR_SIZE];
  snprintf(aux, INSTR_DEFAULT_STR_SIZE, "%d#%d", src, dst);
  xbt_dynar_t d = xbt_dict_get_or_null(keys, aux);

  int length = xbt_dynar_length(d);
  char *s = xbt_dynar_get_as (d, length-1, char *);
  snprintf (key, n, "%s", s);
  xbt_dynar_remove_at (d, length-1, NULL);
  return key;
}

static xbt_dict_t process_category;

void TRACE_internal_smpi_set_category (const char *category)
{
  if (!TRACE_smpi_is_enabled()) return;

  //declare category
  TRACE_category (category);

  char processid[INSTR_DEFAULT_STR_SIZE];
  snprintf (processid, INSTR_DEFAULT_STR_SIZE, "%p", SIMIX_process_self());
  if (xbt_dict_get_or_null (process_category, processid))
    xbt_dict_remove (process_category, processid);
  if (category != NULL)
    xbt_dict_set (process_category, processid, xbt_strdup(category), xbt_free);
}

const char *TRACE_internal_smpi_get_category (void)
{
  if (!TRACE_smpi_is_enabled()) return NULL;

  char processid[INSTR_DEFAULT_STR_SIZE];
  snprintf (processid, INSTR_DEFAULT_STR_SIZE, "%p", SIMIX_process_self());
  return xbt_dict_get_or_null (process_category, processid);
}

void TRACE_smpi_alloc()
{
  keys = xbt_dict_new();
  process_category = xbt_dict_new();
}

void TRACE_smpi_start(void)
{
  if (TRACE_smpi_is_enabled()) {
    TRACE_start();
  }
}

void TRACE_smpi_release(void)
{
  TRACE_surf_release();
  if (TRACE_smpi_is_enabled()) {
    TRACE_end();
  }
}

void TRACE_smpi_init(int rank)
{
  if (!TRACE_smpi_is_enabled())
    return;

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);

  container_t father;
  if (TRACE_smpi_is_grouped()){
    father = getContainer (SIMIX_host_self_get_name());
  }else{
    father = getContainer ("0");
  }
  xbt_assert2(father!=NULL,
      "Could not find a parent for mpi rank %s at function %s", str, __FUNCTION__);
  newContainer(str, INSTR_SMPI, father);
}

void TRACE_smpi_finalize(int rank)
{
  if (!TRACE_smpi_is_enabled())
    return;

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  destroyContainer(getContainer (str));
}

void TRACE_smpi_collective_in(int rank, int root, const char *operation)
{
  if (!TRACE_smpi_is_enabled())
    return;

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = getContainer (str);
  type_t type = getType ("MPI_STATE");

  pajePushState(SIMIX_get_clock(), type->id, container->id, operation);
}

void TRACE_smpi_collective_out(int rank, int root, const char *operation)
{
  if (!TRACE_smpi_is_enabled())
    return;

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = getContainer (str);
  type_t type = getType ("MPI_STATE");

  pajePopState(SIMIX_get_clock(), type->id, container->id);
}

void TRACE_smpi_ptp_in(int rank, int src, int dst, const char *operation)
{
  if (!TRACE_smpi_is_enabled())
    return;

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = getContainer (str);
  type_t type = getType ("MPI_STATE");

  pajePushState(SIMIX_get_clock(), type->id, container->id, operation);
}

void TRACE_smpi_ptp_out(int rank, int src, int dst, const char *operation)
{
  if (!TRACE_smpi_is_enabled())
    return;

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = getContainer (str);
  type_t type = getType ("MPI_STATE");

  pajePopState(SIMIX_get_clock(), type->id, container->id);
}

void TRACE_smpi_send(int rank, int src, int dst)
{
  if (!TRACE_smpi_is_enabled())
    return;

  char key[INSTR_DEFAULT_STR_SIZE];
  TRACE_smpi_put_key(src, dst, key, INSTR_DEFAULT_STR_SIZE);

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(src, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = getContainer (str);
  type_t type = getType ("MPI_LINK");

  pajeStartLink(SIMIX_get_clock(), type->id, type->father->id, "PTP", container->id, key);
}

void TRACE_smpi_recv(int rank, int src, int dst)
{
  if (!TRACE_smpi_is_enabled())
    return;

  char key[INSTR_DEFAULT_STR_SIZE];
  TRACE_smpi_get_key(src, dst, key, INSTR_DEFAULT_STR_SIZE);

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(dst, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = getContainer (str);
  type_t type = getType ("MPI_LINK");

  pajeEndLink(SIMIX_get_clock(), type->id, type->father->id, "PTP", container->id, key);
}
#endif /* HAVE_TRACING */

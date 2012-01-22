/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"
#include <ctype.h>
#include <wchar.h>


#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_smpi, instr, "Tracing SMPI");

static xbt_dict_t keys;

static const char *smpi_colors[] ={
    "recv",     "255 000 000",
    "irecv",    "255 135 135",
    "send",     "000 000 255",
    "isend",    "135 135 255",
    "sendrecv", "000 255 255",
    "wait",     "255 255 000",
    "waitall",  "200 200 000",
    "waitany",  "200 200 150",

    "allgather",     "255 000 000",
    "allgatherv",    "255 135 135",
    "allreduce",     "255 000 255",
    "alltoall",      "135 000 255",
    "alltoallv",     "200 135 255",
    "barrier",       "000 200 200",
    "bcast",         "000 200 100",
    "gather",        "255 255 000",
    "gatherv",       "255 255 135",
    "reduce",        "000 255 000",
    "reducescatter", "135 255 135",
    "scan",          "255 150 060",
    "scatterv",      "135 000 135",
    "scatter",       "255 190 140",

    NULL, NULL,
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
  const char *ret = NULL;
  const char *current;
  unsigned int i = 0;
  while ((current = smpi_colors[i])){
    if (strcmp (state, current) == 0){ ret = smpi_colors[i+1]; break; } //exact match
    if (strstr(target, current)) { ret = smpi_colors[i+1]; break; }; //as substring
    i+=2;
  }
  free (target);
  return ret;
}


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
    xbt_dict_set(keys, aux, d, NULL);
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

  xbt_assert(!xbt_dynar_is_empty(d),
      "Trying to get a link key (for message reception) that has no corresponding send (%s).", __FUNCTION__);
  char *s = xbt_dynar_get_as (d, 0, char *);
  snprintf (key, n, "%s", s);
  xbt_dynar_remove_at (d, 0, NULL);
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
    xbt_dict_set (process_category, processid, xbt_strdup(category), NULL);
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
  keys = xbt_dict_new_homogeneous(xbt_free);
  process_category = xbt_dict_new_homogeneous(xbt_free);
}

void TRACE_smpi_release(void)
{
  xbt_dict_free(&keys);
  xbt_dict_free(&process_category);
}

void TRACE_smpi_init(int rank)
{
  if (!TRACE_smpi_is_enabled()) return;

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);

  container_t father;
  if (TRACE_smpi_is_grouped()){
    father = getContainer (SIMIX_host_self_get_name());
  }else{
    father = getRootContainer ();
  }
  xbt_assert(father!=NULL,
      "Could not find a parent for mpi rank %s at function %s", str, __FUNCTION__);
  newContainer(str, INSTR_SMPI, father);
}

void TRACE_smpi_finalize(int rank)
{
  if (!TRACE_smpi_is_enabled()) return;

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  destroyContainer(getContainer (str));
}

void TRACE_smpi_collective_in(int rank, int root, const char *operation)
{
  if (!TRACE_smpi_is_enabled()) return;

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = getContainer (str);
  type_t type = getType ("MPI_STATE", container->type);
  const char *color = instr_find_color (operation);
  val_t value = getValue (operation, color, type);

  new_pajePushState (SIMIX_get_clock(), container, type, value);
}

void TRACE_smpi_collective_out(int rank, int root, const char *operation)
{
  if (!TRACE_smpi_is_enabled()) return;

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = getContainer (str);
  type_t type = getType ("MPI_STATE", container->type);

  new_pajePopState (SIMIX_get_clock(), container, type);
}

void TRACE_smpi_ptp_in(int rank, int src, int dst, const char *operation)
{
  if (!TRACE_smpi_is_enabled()) return;


  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = getContainer (str);
  type_t type = getType ("MPI_STATE", container->type);
  const char *color = instr_find_color (operation);
  val_t value = getValue (operation, color, type);

  new_pajePushState (SIMIX_get_clock(), container, type, value);
}

void TRACE_smpi_ptp_out(int rank, int src, int dst, const char *operation)
{
  if (!TRACE_smpi_is_enabled()) return;

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(rank, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = getContainer (str);
  type_t type = getType ("MPI_STATE", container->type);

  new_pajePopState (SIMIX_get_clock(), container, type);
}

void TRACE_smpi_send(int rank, int src, int dst)
{
  if (!TRACE_smpi_is_enabled()) return;

  char key[INSTR_DEFAULT_STR_SIZE];
  TRACE_smpi_put_key(src, dst, key, INSTR_DEFAULT_STR_SIZE);

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(src, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = getContainer (str);
  type_t type = getType ("MPI_LINK", getRootType());

  new_pajeStartLink (SIMIX_get_clock(), getRootContainer(), type, container, "PTP", key);
}

void TRACE_smpi_recv(int rank, int src, int dst)
{
  if (!TRACE_smpi_is_enabled()) return;

  char key[INSTR_DEFAULT_STR_SIZE];
  TRACE_smpi_get_key(src, dst, key, INSTR_DEFAULT_STR_SIZE);

  char str[INSTR_DEFAULT_STR_SIZE];
  smpi_container(dst, str, INSTR_DEFAULT_STR_SIZE);
  container_t container = getContainer (str);
  type_t type = getType ("MPI_LINK", getRootType());

  new_pajeEndLink (SIMIX_get_clock(), getRootContainer(), type, container, "PTP", key);
}
#endif /* HAVE_TRACING */

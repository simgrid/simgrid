/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/private.h"

#ifdef HAVE_TRACING

static xbt_dict_t keys;

static char *TRACE_smpi_container (int rank, char *container, int n)
{
  snprintf (container, n, "rank-%d", rank);
  return container;
}

static char *TRACE_smpi_put_key (int src, int dst, char *key, int n)
{
  //get the dynar for src#dst
  char aux[100];
  snprintf (aux, 100, "%d#%d", src, dst);
  xbt_dynar_t d = xbt_dict_get_or_null (keys, aux);
  if (d == NULL){
    d = xbt_dynar_new (sizeof(char*), xbt_free);
    xbt_dict_set (keys, aux, d, xbt_free);
  }
  //generate the key
  static long long counter = 0;
  snprintf (key, n, "%d%d%lld", src, dst, counter++);

  xbt_dynar_insert_at (d, 0, xbt_strdup (key));
  return key;
}

static char *TRACE_smpi_get_key (int src, int dst, char *key, int n)
{
  char aux[100];
  snprintf (aux, 100, "%d#%d", src, dst);
  xbt_dynar_t d = xbt_dict_get_or_null (keys, aux);

  int length = xbt_dynar_length (d);
  char stored_key[n];
  xbt_dynar_remove_at (d, length-1, stored_key);
  strncpy (key, stored_key, n);
  return key;
}

void TRACE_smpi_alloc ()
{
  keys = xbt_dict_new();
}

void TRACE_smpi_start (void)
{
  if (IS_TRACING_SMPI){
    TRACE_start ();
  }
}

void TRACE_smpi_release (void)
{
  TRACE_surf_release ();
  if (IS_TRACING_SMPI){
    TRACE_end();
  }
}

void TRACE_smpi_init (int rank)
{
  if (!IS_TRACING_SMPI) return;

  char str[100];
  TRACE_smpi_container (rank, str, 100);
  pajeCreateContainer (SIMIX_get_clock(), str, "MPI_PROCESS",
      SIMIX_host_get_name(SIMIX_host_self()), str);
}

void TRACE_smpi_finalize (int rank)
{
  if (!IS_TRACING_SMPI) return;

  char str[100];
  pajeDestroyContainer (SIMIX_get_clock(), "MPI_PROCESS",
      TRACE_smpi_container (rank, str, 100));
}

void TRACE_smpi_collective_in (int rank, int root, const char *operation)
{
  if (!IS_TRACING_SMPI) return;

  char str[100];
  pajePushState (SIMIX_get_clock(), "MPI_STATE",
      TRACE_smpi_container (rank, str, 100), operation);
}

void TRACE_smpi_collective_out (int rank, int root, const char *operation)
{
  if (!IS_TRACING_SMPI) return;

  char str[100];
  pajePopState (SIMIX_get_clock(), "MPI_STATE",
      TRACE_smpi_container (rank, str, 100));
}

void TRACE_smpi_ptp_in (int rank, int src, int dst, const char *operation)
{
  if (!IS_TRACING_SMPI) return;

  char str[100];
  pajePushState (SIMIX_get_clock(), "MPI_STATE",
      TRACE_smpi_container (rank, str, 100), operation);
}

void TRACE_smpi_ptp_out (int rank, int src, int dst, const char *operation)
{
  if (!IS_TRACING_SMPI) return;

  char str[100];
  pajePopState (SIMIX_get_clock(), "MPI_STATE",
      TRACE_smpi_container (rank, str, 100));
}

void TRACE_smpi_send (int rank, int src, int dst)
{
  if (!IS_TRACING_SMPI) return;

  char key[100], str[100];
  TRACE_smpi_put_key (src, dst, key, 100);
  pajeStartLink (SIMIX_get_clock(), "MPI_LINK", "0", "PTP",
      TRACE_smpi_container (src, str, 100), key);
}

void TRACE_smpi_recv (int rank, int src, int dst)
{
  if (!IS_TRACING_SMPI) return;

  char key[100], str[100];
  TRACE_smpi_get_key (src, dst, key, 100);
  pajeEndLink (SIMIX_get_clock(), "MPI_LINK", "0", "PTP",
      TRACE_smpi_container (dst, str, 100), key);
}
#endif


/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/private.h"

#ifdef HAVE_TRACING

static char *_TRACE_smpi_container (int rank, char *container, int n)
{
  snprintf (container, n, "rank-%d", rank);
  return container;
}

void TRACE_smpi_start (void)
{
  if (IS_TRACING_SMPI){
    TRACE_start ();
  }
}

void TRACE_smpi_end (void)
{
  if (IS_TRACING_SMPI){
    TRACE_end();
  }
}

void TRACE_smpi_init (int rank)
{
  if (!IS_TRACING_SMPI) return;

  char str[100];
  _TRACE_smpi_container (rank, str, 100);
  pajeCreateContainer (SIMIX_get_clock(), str, "MPI_PROCESS",
      SIMIX_host_get_name(SIMIX_host_self()), str);
}

void TRACE_smpi_finalize (int rank)
{
  if (!IS_TRACING_SMPI) return;

  char str[100];
  pajeDestroyContainer (SIMIX_get_clock(), "MPI_PROCESS",
      _TRACE_smpi_container (rank, str, 100));
}

void TRACE_smpi_collective_in (int rank, int root, const char *operation)
{
  if (!IS_TRACING_SMPI) return;

  char str[100];
  pajePushState (SIMIX_get_clock(), "MPI_STATE",
      _TRACE_smpi_container (rank, str, 100), operation);
}

void TRACE_smpi_collective_out (int rank, int root, const char *operation)
{
  if (!IS_TRACING_SMPI) return;

  char str[100];
  pajePopState (SIMIX_get_clock(), "MPI_STATE",
      _TRACE_smpi_container (rank, str, 100));
}

#endif


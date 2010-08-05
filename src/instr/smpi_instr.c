/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/private.h"

#ifdef HAVE_TRACING

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
  //double time = SIMIX_get_clock();
  //smx_host_t host = SIMIX_host_self();
}

void TRACE_smpi_finalize (int rank)
{
  if (!IS_TRACING_SMPI) return;
  //double time = SIMIX_get_clock();
  //smx_host_t host = SIMIX_host_self();
}

#endif


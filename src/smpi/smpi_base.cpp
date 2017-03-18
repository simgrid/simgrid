/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/config.hpp>
#include <algorithm>

#include "private.h"
#include "xbt/virtu.h"
#include "mc/mc.h"
#include "src/mc/mc_replay.h"
#include <errno.h>
#include "src/simix/smx_private.h"
#include "surf/surf.h"
#include "simgrid/sg_config.h"
#include "smpi/smpi_utils.hpp"
#include <simgrid/s4u/host.hpp>

#include "src/kernel/activity/SynchroComm.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_base, smpi, "Logging specific to SMPI (base)");

static simgrid::config::Flag<double> smpi_wtime_sleep(
  "smpi/wtime", "Minimum time to inject inside a call to MPI_Wtime", 0.0);
static simgrid::config::Flag<double> smpi_init_sleep(
  "smpi/init", "Time to inject inside a call to MPI_Init", 0.0);

void smpi_mpi_init() {
  if(smpi_init_sleep > 0) 
    simcall_process_sleep(smpi_init_sleep);
}

double smpi_mpi_wtime(){
  double time;
  if (smpi_process()->initialized() != 0 && smpi_process()->finalized() == 0 && smpi_process()->sampling() == 0) {
    smpi_bench_end();
    time = SIMIX_get_clock();
    // to avoid deadlocks if used as a break condition, such as
    //     while (MPI_Wtime(...) < time_limit) {
    //       ....
    //     }
    // because the time will not normally advance when only calls to MPI_Wtime
    // are made -> deadlock (MPI_Wtime never reaches the time limit)
    if(smpi_wtime_sleep > 0) 
      simcall_process_sleep(smpi_wtime_sleep);
    smpi_bench_begin();
  } else {
    time = SIMIX_get_clock();
  }
  return time;
}

void smpi_empty_status(MPI_Status * status)
{
  if(status != MPI_STATUS_IGNORE) {
    status->MPI_SOURCE = MPI_ANY_SOURCE;
    status->MPI_TAG = MPI_ANY_TAG;
    status->MPI_ERROR = MPI_SUCCESS;
    status->count=0;
  }
}

int smpi_mpi_get_count(MPI_Status * status, MPI_Datatype datatype)
{
  return status->count / datatype->size();
}

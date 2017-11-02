/* Copyright (c) 2013-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cfloat>
#include <exception>

#include "colls_private.hpp"
#include "smpi_process.hpp"

//attempt to do a quick autotuning version of the collective,
#define TRACE_AUTO_COLL(cat)                                                                                           \
  if (TRACE_is_enabled()) {                                                                                            \
    simgrid::instr::EventType* type = simgrid::instr::Container::getRoot()->type_->getOrCreateEventType(#cat);         \
                                                                                                                       \
    std::string cont_name = std::string("rank-" + std::to_string(smpi_process()->index()));                            \
    type->addEntityValue(Colls::mpi_coll_##cat##_description[i].name, "1.0 1.0 1.0");                                  \
    new simgrid::instr::NewEvent(SIMIX_get_clock(), simgrid::instr::Container::byName(cont_name), type,                \
                                 type->getEntityValue(Colls::mpi_coll_##cat##_description[i].name));                   \
  }

#define AUTOMATIC_COLL_BENCH(cat, ret, args, args2)                                                                    \
  ret Coll_##cat##_automatic::cat(COLL_UNPAREN args)                                                                   \
  {                                                                                                                    \
    double time1, time2, time_min = DBL_MAX;                                                                           \
    int min_coll = -1, global_coll = -1;                                                                               \
    int i;                                                                                                             \
    double buf_in, buf_out, max_min = DBL_MAX;                                                                         \
    for (i = 0; Colls::mpi_coll_##cat##_description[i].name; i++) {                                                    \
      if (not strcmp(Colls::mpi_coll_##cat##_description[i].name, "automatic"))                                        \
        continue;                                                                                                      \
      if (not strcmp(Colls::mpi_coll_##cat##_description[i].name, "default"))                                          \
        continue;                                                                                                      \
      Coll_barrier_default::barrier(comm);                                                                             \
      TRACE_AUTO_COLL(cat)                                                                                             \
      time1 = SIMIX_get_clock();                                                                                       \
      try {                                                                                                            \
        ((int(*) args)Colls::mpi_coll_##cat##_description[i].coll) args2;                                              \
      } catch (std::exception & ex) {                                                                                  \
        continue;                                                                                                      \
      }                                                                                                                \
      time2   = SIMIX_get_clock();                                                                                     \
      buf_out = time2 - time1;                                                                                         \
      Coll_reduce_default::reduce((void*)&buf_out, (void*)&buf_in, 1, MPI_DOUBLE, MPI_MAX, 0, comm);                   \
      if (time2 - time1 < time_min) {                                                                                  \
        min_coll = i;                                                                                                  \
        time_min = time2 - time1;                                                                                      \
      }                                                                                                                \
      if (comm->rank() == 0) {                                                                                         \
        if (buf_in < max_min) {                                                                                        \
          max_min     = buf_in;                                                                                        \
          global_coll = i;                                                                                             \
        }                                                                                                              \
      }                                                                                                                \
    }                                                                                                                  \
    if (comm->rank() == 0) {                                                                                           \
      XBT_WARN("For rank 0, the quickest was %s : %f , but global was %s : %f at max",                                 \
               Colls::mpi_coll_##cat##_description[min_coll].name, time_min,                                           \
               Colls::mpi_coll_##cat##_description[global_coll].name, max_min);                                        \
    } else                                                                                                             \
      XBT_WARN("The quickest %s was %s on rank %d and took %f", #cat,                                                  \
               Colls::mpi_coll_##cat##_description[min_coll].name, comm->rank(), time_min);                            \
    return (min_coll != -1) ? MPI_SUCCESS : MPI_ERR_INTERN;                                                            \
  }

namespace simgrid{
namespace smpi{

COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_ALLGATHERV_SIG, (send_buff, send_count, send_type, recv_buff, recv_count, recv_disps, recv_type, comm));
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_ALLREDUCE_SIG, (sbuf, rbuf, rcount, dtype, op, comm));
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_GATHER_SIG, (send_buff, send_count, send_type, recv_buff, recv_count, recv_type, root, comm));
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_ALLGATHER_SIG, (send_buff,send_count,send_type,recv_buff,recv_count,recv_type,comm));
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_ALLTOALL_SIG,(send_buff, send_count, send_type, recv_buff, recv_count, recv_type,comm));
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_ALLTOALLV_SIG, (send_buff, send_counts, send_disps, send_type, recv_buff, recv_counts, recv_disps, recv_type, comm));
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_BCAST_SIG , (buf, count, datatype, root, comm));
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_REDUCE_SIG,(buf,rbuf, count, datatype, op, root, comm));
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_REDUCE_SCATTER_SIG ,(sbuf,rbuf, rcounts,dtype,op,comm));
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_SCATTER_SIG ,(sendbuf, sendcount, sendtype,recvbuf, recvcount, recvtype,root, comm));
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_BARRIER_SIG,(comm));

}
}

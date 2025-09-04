/* Copyright (c) 2013-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cfloat>
#include <exception>

#include "colls_private.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "src/instr/instr_paje_types.hpp"
#include "src/smpi/include/smpi_actor.hpp"

//attempt to do a quick autotuning version of the collective,
#define AUTOMATIC_COLL_BENCH(cat, ret, args, args2)                                                                    \
  ret _XBT_CONCAT2(cat, __automatic)(COLL_UNPAREN args)                                                                \
  {                                                                                                                    \
    double time1, time2, time_min = DBL_MAX;                                                                           \
    int min_coll = -1, global_coll = -1;                                                                               \
    double buf_in, buf_out, max_min = DBL_MAX;                                                                         \
    auto descriptions = simgrid::smpi::colls::get_smpi_coll_descriptions(_XBT_STRINGIFY(cat));                         \
    for (unsigned long i = 0; i < descriptions->size(); i++) {                                                         \
      auto desc = &descriptions->at(i);                                                                                \
      if (desc->name == "automatic" || desc->name == "default")                                                        \
        continue;                                                                                                      \
      barrier__default(comm);                                                                                          \
      if (TRACE_is_enabled()) {                                                                                        \
        simgrid::instr::EventType* type =                                                                              \
            simgrid::instr::Container::get_root()->get_type()->by_name_or_create<simgrid::instr::EventType>(           \
                _XBT_STRINGIFY(cat));                                                                                  \
                                                                                                                       \
        std::string cont_name = "rank-" + std::to_string(simgrid::s4u::this_actor::get_pid());                         \
        type->add_entity_value(desc->name, "1.0 1.0 1.0");                                                             \
        new simgrid::instr::NewEvent(simgrid::s4u::Engine::get_clock(), simgrid::instr::Container::by_name(cont_name), \
                                     type, type->get_entity_value(desc->name));                                        \
      }                                                                                                                \
      time1 = simgrid::s4u::Engine::get_clock();                                                                       \
      try {                                                                                                            \
        ((int(*) args)desc->coll) args2;                                                                               \
      } catch (std::exception & ex) {                                                                                  \
        continue;                                                                                                      \
      }                                                                                                                \
      time2   = simgrid::s4u::Engine::get_clock();                                                                     \
      buf_out = time2 - time1;                                                                                         \
      reduce__default((void*)&buf_out, (void*)&buf_in, 1, MPI_DOUBLE, MPI_MAX, 0, comm);                               \
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
               descriptions->at(min_coll).name.c_str(), time_min, descriptions->at(global_coll).name.c_str(),          \
               max_min);                                                                                               \
    } else                                                                                                             \
      XBT_WARN("The quickest " _XBT_STRINGIFY(cat) " was %s on rank %d and took %f",                                   \
               descriptions->at(min_coll).name.c_str(), comm->rank(), time_min);                                       \
    return (min_coll != -1) ? MPI_SUCCESS : MPI_ERR_INTERN;                                                            \
  }

namespace simgrid::smpi {

COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_ALLGATHERV_SIG, (send_buff, send_count, send_type, recv_buff, recv_count, recv_disps, recv_type, comm))
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_ALLREDUCE_SIG, (sbuf, rbuf, rcount, dtype, op, comm))
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_GATHER_SIG, (send_buff, send_count, send_type, recv_buff, recv_count, recv_type, root, comm))
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_ALLGATHER_SIG, (send_buff,send_count,send_type,recv_buff,recv_count,recv_type,comm))
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_ALLTOALL_SIG,(send_buff, send_count, send_type, recv_buff, recv_count, recv_type,comm))
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_ALLTOALLV_SIG, (send_buff, send_counts, send_disps, send_type, recv_buff, recv_counts, recv_disps, recv_type, comm))
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_BCAST_SIG , (buf, count, datatype, root, comm))
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_REDUCE_SIG,(buf,rbuf, count, datatype, op, root, comm))
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_REDUCE_SCATTER_SIG ,(sbuf,rbuf, rcounts,dtype,op,comm))
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_SCATTER_SIG ,(sendbuf, sendcount, sendtype,recvbuf, recvcount, recvtype,root, comm))
COLL_APPLY(AUTOMATIC_COLL_BENCH, COLL_BARRIER_SIG,(comm))

} // namespace simgrid::smpi

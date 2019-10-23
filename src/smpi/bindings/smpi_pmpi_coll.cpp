/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "smpi_coll.hpp"
#include "smpi_comm.hpp"
#include "smpi_request.hpp"
#include "smpi_datatype_derived.hpp"
#include "smpi_op.hpp"
#include "src/smpi/include/smpi_actor.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smpi_pmpi);

#define CHECK_ARGS(test, errcode, ...)                                                                                 \
  if (test) {                                                                                                          \
    XBT_WARN(__VA_ARGS__);                                                                                             \
    return (errcode);                                                                                                  \
  }

/* PMPI User level calls */

int PMPI_Barrier(MPI_Comm comm)
{
  return PMPI_Ibarrier(comm, MPI_REQUEST_IGNORED);
}

int PMPI_Ibarrier(MPI_Comm comm, MPI_Request *request)
{
  if (comm == MPI_COMM_NULL)
    return MPI_ERR_COMM;
  if (request == nullptr)
    return MPI_ERR_ARG;

  smpi_bench_end();
  int rank = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(rank, request == MPI_REQUEST_IGNORED ? "PMPI_Barrier" : "PMPI_Ibarrier",
                     new simgrid::instr::NoOpTIData(request == MPI_REQUEST_IGNORED ? "barrier" : "ibarrier"));
  if (request == MPI_REQUEST_IGNORED) {
    simgrid::smpi::Colls::barrier(comm);
    // Barrier can be used to synchronize RMA calls. Finish all requests from comm before.
    comm->finish_rma_calls();
  } else
    simgrid::smpi::Colls::ibarrier(comm, request);

  TRACE_smpi_comm_out(rank);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Bcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
  return PMPI_Ibcast(buf, count, datatype, root, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Ibcast(void *buf, int count, MPI_Datatype datatype, 
                   int root, MPI_Comm comm, MPI_Request* request)
{
  if (comm == MPI_COMM_NULL)
    return MPI_ERR_COMM;
  if (buf == nullptr && count > 0)
    return MPI_ERR_BUFFER;
  if (datatype == MPI_DATATYPE_NULL || not datatype->is_valid())
    return MPI_ERR_TYPE;
  if (count < 0)
    return MPI_ERR_COUNT;
  if (root < 0 || root >= comm->size())
    return MPI_ERR_ROOT;
  if (request == nullptr)
    return MPI_ERR_ARG;

  smpi_bench_end();
  int rank = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(rank, request == MPI_REQUEST_IGNORED ? "PMPI_Bcast" : "PMPI_Ibcast",
                     new simgrid::instr::CollTIData(request == MPI_REQUEST_IGNORED ? "bcast" : "ibcast", root, -1.0,
                                                    datatype->is_replayable() ? count : count * datatype->size(), -1,
                                                    simgrid::smpi::Datatype::encode(datatype), ""));
  if (comm->size() > 1) {
    if (request == MPI_REQUEST_IGNORED)
      simgrid::smpi::Colls::bcast(buf, count, datatype, root, comm);
    else
      simgrid::smpi::Colls::ibcast(buf, count, datatype, root, comm, request);
  } else {
    if (request != MPI_REQUEST_IGNORED)
      *request = MPI_REQUEST_NULL;
  }

  TRACE_smpi_comm_out(rank);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount, MPI_Datatype recvtype,
                int root, MPI_Comm comm){
  return PMPI_Igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Igather(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                 MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request* request)
{
  if (comm == MPI_COMM_NULL)
    return MPI_ERR_COMM;
  if ((sendbuf == nullptr && sendcount > 0) || ((comm->rank() == root) && recvbuf == nullptr && recvcount > 0))
    return MPI_ERR_BUFFER;
  if (((sendbuf != MPI_IN_PLACE && sendcount > 0) && (sendtype == MPI_DATATYPE_NULL)) ||
      ((comm->rank() == root) && (recvtype == MPI_DATATYPE_NULL)))
    return MPI_ERR_TYPE;
  if (((sendbuf != MPI_IN_PLACE) && (sendcount < 0)) || ((comm->rank() == root) && (recvcount < 0)))
    return MPI_ERR_COUNT;
  if (root < 0 || root >= comm->size())
    return MPI_ERR_ROOT;
  if (request == nullptr)
    return MPI_ERR_ARG;

  smpi_bench_end();
  const void* real_sendbuf   = sendbuf;
  int real_sendcount         = sendcount;
  MPI_Datatype real_sendtype = sendtype;
  if ((comm->rank() == root) && (sendbuf == MPI_IN_PLACE)) {
    real_sendcount = 0;
    real_sendtype  = recvtype;
  }
  int rank = simgrid::s4u::this_actor::get_pid();

  TRACE_smpi_comm_in(rank, request == MPI_REQUEST_IGNORED ? "PMPI_Gather" : "PMPI_Igather",
                     new simgrid::instr::CollTIData(
                         request == MPI_REQUEST_IGNORED ? "gather" : "igather", root, -1.0,
                         real_sendtype->is_replayable() ? real_sendcount : real_sendcount * real_sendtype->size(),
                         (comm->rank() != root || recvtype->is_replayable()) ? recvcount : recvcount * recvtype->size(),
                         simgrid::smpi::Datatype::encode(real_sendtype), simgrid::smpi::Datatype::encode(recvtype)));
  if (request == MPI_REQUEST_IGNORED)
    simgrid::smpi::Colls::gather(real_sendbuf, real_sendcount, real_sendtype, recvbuf, recvcount, recvtype, root, comm);
  else
    simgrid::smpi::Colls::igather(real_sendbuf, real_sendcount, real_sendtype, recvbuf, recvcount, recvtype, root, comm,
                                  request);

  TRACE_smpi_comm_out(rank);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs,
                MPI_Datatype recvtype, int root, MPI_Comm comm){
  return PMPI_Igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Igatherv(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, const int* recvcounts, const int* displs,
                  MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request* request)
{
  if (comm == MPI_COMM_NULL)
    return MPI_ERR_COMM;
  if ((sendbuf == nullptr && sendcount > 0) || ((comm->rank() == root) && recvbuf == nullptr))
    return MPI_ERR_BUFFER;
  if (((sendbuf != MPI_IN_PLACE) && (sendtype == MPI_DATATYPE_NULL)) ||
      ((comm->rank() == root) && (recvtype == MPI_DATATYPE_NULL)))
    return MPI_ERR_TYPE;
  if ((sendbuf != MPI_IN_PLACE) && (sendcount < 0))
    return MPI_ERR_COUNT;
  if ((comm->rank() == root) && (recvcounts == nullptr || displs == nullptr))
    return MPI_ERR_ARG;
  if (root < 0 || root >= comm->size())
    return MPI_ERR_ROOT;
  if (request == nullptr)
    return MPI_ERR_ARG;

  for (int i = 0; i < comm->size(); i++) {
    if ((comm->rank() == root) && (recvcounts[i] < 0))
      return MPI_ERR_COUNT;
  }

  smpi_bench_end();
  const void* real_sendbuf   = sendbuf;
  int real_sendcount         = sendcount;
  MPI_Datatype real_sendtype = sendtype;
  if ((comm->rank() == root) && (sendbuf == MPI_IN_PLACE)) {
    real_sendcount = 0;
    real_sendtype  = recvtype;
  }

  int rank         = simgrid::s4u::this_actor::get_pid();
  int dt_size_recv = recvtype->is_replayable() ? 1 : recvtype->size();

  std::vector<int>* trace_recvcounts = new std::vector<int>;
  if (comm->rank() == root) {
    for (int i = 0; i < comm->size(); i++) // copy data to avoid bad free
      trace_recvcounts->push_back(recvcounts[i] * dt_size_recv);
  }

  TRACE_smpi_comm_in(rank, request == MPI_REQUEST_IGNORED ? "PMPI_Gatherv" : "PMPI_Igatherv",
                     new simgrid::instr::VarCollTIData(
                         request == MPI_REQUEST_IGNORED ? "gatherv" : "igatherv", root,
                         real_sendtype->is_replayable() ? real_sendcount : real_sendcount * real_sendtype->size(),
                         nullptr, dt_size_recv, trace_recvcounts, simgrid::smpi::Datatype::encode(real_sendtype),
                         simgrid::smpi::Datatype::encode(recvtype)));
  if (request == MPI_REQUEST_IGNORED)
    simgrid::smpi::Colls::gatherv(real_sendbuf, real_sendcount, real_sendtype, recvbuf, recvcounts, displs, recvtype,
                                  root, comm);
  else
    simgrid::smpi::Colls::igatherv(real_sendbuf, real_sendcount, real_sendtype, recvbuf, recvcounts, displs, recvtype,
                                   root, comm, request);

  TRACE_smpi_comm_out(rank);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm){
  return PMPI_Iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Iallgather(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                    MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* request)
{
  CHECK_ARGS(comm == MPI_COMM_NULL, MPI_ERR_COMM, "Iallgather: the communicator cannot be MPI_COMM_NULL");
  CHECK_ARGS(recvbuf == nullptr && recvcount > 0, MPI_ERR_BUFFER, "Iallgather: param 4 recvbuf cannot be NULL");
  CHECK_ARGS(sendbuf == nullptr && sendcount > 0, MPI_ERR_BUFFER,
             "Iallgather: param 1 sendbuf cannot be NULL when sendcound > 0");
  CHECK_ARGS((sendbuf != MPI_IN_PLACE) && (sendtype == MPI_DATATYPE_NULL), MPI_ERR_TYPE,
             "Iallgather: param 3 sendtype cannot be MPI_DATATYPE_NULL when sendbuff is not MPI_IN_PLACE");
  CHECK_ARGS(recvtype == MPI_DATATYPE_NULL, MPI_ERR_TYPE, "Iallgather: param 6 recvtype cannot be MPI_DATATYPE_NULL");
  CHECK_ARGS(recvcount < 0, MPI_ERR_COUNT, "Iallgather: param 5 recvcount cannot be negative");
  CHECK_ARGS((sendbuf != MPI_IN_PLACE) && (sendcount < 0), MPI_ERR_COUNT,
             "Iallgather: param 2 sendcount cannot be negative when sendbuf is not MPI_IN_PLACE");
  CHECK_ARGS(request == nullptr, MPI_ERR_ARG, "Iallgather: param 8 request cannot be NULL");

  smpi_bench_end();
  if (sendbuf == MPI_IN_PLACE) {
    sendbuf   = static_cast<char*>(recvbuf) + recvtype->get_extent() * recvcount * comm->rank();
    sendcount = recvcount;
    sendtype  = recvtype;
  }
  int rank = simgrid::s4u::this_actor::get_pid();

  TRACE_smpi_comm_in(rank, request == MPI_REQUEST_IGNORED ? "PMPI_Allgather" : "PMPI_Iallggather",
                     new simgrid::instr::CollTIData(
                         request == MPI_REQUEST_IGNORED ? "allgather" : "iallgather", -1, -1.0,
                         sendtype->is_replayable() ? sendcount : sendcount * sendtype->size(),
                         recvtype->is_replayable() ? recvcount : recvcount * recvtype->size(),
                         simgrid::smpi::Datatype::encode(sendtype), simgrid::smpi::Datatype::encode(recvtype)));
  if (request == MPI_REQUEST_IGNORED)
    simgrid::smpi::Colls::allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
  else
    simgrid::smpi::Colls::iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);

  TRACE_smpi_comm_out(rank);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm){
  return PMPI_Iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Iallgatherv(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, const int* recvcounts, const int* displs,
                     MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* request)
{
  if (comm == MPI_COMM_NULL)
    return MPI_ERR_COMM;
  if (sendbuf == nullptr && sendcount > 0)
    return MPI_ERR_BUFFER;
  if (((sendbuf != MPI_IN_PLACE) && (sendtype == MPI_DATATYPE_NULL)) || (recvtype == MPI_DATATYPE_NULL))
    return MPI_ERR_TYPE;
  if ((sendbuf != MPI_IN_PLACE) && (sendcount < 0))
    return MPI_ERR_COUNT;
  if (recvcounts == nullptr || displs == nullptr)
    return MPI_ERR_ARG;
  if (request == nullptr)
    return MPI_ERR_ARG;

  for (int i = 0; i < comm->size(); i++) {
    if (recvcounts[i] < 0)
      return MPI_ERR_COUNT;
    else if (recvcounts[i] > 0 && recvbuf == nullptr)
      return MPI_ERR_BUFFER;
  }

  smpi_bench_end();
  if (sendbuf == MPI_IN_PLACE) {
    sendbuf   = static_cast<char*>(recvbuf) + recvtype->get_extent() * displs[comm->rank()];
    sendcount = recvcounts[comm->rank()];
    sendtype  = recvtype;
  }
  int rank         = simgrid::s4u::this_actor::get_pid();
  int dt_size_recv = recvtype->is_replayable() ? 1 : recvtype->size();

  std::vector<int>* trace_recvcounts = new std::vector<int>;
  for (int i = 0; i < comm->size(); i++) { // copy data to avoid bad free
    trace_recvcounts->push_back(recvcounts[i] * dt_size_recv);
  }

  TRACE_smpi_comm_in(
      rank, request == MPI_REQUEST_IGNORED ? "PMPI_Allgatherv" : "PMPI_Iallgatherv",
      new simgrid::instr::VarCollTIData(request == MPI_REQUEST_IGNORED ? "allgatherv" : "iallgatherv", -1,
                                        sendtype->is_replayable() ? sendcount : sendcount * sendtype->size(), nullptr,
                                        dt_size_recv, trace_recvcounts, simgrid::smpi::Datatype::encode(sendtype),
                                        simgrid::smpi::Datatype::encode(recvtype)));
  if (request == MPI_REQUEST_IGNORED)
    simgrid::smpi::Colls::allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
  else
    simgrid::smpi::Colls::iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm,
                                      request);

  TRACE_smpi_comm_out(rank);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm){
  return PMPI_Iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Iscatter(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                  MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request* request)
{
  if (comm == MPI_COMM_NULL)
    return MPI_ERR_COMM;
  if (((comm->rank() == root) && (sendtype == MPI_DATATYPE_NULL || not sendtype->is_valid())) ||
      ((recvbuf != MPI_IN_PLACE) && (recvtype == MPI_DATATYPE_NULL || not recvtype->is_valid())))
    return MPI_ERR_TYPE;
  if (((comm->rank() == root) && (sendcount < 0)) || ((recvbuf != MPI_IN_PLACE) && (recvcount < 0)))
    return MPI_ERR_COUNT;
  if ((sendbuf == recvbuf) || ((comm->rank() == root) && sendcount > 0 && (sendbuf == nullptr)) ||
      (recvcount > 0 && recvbuf == nullptr))
    return MPI_ERR_BUFFER;
  if (root < 0 || root >= comm->size())
    return MPI_ERR_ROOT;
  if (request == nullptr)
    return MPI_ERR_ARG;

  smpi_bench_end();
  if (recvbuf == MPI_IN_PLACE) {
    recvtype  = sendtype;
    recvcount = sendcount;
  }
  int rank = simgrid::s4u::this_actor::get_pid();

  TRACE_smpi_comm_in(rank, request == MPI_REQUEST_IGNORED ? "PMPI_Scatter" : "PMPI_Iscatter",
                     new simgrid::instr::CollTIData(
                         request == MPI_REQUEST_IGNORED ? "scatter" : "iscatter", root, -1.0,
                         (comm->rank() != root || sendtype->is_replayable()) ? sendcount : sendcount * sendtype->size(),
                         recvtype->is_replayable() ? recvcount : recvcount * recvtype->size(),
                         simgrid::smpi::Datatype::encode(sendtype), simgrid::smpi::Datatype::encode(recvtype)));
  if (request == MPI_REQUEST_IGNORED)
    simgrid::smpi::Colls::scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
  else
    simgrid::smpi::Colls::iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request);

  TRACE_smpi_comm_out(rank);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Scatterv(const void *sendbuf, const int *sendcounts, const int *displs,
                 MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm){
  return PMPI_Iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Iscatterv(const void* sendbuf, const int* sendcounts, const int* displs, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                   MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request* request)
{
  CHECK_ARGS(comm == MPI_COMM_NULL, MPI_ERR_COMM, "Iscatterv: the communicator cannot be MPI_COMM_NULL");
  CHECK_ARGS((comm->rank() == root) && (sendcounts == nullptr), MPI_ERR_ARG,
             "Iscatterv: param 2 sendcounts cannot be NULL on the root rank");
  CHECK_ARGS((comm->rank() == root) && (displs == nullptr), MPI_ERR_ARG,
             "Iscatterv: param 3 displs cannot be NULL on the root rank");
  CHECK_ARGS((comm->rank() == root) && (sendtype == MPI_DATATYPE_NULL), MPI_ERR_TYPE,
             "Iscatterv: The sendtype cannot be NULL on the root rank");
  CHECK_ARGS((recvbuf != MPI_IN_PLACE) && (recvtype == MPI_DATATYPE_NULL), MPI_ERR_TYPE,
             "Iscatterv: the recvtype cannot be NULL when not receiving in place");
  CHECK_ARGS(request == nullptr, MPI_ERR_ARG, "Iscatterv: param 10 request cannot be NULL");
  CHECK_ARGS(recvbuf != MPI_IN_PLACE && recvcount < 0, MPI_ERR_COUNT,
             "Iscatterv: When not receiving in place, the recvcound cannot be negative");
  CHECK_ARGS(root < 0, MPI_ERR_ROOT, "Iscatterv: root cannot be negative");
  CHECK_ARGS(root >= comm->size(), MPI_ERR_ROOT, "Iscatterv: root (=%d) is larger than communicator size (=%d)", root,
             comm->size());

  if (comm->rank() == root) {
    if (recvbuf == MPI_IN_PLACE) {
      recvtype  = sendtype;
      recvcount = sendcounts[comm->rank()];
    }
    for (int i = 0; i < comm->size(); i++)
      CHECK_ARGS(sendcounts[i] < 0, MPI_ERR_COUNT, "Iscatterv: sendcounts[%d]=%d but this cannot be negative", i,
                 sendcounts[i]);
  }

  smpi_bench_end();

  int rank         = simgrid::s4u::this_actor::get_pid();
  int dt_size_send = sendtype->is_replayable() ? 1 : sendtype->size();

  std::vector<int>* trace_sendcounts = new std::vector<int>;
  if (comm->rank() == root) {
    for (int i = 0; i < comm->size(); i++) { // copy data to avoid bad free
      trace_sendcounts->push_back(sendcounts[i] * dt_size_send);
    }
  }

  TRACE_smpi_comm_in(rank, request == MPI_REQUEST_IGNORED ? "PMPI_Scatterv" : "PMPI_Iscatterv",
                     new simgrid::instr::VarCollTIData(
                         request == MPI_REQUEST_IGNORED ? "scatterv" : "iscatterv", root, dt_size_send,
                         trace_sendcounts, recvtype->is_replayable() ? recvcount : recvcount * recvtype->size(),
                         nullptr, simgrid::smpi::Datatype::encode(sendtype),
                         simgrid::smpi::Datatype::encode(recvtype)));
  if (request == MPI_REQUEST_IGNORED)
    simgrid::smpi::Colls::scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);
  else
    simgrid::smpi::Colls::iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm,
                                    request);

  TRACE_smpi_comm_out(rank);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
  return PMPI_Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Ireduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request* request)
{
  if (comm == MPI_COMM_NULL)
    return MPI_ERR_COMM;
  if ((sendbuf == nullptr && count > 0) || ((comm->rank() == root) && recvbuf == nullptr))
    return MPI_ERR_BUFFER;
  if (datatype == MPI_DATATYPE_NULL || not datatype->is_valid())
    return MPI_ERR_TYPE;
  if (op == MPI_OP_NULL)
    return MPI_ERR_OP;
  if (request == nullptr)
    return MPI_ERR_ARG;
  if (root < 0 || root >= comm->size())
    return MPI_ERR_ROOT;
  if (count < 0)
    return MPI_ERR_COUNT;

  smpi_bench_end();
  int rank = simgrid::s4u::this_actor::get_pid();

  TRACE_smpi_comm_in(rank, request == MPI_REQUEST_IGNORED ? "PMPI_Reduce" : "PMPI_Ireduce",
                     new simgrid::instr::CollTIData(request == MPI_REQUEST_IGNORED ? "reduce" : "ireduce", root, 0,
                                                    datatype->is_replayable() ? count : count * datatype->size(), -1,
                                                    simgrid::smpi::Datatype::encode(datatype), ""));
  if (request == MPI_REQUEST_IGNORED)
    simgrid::smpi::Colls::reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
  else
    simgrid::smpi::Colls::ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, request);

  TRACE_smpi_comm_out(rank);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Reduce_local(const void* inbuf, void* inoutbuf, int count, MPI_Datatype datatype, MPI_Op op)
{
  if (datatype == MPI_DATATYPE_NULL || not datatype->is_valid())
    return MPI_ERR_TYPE;
  if (op == MPI_OP_NULL)
    return MPI_ERR_OP;
  if (count < 0)
    return MPI_ERR_COUNT;

  smpi_bench_end();
  op->apply(inbuf, inoutbuf, &count, datatype);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return PMPI_Iallreduce(sendbuf, recvbuf, count, datatype, op, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Iallreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
  if (comm == MPI_COMM_NULL)
    return MPI_ERR_COMM;
  if ((sendbuf == nullptr && count > 0) || (recvbuf == nullptr))
    return MPI_ERR_BUFFER;
  if (datatype == MPI_DATATYPE_NULL || not datatype->is_valid())
    return MPI_ERR_TYPE;
  if (count < 0)
    return MPI_ERR_COUNT;
  if (op == MPI_OP_NULL)
    return MPI_ERR_OP;
  if (request == nullptr)
    return MPI_ERR_ARG;

  smpi_bench_end();
  const void* real_sendbuf = sendbuf;
  std::unique_ptr<unsigned char[]> tmp_sendbuf;
  if (sendbuf == MPI_IN_PLACE) {
    tmp_sendbuf.reset(new unsigned char[count * datatype->get_extent()]);
    simgrid::smpi::Datatype::copy(recvbuf, count, datatype, tmp_sendbuf.get(), count, datatype);
    real_sendbuf = tmp_sendbuf.get();
  }
  int rank = simgrid::s4u::this_actor::get_pid();

  TRACE_smpi_comm_in(rank, request == MPI_REQUEST_IGNORED ? "PMPI_Allreduce" : "PMPI_Iallreduce",
                     new simgrid::instr::CollTIData(request == MPI_REQUEST_IGNORED ? "allreduce" : "iallreduce", -1, 0,
                                                    datatype->is_replayable() ? count : count * datatype->size(), -1,
                                                    simgrid::smpi::Datatype::encode(datatype), ""));

  if (request == MPI_REQUEST_IGNORED)
    simgrid::smpi::Colls::allreduce(real_sendbuf, recvbuf, count, datatype, op, comm);
  else
    simgrid::smpi::Colls::iallreduce(real_sendbuf, recvbuf, count, datatype, op, comm, request);

  TRACE_smpi_comm_out(rank);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return PMPI_Iscan(sendbuf, recvbuf, count, datatype, op, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Iscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request* request)
{
  if (comm == MPI_COMM_NULL)
    return MPI_ERR_COMM;
  if (datatype == MPI_DATATYPE_NULL || not datatype->is_valid())
    return MPI_ERR_TYPE;
  if (op == MPI_OP_NULL)
    return MPI_ERR_OP;
  if (request == nullptr)
    return MPI_ERR_ARG;
  if (count < 0)
    return MPI_ERR_COUNT;
  if (sendbuf == nullptr || recvbuf == nullptr)
    return MPI_ERR_BUFFER;

  smpi_bench_end();
  int rank         = simgrid::s4u::this_actor::get_pid();
  const void* real_sendbuf = sendbuf;
  std::unique_ptr<unsigned char[]> tmp_sendbuf;
  if (sendbuf == MPI_IN_PLACE) {
    tmp_sendbuf.reset(new unsigned char[count * datatype->size()]);
    real_sendbuf = memcpy(tmp_sendbuf.get(), recvbuf, count * datatype->size());
  }
  TRACE_smpi_comm_in(rank, request == MPI_REQUEST_IGNORED ? "PMPI_Scan" : "PMPI_Iscan",
                     new simgrid::instr::Pt2PtTIData(request == MPI_REQUEST_IGNORED ? "scan" : "iscan", -1,
                                                     datatype->is_replayable() ? count : count * datatype->size(),
                                                     simgrid::smpi::Datatype::encode(datatype)));

  int retval;
  if (request == MPI_REQUEST_IGNORED)
    retval = simgrid::smpi::Colls::scan(real_sendbuf, recvbuf, count, datatype, op, comm);
  else
    retval = simgrid::smpi::Colls::iscan(real_sendbuf, recvbuf, count, datatype, op, comm, request);

  TRACE_smpi_comm_out(rank);
  smpi_bench_begin();
  return retval;
}

int PMPI_Exscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return PMPI_Iexscan(sendbuf, recvbuf, count, datatype, op, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Iexscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request* request){
  if (comm == MPI_COMM_NULL)
    return MPI_ERR_COMM;
  if (not datatype->is_valid())
    return MPI_ERR_TYPE;
  if (op == MPI_OP_NULL)
    return MPI_ERR_OP;
  if (request == nullptr)
    return MPI_ERR_ARG;
  if (count < 0)
    return MPI_ERR_COUNT;
  if (sendbuf == nullptr || recvbuf == nullptr)
    return MPI_ERR_BUFFER;

  smpi_bench_end();
  int rank         = simgrid::s4u::this_actor::get_pid();
  const void* real_sendbuf = sendbuf;
  std::unique_ptr<unsigned char[]> tmp_sendbuf;
  if (sendbuf == MPI_IN_PLACE) {
    tmp_sendbuf.reset(new unsigned char[count * datatype->size()]);
    real_sendbuf = memcpy(tmp_sendbuf.get(), recvbuf, count * datatype->size());
  }

  TRACE_smpi_comm_in(rank, request == MPI_REQUEST_IGNORED ? "PMPI_Exscan" : "PMPI_Iexscan",
                     new simgrid::instr::Pt2PtTIData(request == MPI_REQUEST_IGNORED ? "exscan" : "iexscan", -1,
                                                     datatype->is_replayable() ? count : count * datatype->size(),
                                                     simgrid::smpi::Datatype::encode(datatype)));

  int retval;
  if (request == MPI_REQUEST_IGNORED)
    retval = simgrid::smpi::Colls::exscan(real_sendbuf, recvbuf, count, datatype, op, comm);
  else
    retval = simgrid::smpi::Colls::iexscan(real_sendbuf, recvbuf, count, datatype, op, comm, request);

  TRACE_smpi_comm_out(rank);
  smpi_bench_begin();
  return retval;
}

int PMPI_Reduce_scatter(const void *sendbuf, void *recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return PMPI_Ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Ireduce_scatter(const void *sendbuf, void *recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
  if (comm == MPI_COMM_NULL)
    return MPI_ERR_COMM;
  if ((sendbuf == nullptr) || (recvbuf == nullptr))
    return MPI_ERR_BUFFER;
  if (datatype == MPI_DATATYPE_NULL || not datatype->is_valid())
    return MPI_ERR_TYPE;
  if (op == MPI_OP_NULL)
    return MPI_ERR_OP;
  if (recvcounts == nullptr)
    return MPI_ERR_ARG;
  if (request == nullptr)
    return MPI_ERR_ARG;

  for (int i = 0; i < comm->size(); i++) { // copy data to avoid bad free
    if (recvcounts[i] < 0)
      return MPI_ERR_COUNT;
  }

  smpi_bench_end();
  int rank                           = simgrid::s4u::this_actor::get_pid();
  std::vector<int>* trace_recvcounts = new std::vector<int>;
  int dt_send_size                   = datatype->is_replayable() ? 1 : datatype->size();
  int totalcount                     = 0;

  for (int i = 0; i < comm->size(); i++) { // copy data to avoid bad free
    trace_recvcounts->push_back(recvcounts[i] * dt_send_size);
    totalcount += recvcounts[i];
  }

  const void* real_sendbuf = sendbuf;
  std::unique_ptr<unsigned char[]> tmp_sendbuf;
  if (sendbuf == MPI_IN_PLACE) {
    tmp_sendbuf.reset(new unsigned char[totalcount * datatype->size()]);
    real_sendbuf = memcpy(tmp_sendbuf.get(), recvbuf, totalcount * datatype->size());
  }

  TRACE_smpi_comm_in(rank, request == MPI_REQUEST_IGNORED ? "PMPI_Reduce_scatter" : "PMPI_Ireduce_scatter",
                     new simgrid::instr::VarCollTIData(
                         request == MPI_REQUEST_IGNORED ? "reducescatter" : "ireducescatter", -1, dt_send_size, nullptr,
                         -1, trace_recvcounts, simgrid::smpi::Datatype::encode(datatype), ""));

  if (request == MPI_REQUEST_IGNORED)
    simgrid::smpi::Colls::reduce_scatter(real_sendbuf, recvbuf, recvcounts, datatype, op, comm);
  else
    simgrid::smpi::Colls::ireduce_scatter(real_sendbuf, recvbuf, recvcounts, datatype, op, comm, request);

  TRACE_smpi_comm_out(rank);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount,
                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return PMPI_Ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Ireduce_scatter_block(const void* sendbuf, void* recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op,
                               MPI_Comm comm, MPI_Request* request)
{
  if (comm == MPI_COMM_NULL)
    return MPI_ERR_COMM;
  if (not datatype->is_valid())
    return MPI_ERR_TYPE;
  if (op == MPI_OP_NULL)
    return MPI_ERR_OP;
  if (recvcount < 0)
    return MPI_ERR_ARG;
  if (request == nullptr)
    return MPI_ERR_ARG;

  smpi_bench_end();
  int count = comm->size();

  int rank                           = simgrid::s4u::this_actor::get_pid();
  int dt_send_size                   = datatype->is_replayable() ? 1 : datatype->size();
  std::vector<int>* trace_recvcounts = new std::vector<int>(recvcount * dt_send_size); // copy data to avoid bad free

  const void* real_sendbuf = sendbuf;
  std::unique_ptr<unsigned char[]> tmp_sendbuf;
  if (sendbuf == MPI_IN_PLACE) {
    tmp_sendbuf.reset(new unsigned char[recvcount * count * datatype->size()]);
    real_sendbuf = memcpy(tmp_sendbuf.get(), recvbuf, recvcount * count * datatype->size());
  }

  TRACE_smpi_comm_in(
      rank, request == MPI_REQUEST_IGNORED ? "PMPI_Reduce_scatter_block" : "PMPI_Ireduce_scatter_block",
      new simgrid::instr::VarCollTIData(request == MPI_REQUEST_IGNORED ? "reducescatter" : "ireducescatter", -1, 0,
                                        nullptr, -1, trace_recvcounts, simgrid::smpi::Datatype::encode(datatype), ""));

  int* recvcounts = new int[count];
  for (int i      = 0; i < count; i++)
    recvcounts[i] = recvcount;
  if (request == MPI_REQUEST_IGNORED)
    simgrid::smpi::Colls::reduce_scatter(real_sendbuf, recvbuf, recvcounts, datatype, op, comm);
  else
    simgrid::smpi::Colls::ireduce_scatter(real_sendbuf, recvbuf, recvcounts, datatype, op, comm, request);
  delete[] recvcounts;

  TRACE_smpi_comm_out(rank);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Alltoall(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                  MPI_Datatype recvtype, MPI_Comm comm){
  return PMPI_Ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Ialltoall(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                   MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* request)
{
  if (comm == MPI_COMM_NULL)
    return MPI_ERR_COMM;
  if ((sendbuf == nullptr && sendcount > 0) || (recvbuf == nullptr && recvcount > 0))
    return MPI_ERR_BUFFER;
  if ((sendbuf != MPI_IN_PLACE && sendtype == MPI_DATATYPE_NULL) || recvtype == MPI_DATATYPE_NULL)
    return MPI_ERR_TYPE;
  if ((sendbuf != MPI_IN_PLACE && sendcount < 0) || recvcount < 0)
    return MPI_ERR_COUNT;
  if (request == nullptr)
    return MPI_ERR_ARG;

  smpi_bench_end();
  int rank                 = simgrid::s4u::this_actor::get_pid();
  const void* real_sendbuf = sendbuf;
  int real_sendcount         = sendcount;
  MPI_Datatype real_sendtype = sendtype;
  std::unique_ptr<unsigned char[]> tmp_sendbuf;
  if (sendbuf == MPI_IN_PLACE) {
    tmp_sendbuf.reset(new unsigned char[recvcount * comm->size() * recvtype->size()]);
    // memcpy(??,nullptr,0) is actually undefined behavior, even if harmless.
    if (recvbuf != nullptr)
      memcpy(tmp_sendbuf.get(), recvbuf, recvcount * comm->size() * recvtype->size());
    real_sendbuf = tmp_sendbuf.get();
    real_sendcount = recvcount;
    real_sendtype  = recvtype;
  }

  TRACE_smpi_comm_in(rank, request == MPI_REQUEST_IGNORED ? "PMPI_Alltoall" : "PMPI_Ialltoall",
                     new simgrid::instr::CollTIData(
                         request == MPI_REQUEST_IGNORED ? "alltoall" : "ialltoall", -1, -1.0,
                         real_sendtype->is_replayable() ? real_sendcount : real_sendcount * real_sendtype->size(),
                         recvtype->is_replayable() ? recvcount : recvcount * recvtype->size(),
                         simgrid::smpi::Datatype::encode(real_sendtype), simgrid::smpi::Datatype::encode(recvtype)));
  int retval;
  if (request == MPI_REQUEST_IGNORED)
    retval =
        simgrid::smpi::Colls::alltoall(real_sendbuf, real_sendcount, real_sendtype, recvbuf, recvcount, recvtype, comm);
  else
    retval = simgrid::smpi::Colls::ialltoall(real_sendbuf, real_sendcount, real_sendtype, recvbuf, recvcount, recvtype,
                                             comm, request);

  TRACE_smpi_comm_out(rank);
  smpi_bench_begin();
  return retval;
}

int PMPI_Alltoallv(const void* sendbuf, const int* sendcounts, const int* senddisps, MPI_Datatype sendtype, void* recvbuf,
                   const int* recvcounts, const int* recvdisps, MPI_Datatype recvtype, MPI_Comm comm)
{
  return PMPI_Ialltoallv(sendbuf, sendcounts, senddisps, sendtype, recvbuf, recvcounts, recvdisps, recvtype, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Ialltoallv(const void* sendbuf, const int* sendcounts, const int* senddisps, MPI_Datatype sendtype, void* recvbuf,
                    const int* recvcounts, const int* recvdisps, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* request)
{
  if (comm == MPI_COMM_NULL)
    return MPI_ERR_COMM;
  if (sendbuf == nullptr || recvbuf == nullptr)
    return MPI_ERR_BUFFER;
  if ((sendbuf != MPI_IN_PLACE && sendtype == MPI_DATATYPE_NULL) || recvtype == MPI_DATATYPE_NULL)
    return MPI_ERR_TYPE;
  if ((sendbuf != MPI_IN_PLACE && (sendcounts == nullptr || senddisps == nullptr)) || recvcounts == nullptr ||
      recvdisps == nullptr)
    return MPI_ERR_ARG;
  if (request == nullptr)
    return MPI_ERR_ARG;

  int rank = simgrid::s4u::this_actor::get_pid();
  int size = comm->size();
  for (int i = 0; i < size; i++) {
    if (recvcounts[i] < 0 || (sendbuf != MPI_IN_PLACE && sendcounts[i] < 0))
      return MPI_ERR_COUNT;
  }

  smpi_bench_end();
  int send_size                      = 0;
  int recv_size                      = 0;
  std::vector<int>* trace_sendcounts = new std::vector<int>;
  std::vector<int>* trace_recvcounts = new std::vector<int>;
  int dt_size_recv                   = recvtype->size();

  const void* real_sendbuf   = sendbuf;
  const int* real_sendcounts = sendcounts;
  const int* real_senddisps  = senddisps;
  MPI_Datatype real_sendtype = sendtype;
  int maxsize              = 0;
  for (int i = 0; i < size; i++) { // copy data to avoid bad free
    recv_size += recvcounts[i] * dt_size_recv;
    trace_recvcounts->push_back(recvcounts[i] * dt_size_recv);
    if (((recvdisps[i] + recvcounts[i]) * dt_size_recv) > maxsize)
      maxsize = (recvdisps[i] + recvcounts[i]) * dt_size_recv;
  }

  std::unique_ptr<unsigned char[]> tmp_sendbuf;
  std::unique_ptr<int[]> tmp_sendcounts;
  std::unique_ptr<int[]> tmp_senddisps;
  if (sendbuf == MPI_IN_PLACE) {
    tmp_sendbuf.reset(new unsigned char[maxsize]);
    real_sendbuf = memcpy(tmp_sendbuf.get(), recvbuf, maxsize);
    tmp_sendcounts.reset(new int[size]);
    std::copy(recvcounts, recvcounts + size, tmp_sendcounts.get());
    real_sendcounts = tmp_sendcounts.get();
    tmp_senddisps.reset(new int[size]);
    std::copy(recvdisps, recvdisps + size, tmp_senddisps.get());
    real_senddisps = tmp_senddisps.get();
    real_sendtype  = recvtype;
  }

  int dt_size_send = real_sendtype->size();

  for (int i = 0; i < size; i++) { // copy data to avoid bad free
    send_size += real_sendcounts[i] * dt_size_send;
    trace_sendcounts->push_back(real_sendcounts[i] * dt_size_send);
  }

  TRACE_smpi_comm_in(rank, request == MPI_REQUEST_IGNORED ? "PMPI_Alltoallv" : "PMPI_Ialltoallv",
                     new simgrid::instr::VarCollTIData(request == MPI_REQUEST_IGNORED ? "alltoallv" : "ialltoallv", -1,
                                                       send_size, trace_sendcounts, recv_size, trace_recvcounts,
                                                       simgrid::smpi::Datatype::encode(real_sendtype),
                                                       simgrid::smpi::Datatype::encode(recvtype)));

  int retval;
  if (request == MPI_REQUEST_IGNORED)
    retval = simgrid::smpi::Colls::alltoallv(real_sendbuf, real_sendcounts, real_senddisps, real_sendtype, recvbuf,
                                             recvcounts, recvdisps, recvtype, comm);
  else
    retval = simgrid::smpi::Colls::ialltoallv(real_sendbuf, real_sendcounts, real_senddisps, real_sendtype, recvbuf,
                                              recvcounts, recvdisps, recvtype, comm, request);

  TRACE_smpi_comm_out(rank);
  smpi_bench_begin();
  return retval;
}

int PMPI_Alltoallw(const void* sendbuf, const int* sendcounts, const int* senddisps, const MPI_Datatype* sendtypes, void* recvbuf,
                   const int* recvcounts, const int* recvdisps, const MPI_Datatype* recvtypes, MPI_Comm comm)
{
  return PMPI_Ialltoallw(sendbuf, sendcounts, senddisps, sendtypes, recvbuf, recvcounts, recvdisps, recvtypes, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Ialltoallw(const void* sendbuf, const int* sendcounts, const int* senddisps, const MPI_Datatype* sendtypes, void* recvbuf,
                    const int* recvcounts, const int* recvdisps, const MPI_Datatype* recvtypes, MPI_Comm comm, MPI_Request* request)
{
  if (comm == MPI_COMM_NULL)
    return MPI_ERR_COMM;
  if (sendbuf == nullptr || recvbuf == nullptr)
    return MPI_ERR_BUFFER;
  if ((sendbuf != MPI_IN_PLACE && sendtypes == nullptr) || recvtypes == nullptr)
    return MPI_ERR_TYPE;
  if ((sendbuf != MPI_IN_PLACE && (sendcounts == nullptr || senddisps == nullptr)) || recvcounts == nullptr ||
      recvdisps == nullptr)
    return MPI_ERR_ARG;
  if (request == nullptr)
    return MPI_ERR_ARG;

  smpi_bench_end();
  int rank = simgrid::s4u::this_actor::get_pid();
  int size = comm->size();
  for (int i = 0; i < size; i++) {
    if (recvcounts[i] < 0 || (sendbuf != MPI_IN_PLACE && sendcounts[i] < 0))
      return MPI_ERR_COUNT;
  }
  int send_size                      = 0;
  int recv_size                      = 0;
  std::vector<int>* trace_sendcounts = new std::vector<int>;
  std::vector<int>* trace_recvcounts = new std::vector<int>;

  const void* real_sendbuf           = sendbuf;
  const int* real_sendcounts         = sendcounts;
  const int* real_senddisps          = senddisps;
  const MPI_Datatype* real_sendtypes = sendtypes;
  unsigned long maxsize      = 0;
  for (int i = 0; i < size; i++) { // copy data to avoid bad free
    if (recvtypes[i] == MPI_DATATYPE_NULL) {
      delete trace_recvcounts;
      delete trace_sendcounts;
      return MPI_ERR_TYPE;
    }
    recv_size += recvcounts[i] * recvtypes[i]->size();
    trace_recvcounts->push_back(recvcounts[i] * recvtypes[i]->size());
    if ((recvdisps[i] + (recvcounts[i] * recvtypes[i]->size())) > maxsize)
      maxsize = recvdisps[i] + (recvcounts[i] * recvtypes[i]->size());
  }

  std::unique_ptr<unsigned char[]> tmp_sendbuf;
  std::unique_ptr<int[]> tmp_sendcounts;
  std::unique_ptr<int[]> tmp_senddisps;
  std::unique_ptr<MPI_Datatype[]> tmp_sendtypes;
  if (sendbuf == MPI_IN_PLACE) {
    tmp_sendbuf.reset(new unsigned char[maxsize]);
    real_sendbuf = memcpy(tmp_sendbuf.get(), recvbuf, maxsize);
    tmp_sendcounts.reset(new int[size]);
    std::copy(recvcounts, recvcounts + size, tmp_sendcounts.get());
    real_sendcounts = tmp_sendcounts.get();
    tmp_senddisps.reset(new int[size]);
    std::copy(recvdisps, recvdisps + size, tmp_senddisps.get());
    real_senddisps = tmp_senddisps.get();
    tmp_sendtypes.reset(new MPI_Datatype[size]);
    std::copy(recvtypes, recvtypes + size, tmp_sendtypes.get());
    real_sendtypes = tmp_sendtypes.get();
  }

  for (int i = 0; i < size; i++) { // copy data to avoid bad free
    send_size += real_sendcounts[i] * real_sendtypes[i]->size();
    trace_sendcounts->push_back(real_sendcounts[i] * real_sendtypes[i]->size());
  }

  TRACE_smpi_comm_in(rank, request == MPI_REQUEST_IGNORED ? "PMPI_Alltoallw" : "PMPI_Ialltoallw",
                     new simgrid::instr::VarCollTIData(request == MPI_REQUEST_IGNORED ? "alltoallv" : "ialltoallv", -1,
                                                       send_size, trace_sendcounts, recv_size, trace_recvcounts,
                                                       simgrid::smpi::Datatype::encode(real_sendtypes[0]),
                                                       simgrid::smpi::Datatype::encode(recvtypes[0])));

  int retval;
  if (request == MPI_REQUEST_IGNORED)
    retval = simgrid::smpi::Colls::alltoallw(real_sendbuf, real_sendcounts, real_senddisps, real_sendtypes, recvbuf,
                                             recvcounts, recvdisps, recvtypes, comm);
  else
    retval = simgrid::smpi::Colls::ialltoallw(real_sendbuf, real_sendcounts, real_senddisps, real_sendtypes, recvbuf,
                                              recvcounts, recvdisps, recvtypes, comm, request);

  TRACE_smpi_comm_out(rank);
  smpi_bench_begin();
  return retval;
}

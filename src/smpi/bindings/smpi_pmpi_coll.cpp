/* Copyright (c) 2007-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "smpi_coll.hpp"
#include "smpi_comm.hpp"
#include "smpi_datatype_derived.hpp"
#include "smpi_op.hpp"
#include "src/smpi/include/smpi_actor.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smpi_pmpi);

/* PMPI User level calls */

int PMPI_Bcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_ARG;
  } else {
    int rank = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(rank, __func__,
                       new simgrid::instr::CollTIData("bcast", root, -1.0,
                                                      datatype->is_replayable() ? count : count * datatype->size(), -1,
                                                      simgrid::smpi::Datatype::encode(datatype), ""));
    if (comm->size() > 1)
      simgrid::smpi::Colls::bcast(buf, count, datatype, root, comm);
    retval = MPI_SUCCESS;

    TRACE_smpi_comm_out(rank);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Barrier(MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    int rank = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(rank, __func__, new simgrid::instr::NoOpTIData("barrier"));

    simgrid::smpi::Colls::barrier(comm);

    //Barrier can be used to synchronize RMA calls. Finish all requests from comm before.
    comm->finish_rma_calls();

    retval = MPI_SUCCESS;

    TRACE_smpi_comm_out(rank);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Gather(void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount, MPI_Datatype recvtype,
                int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendtype == MPI_DATATYPE_NULL)) ||
            ((comm->rank() == root) && (recvtype == MPI_DATATYPE_NULL))){
    retval = MPI_ERR_TYPE;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendcount <0)) || ((comm->rank() == root) && (recvcount <0))){
    retval = MPI_ERR_COUNT;
  } else {

    char* sendtmpbuf = static_cast<char*>(sendbuf);
    int sendtmpcount = sendcount;
    MPI_Datatype sendtmptype = sendtype;
    if( (comm->rank() == root) && (sendbuf == MPI_IN_PLACE )) {
      sendtmpcount=0;
      sendtmptype=recvtype;
    }
    int rank = simgrid::s4u::this_actor::get_pid();

    TRACE_smpi_comm_in(
        rank, __func__,
        new simgrid::instr::CollTIData(
            "gather", root, -1.0, sendtmptype->is_replayable() ? sendtmpcount : sendtmpcount * sendtmptype->size(),
            (comm->rank() != root || recvtype->is_replayable()) ? recvcount : recvcount * recvtype->size(),
            simgrid::smpi::Datatype::encode(sendtmptype), simgrid::smpi::Datatype::encode(recvtype)));

    simgrid::smpi::Colls::gather(sendtmpbuf, sendtmpcount, sendtmptype, recvbuf, recvcount, recvtype, root, comm);

    retval = MPI_SUCCESS;
    TRACE_smpi_comm_out(rank);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Gatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *displs,
                MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendtype == MPI_DATATYPE_NULL)) ||
            ((comm->rank() == root) && (recvtype == MPI_DATATYPE_NULL))){
    retval = MPI_ERR_TYPE;
  } else if (( sendbuf != MPI_IN_PLACE) && (sendcount <0)){
    retval = MPI_ERR_COUNT;
  } else if ((comm->rank() == root) && (recvcounts == nullptr || displs == nullptr)) {
    retval = MPI_ERR_ARG;
  } else {
    char* sendtmpbuf = static_cast<char*>(sendbuf);
    int sendtmpcount = sendcount;
    MPI_Datatype sendtmptype = sendtype;
    if( (comm->rank() == root) && (sendbuf == MPI_IN_PLACE )) {
      sendtmpcount=0;
      sendtmptype=recvtype;
    }

    int rank         = simgrid::s4u::this_actor::get_pid();
    int dt_size_recv = recvtype->is_replayable() ? 1 : recvtype->size();

    std::vector<int>* trace_recvcounts = new std::vector<int>;
    if (comm->rank() == root) {
      for (int i = 0; i < comm->size(); i++) // copy data to avoid bad free
        trace_recvcounts->push_back(recvcounts[i] * dt_size_recv);
    }

    TRACE_smpi_comm_in(rank, __func__,
                       new simgrid::instr::VarCollTIData(
                           "gatherv", root,
                           sendtmptype->is_replayable() ? sendtmpcount : sendtmpcount * sendtmptype->size(), nullptr,
                           dt_size_recv, trace_recvcounts, simgrid::smpi::Datatype::encode(sendtmptype),
                           simgrid::smpi::Datatype::encode(recvtype)));

    retval = simgrid::smpi::Colls::gatherv(sendtmpbuf, sendtmpcount, sendtmptype, recvbuf, recvcounts, displs, recvtype, root, comm);
    TRACE_smpi_comm_out(rank);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
  int retval = MPI_SUCCESS;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendtype == MPI_DATATYPE_NULL)) ||
            (recvtype == MPI_DATATYPE_NULL)){
    retval = MPI_ERR_TYPE;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendcount <0)) ||
            (recvcount <0)){
    retval = MPI_ERR_COUNT;
  } else {
    if(sendbuf == MPI_IN_PLACE) {
      sendbuf=static_cast<char*>(recvbuf)+recvtype->get_extent()*recvcount*comm->rank();
      sendcount=recvcount;
      sendtype=recvtype;
    }
    int rank = simgrid::s4u::this_actor::get_pid();

    TRACE_smpi_comm_in(rank, __func__,
                       new simgrid::instr::CollTIData(
                           "allgather", -1, -1.0, sendtype->is_replayable() ? sendcount : sendcount * sendtype->size(),
                           recvtype->is_replayable() ? recvcount : recvcount * recvtype->size(),
                           simgrid::smpi::Datatype::encode(sendtype), simgrid::smpi::Datatype::encode(recvtype)));

    simgrid::smpi::Colls::allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    TRACE_smpi_comm_out(rank);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int *recvcounts, int *displs, MPI_Datatype recvtype, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (((sendbuf != MPI_IN_PLACE) && (sendtype == MPI_DATATYPE_NULL)) || (recvtype == MPI_DATATYPE_NULL)) {
    retval = MPI_ERR_TYPE;
  } else if (( sendbuf != MPI_IN_PLACE) && (sendcount <0)){
    retval = MPI_ERR_COUNT;
  } else if (recvcounts == nullptr || displs == nullptr) {
    retval = MPI_ERR_ARG;
  } else {

    if(sendbuf == MPI_IN_PLACE) {
      sendbuf=static_cast<char*>(recvbuf)+recvtype->get_extent()*displs[comm->rank()];
      sendcount=recvcounts[comm->rank()];
      sendtype=recvtype;
    }
    int rank               = simgrid::s4u::this_actor::get_pid();
    int dt_size_recv       = recvtype->is_replayable() ? 1 : recvtype->size();

    std::vector<int>* trace_recvcounts = new std::vector<int>;
    for (int i = 0; i < comm->size(); i++) // copy data to avoid bad free
      trace_recvcounts->push_back(recvcounts[i] * dt_size_recv);

    TRACE_smpi_comm_in(rank, __func__,
                       new simgrid::instr::VarCollTIData(
                           "allgatherv", -1, sendtype->is_replayable() ? sendcount : sendcount * sendtype->size(),
                           nullptr, dt_size_recv, trace_recvcounts, simgrid::smpi::Datatype::encode(sendtype),
                           simgrid::smpi::Datatype::encode(recvtype)));

    simgrid::smpi::Colls::allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
    retval = MPI_SUCCESS;
    TRACE_smpi_comm_out(rank);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (((comm->rank() == root) && (not sendtype->is_valid())) ||
             ((recvbuf != MPI_IN_PLACE) && (not recvtype->is_valid()))) {
    retval = MPI_ERR_TYPE;
  } else if ((sendbuf == recvbuf) ||
      ((comm->rank()==root) && sendcount>0 && (sendbuf == nullptr))){
    retval = MPI_ERR_BUFFER;
  }else {

    if (recvbuf == MPI_IN_PLACE) {
      recvtype  = sendtype;
      recvcount = sendcount;
    }
    int rank = simgrid::s4u::this_actor::get_pid();

    TRACE_smpi_comm_in(
        rank, __func__,
        new simgrid::instr::CollTIData(
            "scatter", root, -1.0,
            (comm->rank() != root || sendtype->is_replayable()) ? sendcount : sendcount * sendtype->size(),
            recvtype->is_replayable() ? recvcount : recvcount * recvtype->size(),
            simgrid::smpi::Datatype::encode(sendtype), simgrid::smpi::Datatype::encode(recvtype)));

    simgrid::smpi::Colls::scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    retval = MPI_SUCCESS;
    TRACE_smpi_comm_out(rank);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Scatterv(void *sendbuf, int *sendcounts, int *displs,
                 MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (sendcounts == nullptr || displs == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (((comm->rank() == root) && (sendtype == MPI_DATATYPE_NULL)) ||
             ((recvbuf != MPI_IN_PLACE) && (recvtype == MPI_DATATYPE_NULL))) {
    retval = MPI_ERR_TYPE;
  } else {
    if (recvbuf == MPI_IN_PLACE) {
      recvtype  = sendtype;
      recvcount = sendcounts[comm->rank()];
    }
    int rank               = simgrid::s4u::this_actor::get_pid();
    int dt_size_send       = sendtype->is_replayable() ? 1 : sendtype->size();

    std::vector<int>* trace_sendcounts = new std::vector<int>;
    if (comm->rank() == root) {
      for (int i = 0; i < comm->size(); i++) // copy data to avoid bad free
        trace_sendcounts->push_back(sendcounts[i] * dt_size_send);
    }

    TRACE_smpi_comm_in(rank, __func__,
                       new simgrid::instr::VarCollTIData(
                           "scatterv", root, dt_size_send, trace_sendcounts,
                           recvtype->is_replayable() ? recvcount : recvcount * recvtype->size(), nullptr,
                           simgrid::smpi::Datatype::encode(sendtype), simgrid::smpi::Datatype::encode(recvtype)));

    retval = simgrid::smpi::Colls::scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);

    TRACE_smpi_comm_out(rank);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not datatype->is_valid() || op == MPI_OP_NULL) {
    retval = MPI_ERR_ARG;
  } else {
    int rank = simgrid::s4u::this_actor::get_pid();

    TRACE_smpi_comm_in(rank, __func__,
                       new simgrid::instr::CollTIData("reduce", root, 0,
                                                      datatype->is_replayable() ? count : count * datatype->size(), -1,
                                                      simgrid::smpi::Datatype::encode(datatype), ""));

    simgrid::smpi::Colls::reduce(sendbuf, recvbuf, count, datatype, op, root, comm);

    retval = MPI_SUCCESS;
    TRACE_smpi_comm_out(rank);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Reduce_local(void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype, MPI_Op op){
  int retval = 0;

  smpi_bench_end();
  if (not datatype->is_valid() || op == MPI_OP_NULL) {
    retval = MPI_ERR_ARG;
  } else {
    op->apply(inbuf, inoutbuf, &count, datatype);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Allreduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {

    char* sendtmpbuf = static_cast<char*>(sendbuf);
    if( sendbuf == MPI_IN_PLACE ) {
      sendtmpbuf = static_cast<char*>(xbt_malloc(count*datatype->get_extent()));
      simgrid::smpi::Datatype::copy(recvbuf, count, datatype,sendtmpbuf, count, datatype);
    }
    int rank = simgrid::s4u::this_actor::get_pid();

    TRACE_smpi_comm_in(rank, __func__,
                       new simgrid::instr::CollTIData("allreduce", -1, 0,
                                                      datatype->is_replayable() ? count : count * datatype->size(), -1,
                                                      simgrid::smpi::Datatype::encode(datatype), ""));

    simgrid::smpi::Colls::allreduce(sendtmpbuf, recvbuf, count, datatype, op, comm);

    if( sendbuf == MPI_IN_PLACE )
      xbt_free(sendtmpbuf);

    retval = MPI_SUCCESS;
    TRACE_smpi_comm_out(rank);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Scan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {
    int rank = simgrid::s4u::this_actor::get_pid();
    void* sendtmpbuf = sendbuf;
    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(count * datatype->size()));
      memcpy(sendtmpbuf, recvbuf, count * datatype->size());
    }
    TRACE_smpi_comm_in(rank, __func__, new simgrid::instr::Pt2PtTIData(
                                           "scan", -1, datatype->is_replayable() ? count : count * datatype->size(),
                                           simgrid::smpi::Datatype::encode(datatype)));

    retval = simgrid::smpi::Colls::scan(sendtmpbuf, recvbuf, count, datatype, op, comm);

    TRACE_smpi_comm_out(rank);
    if (sendbuf == MPI_IN_PLACE)
      xbt_free(sendtmpbuf);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Exscan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm){
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {
    int rank         = simgrid::s4u::this_actor::get_pid();
    void* sendtmpbuf = sendbuf;
    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(count * datatype->size()));
      memcpy(sendtmpbuf, recvbuf, count * datatype->size());
    }

    TRACE_smpi_comm_in(rank, __func__, new simgrid::instr::Pt2PtTIData(
                                           "exscan", -1, datatype->is_replayable() ? count : count * datatype->size(),
                                           simgrid::smpi::Datatype::encode(datatype)));

    retval = simgrid::smpi::Colls::exscan(sendtmpbuf, recvbuf, count, datatype, op, comm);

    TRACE_smpi_comm_out(rank);
    if (sendbuf == MPI_IN_PLACE)
      xbt_free(sendtmpbuf);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Reduce_scatter(void *sendbuf, void *recvbuf, int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int retval = 0;
  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else if (recvcounts == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    int rank                           = simgrid::s4u::this_actor::get_pid();
    std::vector<int>* trace_recvcounts = new std::vector<int>;
    int dt_send_size                   = datatype->is_replayable() ? 1 : datatype->size();
    int totalcount    = 0;

    for (int i = 0; i < comm->size(); i++) { // copy data to avoid bad free
      trace_recvcounts->push_back(recvcounts[i] * dt_send_size);
      totalcount += recvcounts[i];
    }

    void* sendtmpbuf = sendbuf;
    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(totalcount * datatype->size()));
      memcpy(sendtmpbuf, recvbuf, totalcount * datatype->size());
    }

    TRACE_smpi_comm_in(rank, __func__, new simgrid::instr::VarCollTIData(
                                           "reducescatter", -1, dt_send_size, nullptr, -1, trace_recvcounts,
                                           simgrid::smpi::Datatype::encode(datatype), ""));

    simgrid::smpi::Colls::reduce_scatter(sendtmpbuf, recvbuf, recvcounts, datatype, op, comm);
    retval = MPI_SUCCESS;
    TRACE_smpi_comm_out(rank);

    if (sendbuf == MPI_IN_PLACE)
      xbt_free(sendtmpbuf);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Reduce_scatter_block(void *sendbuf, void *recvbuf, int recvcount,
                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int retval;
  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else if (recvcount < 0) {
    retval = MPI_ERR_ARG;
  } else {
    int count = comm->size();

    int rank                           = simgrid::s4u::this_actor::get_pid();
    int dt_send_size                   = datatype->is_replayable() ? 1 : datatype->size();
    std::vector<int>* trace_recvcounts = new std::vector<int>(recvcount * dt_send_size); // copy data to avoid bad free

    void* sendtmpbuf       = sendbuf;
    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(recvcount * count * datatype->size()));
      memcpy(sendtmpbuf, recvbuf, recvcount * count * datatype->size());
    }

    TRACE_smpi_comm_in(rank, __func__,
                       new simgrid::instr::VarCollTIData("reducescatter", -1, 0, nullptr, -1, trace_recvcounts,
                                                         simgrid::smpi::Datatype::encode(datatype), ""));

    int* recvcounts = new int[count];
    for (int i      = 0; i < count; i++)
      recvcounts[i] = recvcount;
    simgrid::smpi::Colls::reduce_scatter(sendtmpbuf, recvbuf, recvcounts, datatype, op, comm);
    delete[] recvcounts;
    retval = MPI_SUCCESS;

    TRACE_smpi_comm_out(rank);

    if (sendbuf == MPI_IN_PLACE)
      xbt_free(sendtmpbuf);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Alltoall(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                  MPI_Datatype recvtype, MPI_Comm comm)
{
  int retval = 0;
  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if ((sendbuf != MPI_IN_PLACE && sendtype == MPI_DATATYPE_NULL) || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else {
    int rank                 = simgrid::s4u::this_actor::get_pid();
    void* sendtmpbuf         = static_cast<char*>(sendbuf);
    int sendtmpcount         = sendcount;
    MPI_Datatype sendtmptype = sendtype;
    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(recvcount * comm->size() * recvtype->size()));
      memcpy(sendtmpbuf, recvbuf, recvcount * comm->size() * recvtype->size());
      sendtmpcount = recvcount;
      sendtmptype  = recvtype;
    }

    TRACE_smpi_comm_in(rank, __func__,
                       new simgrid::instr::CollTIData(
                           "alltoall", -1, -1.0,
                           sendtmptype->is_replayable() ? sendtmpcount : sendtmpcount * sendtmptype->size(),
                           recvtype->is_replayable() ? recvcount : recvcount * recvtype->size(),
                           simgrid::smpi::Datatype::encode(sendtmptype), simgrid::smpi::Datatype::encode(recvtype)));

    retval = simgrid::smpi::Colls::alltoall(sendtmpbuf, sendtmpcount, sendtmptype, recvbuf, recvcount, recvtype, comm);

    TRACE_smpi_comm_out(rank);

    if (sendbuf == MPI_IN_PLACE)
      xbt_free(sendtmpbuf);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Alltoallv(void* sendbuf, int* sendcounts, int* senddisps, MPI_Datatype sendtype, void* recvbuf,
                   int* recvcounts, int* recvdisps, MPI_Datatype recvtype, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (sendtype == MPI_DATATYPE_NULL || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if ((sendbuf != MPI_IN_PLACE && (sendcounts == nullptr || senddisps == nullptr)) || recvcounts == nullptr ||
             recvdisps == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    int rank                           = simgrid::s4u::this_actor::get_pid();
    int size               = comm->size();
    int send_size                      = 0;
    int recv_size                      = 0;
    std::vector<int>* trace_sendcounts = new std::vector<int>;
    std::vector<int>* trace_recvcounts = new std::vector<int>;
    int dt_size_recv       = recvtype->size();

    void* sendtmpbuf         = static_cast<char*>(sendbuf);
    int* sendtmpcounts       = sendcounts;
    int* sendtmpdisps        = senddisps;
    MPI_Datatype sendtmptype = sendtype;
    int maxsize              = 0;
    for (int i = 0; i < size; i++) { // copy data to avoid bad free
      recv_size += recvcounts[i] * dt_size_recv;
      trace_recvcounts->push_back(recvcounts[i] * dt_size_recv);
      if (((recvdisps[i] + recvcounts[i]) * dt_size_recv) > maxsize)
        maxsize = (recvdisps[i] + recvcounts[i]) * dt_size_recv;
    }

    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(maxsize));
      memcpy(sendtmpbuf, recvbuf, maxsize);
      sendtmpcounts = static_cast<int*>(xbt_malloc(size * sizeof(int)));
      memcpy(sendtmpcounts, recvcounts, size * sizeof(int));
      sendtmpdisps = static_cast<int*>(xbt_malloc(size * sizeof(int)));
      memcpy(sendtmpdisps, recvdisps, size * sizeof(int));
      sendtmptype = recvtype;
    }

    int dt_size_send = sendtmptype->size();

    for (int i = 0; i < size; i++) { // copy data to avoid bad free
      send_size += sendtmpcounts[i] * dt_size_send;
      trace_sendcounts->push_back(sendtmpcounts[i] * dt_size_send);
    }

    TRACE_smpi_comm_in(rank, __func__,
                       new simgrid::instr::VarCollTIData("alltoallv", -1, send_size, trace_sendcounts, recv_size,
                                                         trace_recvcounts, simgrid::smpi::Datatype::encode(sendtype),
                                                         simgrid::smpi::Datatype::encode(recvtype)));

    retval = simgrid::smpi::Colls::alltoallv(sendtmpbuf, sendtmpcounts, sendtmpdisps, sendtmptype, recvbuf, recvcounts,
                                    recvdisps, recvtype, comm);
    TRACE_smpi_comm_out(rank);

    if (sendbuf == MPI_IN_PLACE) {
      xbt_free(sendtmpbuf);
      xbt_free(sendtmpcounts);
      xbt_free(sendtmpdisps);
    }
  }

  smpi_bench_begin();
  return retval;
}

/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "smpi_comm.hpp"
#include "smpi_coll.hpp"
#include "smpi_datatype_derived.hpp"
#include "smpi_op.hpp"
#include "smpi_process.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smpi_pmpi);


/* PMPI User level calls */
extern "C" { // Obviously, the C MPI interface should use the C linkage

int PMPI_Bcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_ARG;
  } else {
    int rank        = smpi_process()->index();
    int root_traced = comm->group()->index(root);

    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_BCAST;
    extra->root            = root_traced;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = datatype->size();
    extra->send_size = count * dt_size_send;
    TRACE_smpi_collective_in(rank, __FUNCTION__, extra);
    if (comm->size() > 1)
      simgrid::smpi::Colls::bcast(buf, count, datatype, root, comm);
    retval = MPI_SUCCESS;

    TRACE_smpi_collective_out(rank, __FUNCTION__);
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
    int rank               = smpi_process()->index();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_BARRIER;
    TRACE_smpi_collective_in(rank, __FUNCTION__, extra);

    simgrid::smpi::Colls::barrier(comm);

    //Barrier can be used to synchronize RMA calls. Finish all requests from comm before.
    comm->finish_rma_calls();

    retval = MPI_SUCCESS;

    TRACE_smpi_collective_out(rank, __FUNCTION__);
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
    int rank               = smpi_process()->index();
    int root_traced        = comm->group()->index(root);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_GATHER;
    extra->root            = root_traced;
    int known              = 0;
    extra->datatype1       = encode_datatype(sendtmptype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = sendtmptype->size();
    extra->send_size = sendtmpcount * dt_size_send;
    extra->datatype2 = encode_datatype(recvtype, &known);
    int dt_size_recv = 1;
    if ((comm->rank() == root) && known == 0)
      dt_size_recv   = recvtype->size();
    extra->recv_size = recvcount * dt_size_recv;

    TRACE_smpi_collective_in(rank, __FUNCTION__, extra);

    simgrid::smpi::Colls::gather(sendtmpbuf, sendtmpcount, sendtmptype, recvbuf, recvcount, recvtype, root, comm);

    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, __FUNCTION__);
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
  } else if (recvcounts == nullptr || displs == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    char* sendtmpbuf = static_cast<char*>(sendbuf);
    int sendtmpcount = sendcount;
    MPI_Datatype sendtmptype = sendtype;
    if( (comm->rank() == root) && (sendbuf == MPI_IN_PLACE )) {
      sendtmpcount=0;
      sendtmptype=recvtype;
    }

    int rank               = smpi_process()->index();
    int root_traced        = comm->group()->index(root);
    int size               = comm->size();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_GATHERV;
    extra->num_processes   = size;
    extra->root            = root_traced;
    int known              = 0;
    extra->datatype1       = encode_datatype(sendtmptype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = sendtype->size();
    extra->send_size = sendtmpcount * dt_size_send;
    extra->datatype2 = encode_datatype(recvtype, &known);
    int dt_size_recv = 1;
    if (known == 0)
      dt_size_recv = recvtype->size();
    if (comm->rank() == root) {
      extra->recvcounts = xbt_new(int, size);
      for (int i = 0; i < size; i++) // copy data to avoid bad free
        extra->recvcounts[i] = recvcounts[i] * dt_size_recv;
    }
    TRACE_smpi_collective_in(rank, __FUNCTION__, extra);

    retval = simgrid::smpi::Colls::gatherv(sendtmpbuf, sendtmpcount, sendtmptype, recvbuf, recvcounts, displs, recvtype, root, comm);
    TRACE_smpi_collective_out(rank, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
  int retval = 0;

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
    int rank               = smpi_process()->index();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_ALLGATHER;
    int known              = 0;
    extra->datatype1       = encode_datatype(sendtype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = sendtype->size();
    extra->send_size = sendcount * dt_size_send;
    extra->datatype2 = encode_datatype(recvtype, &known);
    int dt_size_recv = 1;
    if (known == 0)
      dt_size_recv   = recvtype->size();
    extra->recv_size = recvcount * dt_size_recv;

    TRACE_smpi_collective_in(rank, __FUNCTION__, extra);

    simgrid::smpi::Colls::allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, __FUNCTION__);
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
    int rank               = smpi_process()->index();
    int i                  = 0;
    int size               = comm->size();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_ALLGATHERV;
    extra->num_processes   = size;
    int known              = 0;
    extra->datatype1       = encode_datatype(sendtype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = sendtype->size();
    extra->send_size = sendcount * dt_size_send;
    extra->datatype2 = encode_datatype(recvtype, &known);
    int dt_size_recv = 1;
    if (known == 0)
      dt_size_recv    = recvtype->size();
    extra->recvcounts = xbt_new(int, size);
    for (i                 = 0; i < size; i++) // copy data to avoid bad free
      extra->recvcounts[i] = recvcounts[i] * dt_size_recv;

    TRACE_smpi_collective_in(rank, __FUNCTION__, extra);

    simgrid::smpi::Colls::allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, __FUNCTION__);
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
    int rank               = smpi_process()->index();
    int root_traced        = comm->group()->index(root);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_SCATTER;
    extra->root            = root_traced;
    int known              = 0;
    extra->datatype1       = encode_datatype(sendtype, &known);
    int dt_size_send       = 1;
    if ((comm->rank() == root) && known == 0)
      dt_size_send   = sendtype->size();
    extra->send_size = sendcount * dt_size_send;
    extra->datatype2 = encode_datatype(recvtype, &known);
    int dt_size_recv = 1;
    if (known == 0)
      dt_size_recv   = recvtype->size();
    extra->recv_size = recvcount * dt_size_recv;
    TRACE_smpi_collective_in(rank, __FUNCTION__, extra);

    simgrid::smpi::Colls::scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, __FUNCTION__);
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
    int rank               = smpi_process()->index();
    int root_traced        = comm->group()->index(root);
    int size               = comm->size();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_SCATTERV;
    extra->num_processes   = size;
    extra->root            = root_traced;
    int known              = 0;
    extra->datatype1       = encode_datatype(sendtype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send = sendtype->size();
    if (comm->rank() == root) {
      extra->sendcounts = xbt_new(int, size);
      for (int i = 0; i < size; i++) // copy data to avoid bad free
        extra->sendcounts[i] = sendcounts[i] * dt_size_send;
    }
    extra->datatype2 = encode_datatype(recvtype, &known);
    int dt_size_recv = 1;
    if (known == 0)
      dt_size_recv   = recvtype->size();
    extra->recv_size = recvcount * dt_size_recv;
    TRACE_smpi_collective_in(rank, __FUNCTION__, extra);

    retval = simgrid::smpi::Colls::scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);

    TRACE_smpi_collective_out(rank, __FUNCTION__);
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
    int rank               = smpi_process()->index();
    int root_traced        = comm->group()->index(root);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_REDUCE;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = datatype->size();
    extra->send_size = count * dt_size_send;
    extra->root      = root_traced;

    TRACE_smpi_collective_in(rank, __FUNCTION__, extra);

    simgrid::smpi::Colls::reduce(sendbuf, recvbuf, count, datatype, op, root, comm);

    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, __FUNCTION__);
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
    int rank               = smpi_process()->index();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_ALLREDUCE;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = datatype->size();
    extra->send_size = count * dt_size_send;

    TRACE_smpi_collective_in(rank, __FUNCTION__, extra);

    simgrid::smpi::Colls::allreduce(sendtmpbuf, recvbuf, count, datatype, op, comm);

    if( sendbuf == MPI_IN_PLACE )
      xbt_free(sendtmpbuf);

    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, __FUNCTION__);
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
    int rank               = smpi_process()->index();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_SCAN;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = datatype->size();
    extra->send_size = count * dt_size_send;

    TRACE_smpi_collective_in(rank, __FUNCTION__, extra);

    retval = simgrid::smpi::Colls::scan(sendbuf, recvbuf, count, datatype, op, comm);

    TRACE_smpi_collective_out(rank, __FUNCTION__);
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
    int rank               = smpi_process()->index();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_EXSCAN;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send   = datatype->size();
    extra->send_size = count * dt_size_send;
    void* sendtmpbuf = sendbuf;
    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(count * datatype->size()));
      memcpy(sendtmpbuf, recvbuf, count * datatype->size());
    }
    TRACE_smpi_collective_in(rank, __FUNCTION__, extra);

    retval = simgrid::smpi::Colls::exscan(sendtmpbuf, recvbuf, count, datatype, op, comm);

    TRACE_smpi_collective_out(rank, __FUNCTION__);
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
    int rank               = smpi_process()->index();
    int i                  = 0;
    int size               = comm->size();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_REDUCE_SCATTER;
    extra->num_processes   = size;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send    = datatype->size();
    extra->send_size  = 0;
    extra->recvcounts = xbt_new(int, size);
    int totalcount    = 0;
    for (i = 0; i < size; i++) { // copy data to avoid bad free
      extra->recvcounts[i] = recvcounts[i] * dt_size_send;
      totalcount += recvcounts[i];
    }
    void* sendtmpbuf = sendbuf;
    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(totalcount * datatype->size()));
      memcpy(sendtmpbuf, recvbuf, totalcount * datatype->size());
    }

    TRACE_smpi_collective_in(rank, __FUNCTION__, extra);

    simgrid::smpi::Colls::reduce_scatter(sendtmpbuf, recvbuf, recvcounts, datatype, op, comm);
    retval = MPI_SUCCESS;
    TRACE_smpi_collective_out(rank, __FUNCTION__);

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

    int rank               = smpi_process()->index();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_REDUCE_SCATTER;
    extra->num_processes   = count;
    int known              = 0;
    extra->datatype1       = encode_datatype(datatype, &known);
    int dt_size_send       = 1;
    if (known == 0)
      dt_size_send    = datatype->size();
    extra->send_size  = 0;
    extra->recvcounts = xbt_new(int, count);
    for (int i             = 0; i < count; i++) // copy data to avoid bad free
      extra->recvcounts[i] = recvcount * dt_size_send;
    void* sendtmpbuf       = sendbuf;
    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(recvcount * count * datatype->size()));
      memcpy(sendtmpbuf, recvbuf, recvcount * count * datatype->size());
    }

    TRACE_smpi_collective_in(rank, __FUNCTION__, extra);

    int* recvcounts = static_cast<int*>(xbt_malloc(count * sizeof(int)));
    for (int i      = 0; i < count; i++)
      recvcounts[i] = recvcount;
    simgrid::smpi::Colls::reduce_scatter(sendtmpbuf, recvbuf, recvcounts, datatype, op, comm);
    xbt_free(recvcounts);
    retval = MPI_SUCCESS;

    TRACE_smpi_collective_out(rank, __FUNCTION__);

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
    int rank               = smpi_process()->index();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_ALLTOALL;

    void* sendtmpbuf         = static_cast<char*>(sendbuf);
    int sendtmpcount         = sendcount;
    MPI_Datatype sendtmptype = sendtype;
    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(recvcount * comm->size() * recvtype->size()));
      memcpy(sendtmpbuf, recvbuf, recvcount * comm->size() * recvtype->size());
      sendtmpcount = recvcount;
      sendtmptype  = recvtype;
    }

    int known        = 0;
    extra->datatype1 = encode_datatype(sendtmptype, &known);
    if (known == 0)
      extra->send_size = sendtmpcount * sendtmptype->size();
    else
      extra->send_size = sendtmpcount;
    extra->datatype2   = encode_datatype(recvtype, &known);
    if (known == 0)
      extra->recv_size = recvcount * recvtype->size();
    else
      extra->recv_size = recvcount;

    TRACE_smpi_collective_in(rank, __FUNCTION__, extra);

    retval = simgrid::smpi::Colls::alltoall(sendtmpbuf, sendtmpcount, sendtmptype, recvbuf, recvcount, recvtype, comm);

    TRACE_smpi_collective_out(rank, __FUNCTION__);

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
    int rank               = smpi_process()->index();
    int i                  = 0;
    int size               = comm->size();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t, 1);
    extra->type            = TRACING_ALLTOALLV;
    extra->send_size       = 0;
    extra->recv_size       = 0;
    extra->recvcounts      = xbt_new(int, size);
    extra->sendcounts      = xbt_new(int, size);
    int known              = 0;
    extra->datatype2       = encode_datatype(recvtype, &known);
    int dt_size_recv       = recvtype->size();

    void* sendtmpbuf         = static_cast<char*>(sendbuf);
    int* sendtmpcounts       = sendcounts;
    int* sendtmpdisps        = senddisps;
    MPI_Datatype sendtmptype = sendtype;
    int maxsize              = 0;
    for (i = 0; i < size; i++) { // copy data to avoid bad free
      extra->recv_size += recvcounts[i] * dt_size_recv;
      extra->recvcounts[i] = recvcounts[i] * dt_size_recv;
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

    extra->datatype1 = encode_datatype(sendtmptype, &known);
    int dt_size_send = sendtmptype->size();

    for (i = 0; i < size; i++) { // copy data to avoid bad free
      extra->send_size += sendtmpcounts[i] * dt_size_send;
      extra->sendcounts[i] = sendtmpcounts[i] * dt_size_send;
    }
    extra->num_processes = size;
    TRACE_smpi_collective_in(rank, __FUNCTION__, extra);
    retval = simgrid::smpi::Colls::alltoallv(sendtmpbuf, sendtmpcounts, sendtmpdisps, sendtmptype, recvbuf, recvcounts,
                                    recvdisps, recvtype, comm);
    TRACE_smpi_collective_out(rank, __FUNCTION__);

    if (sendbuf == MPI_IN_PLACE) {
      xbt_free(sendtmpbuf);
      xbt_free(sendtmpcounts);
      xbt_free(sendtmpdisps);
    }
  }

  smpi_bench_begin();
  return retval;
}

}

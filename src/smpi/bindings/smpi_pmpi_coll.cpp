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

/* PMPI User level calls */

int PMPI_Barrier(MPI_Comm comm)
{
  return PMPI_Ibarrier(comm, MPI_REQUEST_IGNORED);
}

int PMPI_Ibarrier(MPI_Comm comm, MPI_Request *request)
{
  int retval = 0;
  smpi_bench_end();
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if(request == nullptr){
    retval = MPI_ERR_ARG;
  }else{
    int rank = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(rank, request==MPI_REQUEST_IGNORED? "PMPI_Barrier" : "PMPI_Ibarrier", new simgrid::instr::NoOpTIData(request==MPI_REQUEST_IGNORED? "barrier" : "ibarrier"));
    if(request==MPI_REQUEST_IGNORED){
      simgrid::smpi::Colls::barrier(comm);
      //Barrier can be used to synchronize RMA calls. Finish all requests from comm before.
      comm->finish_rma_calls();
    } else
      simgrid::smpi::Colls::ibarrier(comm, request);
    TRACE_smpi_comm_out(rank);
  }    
  smpi_bench_begin();
  return retval;
}

int PMPI_Bcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
  return PMPI_Ibcast(buf, count, datatype, root, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Ibcast(void *buf, int count, MPI_Datatype datatype, 
                   int root, MPI_Comm comm, MPI_Request* request)
{
  if (comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } if (buf == nullptr && count > 0) {
    return MPI_ERR_BUFFER;
  } else if (datatype == MPI_DATATYPE_NULL || not datatype->is_valid()) {
    return MPI_ERR_TYPE;
  } else if (count < 0){
    return MPI_ERR_COUNT;
  } else if (root < 0 || root >= comm->size()){
    return MPI_ERR_ROOT;
  }  else if (request == nullptr){
    return MPI_ERR_ARG;
  } else {
    smpi_bench_end();
    int rank = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(rank, request==MPI_REQUEST_IGNORED?"PMPI_Bcast":"PMPI_Ibcast",
                       new simgrid::instr::CollTIData(request==MPI_REQUEST_IGNORED?"bcast":"ibcast", root, -1.0,
                                                      datatype->is_replayable() ? count : count * datatype->size(), -1,
                                                      simgrid::smpi::Datatype::encode(datatype), ""));
    if (comm->size() > 1){
      if(request==MPI_REQUEST_IGNORED)
        simgrid::smpi::Colls::bcast(buf, count, datatype, root, comm);
      else
        simgrid::smpi::Colls::ibcast(buf, count, datatype, root, comm, request);
    } else {
      if(request!=MPI_REQUEST_IGNORED)
        *request = MPI_REQUEST_NULL;
    }
    TRACE_smpi_comm_out(rank);
    smpi_bench_begin();
    return MPI_SUCCESS;
  }
}

int PMPI_Gather(void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount, MPI_Datatype recvtype,
                int root, MPI_Comm comm){
  return PMPI_Igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Igather(void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount, MPI_Datatype recvtype,
                int root, MPI_Comm comm, MPI_Request *request)
{
  if (comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else if ((sendbuf == nullptr) || ((comm->rank() == root) && recvbuf == nullptr)) {
    return MPI_ERR_BUFFER;
  } else if (((sendbuf != MPI_IN_PLACE && sendcount > 0) && (sendtype == MPI_DATATYPE_NULL)) ||
            ((comm->rank() == root) && (recvtype == MPI_DATATYPE_NULL))){
    return MPI_ERR_TYPE;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendcount <0)) || ((comm->rank() == root) && (recvcount <0))){
    return MPI_ERR_COUNT;
  } else if (root < 0 || root >= comm->size()){
    return MPI_ERR_ROOT;
  } else if (request == nullptr){
    return MPI_ERR_ARG;
  }  else {
    smpi_bench_end();
    char* sendtmpbuf = static_cast<char*>(sendbuf);
    int sendtmpcount = sendcount;
    MPI_Datatype sendtmptype = sendtype;
    if( (comm->rank() == root) && (sendbuf == MPI_IN_PLACE )) {
      sendtmpcount=0;
      sendtmptype=recvtype;
    }
    int rank = simgrid::s4u::this_actor::get_pid();

    TRACE_smpi_comm_in(
        rank, request==MPI_REQUEST_IGNORED?"PMPI_Gather":"PMPI_Igather",
        new simgrid::instr::CollTIData(
            request==MPI_REQUEST_IGNORED ? "gather":"igather", root, -1.0, sendtmptype->is_replayable() ? sendtmpcount : sendtmpcount * sendtmptype->size(),
            (comm->rank() != root || recvtype->is_replayable()) ? recvcount : recvcount * recvtype->size(),
            simgrid::smpi::Datatype::encode(sendtmptype), simgrid::smpi::Datatype::encode(recvtype)));
    if(request == MPI_REQUEST_IGNORED)
      simgrid::smpi::Colls::gather(sendtmpbuf, sendtmpcount, sendtmptype, recvbuf, recvcount, recvtype, root, comm);
    else
      simgrid::smpi::Colls::igather(sendtmpbuf, sendtmpcount, sendtmptype, recvbuf, recvcount, recvtype, root, comm, request);

    TRACE_smpi_comm_out(rank);
    smpi_bench_begin();
    return MPI_SUCCESS;
  }
}

int PMPI_Gatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *displs,
                MPI_Datatype recvtype, int root, MPI_Comm comm){
  return PMPI_Igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Igatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *displs,
                MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
  if (comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else if ((sendbuf == nullptr && sendcount > 0) || ((comm->rank() == root) && recvbuf == nullptr)) {
    return MPI_ERR_BUFFER;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendtype == MPI_DATATYPE_NULL)) ||
            ((comm->rank() == root) && (recvtype == MPI_DATATYPE_NULL))){
    return MPI_ERR_TYPE;
  } else if (( sendbuf != MPI_IN_PLACE) && (sendcount <0)){
    return MPI_ERR_COUNT;
  } else if ((comm->rank() == root) && (recvcounts == nullptr || displs == nullptr)) {
    return MPI_ERR_ARG;
  } else if (root < 0 || root >= comm->size()){
    return MPI_ERR_ROOT;
  } else if (request == nullptr){
    return MPI_ERR_ARG;
  }  else {
    for (int i = 0; i < comm->size(); i++){
      if((comm->rank() == root) && (recvcounts[i]<0))
        return MPI_ERR_COUNT;
    }

    smpi_bench_end();
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

    TRACE_smpi_comm_in(rank, request==MPI_REQUEST_IGNORED?"PMPI_Gatherv":"PMPI_Igatherv",
                       new simgrid::instr::VarCollTIData(
                           request==MPI_REQUEST_IGNORED ? "gatherv":"igatherv", root,
                           sendtmptype->is_replayable() ? sendtmpcount : sendtmpcount * sendtmptype->size(), nullptr,
                           dt_size_recv, trace_recvcounts, simgrid::smpi::Datatype::encode(sendtmptype),
                           simgrid::smpi::Datatype::encode(recvtype)));
    if(request == MPI_REQUEST_IGNORED)
      simgrid::smpi::Colls::gatherv(sendtmpbuf, sendtmpcount, sendtmptype, recvbuf, recvcounts, displs, recvtype, root, comm);
    else
      simgrid::smpi::Colls::igatherv(sendtmpbuf, sendtmpcount, sendtmptype, recvbuf, recvcounts, displs, recvtype, root, comm, request);
    TRACE_smpi_comm_out(rank);
    smpi_bench_begin();
    return MPI_SUCCESS;
  }
}

int PMPI_Allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm){
  return PMPI_Iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPI_REQUEST_IGNORED);
}
              
int PMPI_Iallgather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* request)
{
  int retval = MPI_SUCCESS;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if ((sendbuf == nullptr && sendcount > 0) || (recvbuf == nullptr)){
    retval = MPI_ERR_BUFFER;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendtype == MPI_DATATYPE_NULL)) ||
            (recvtype == MPI_DATATYPE_NULL)){
    retval = MPI_ERR_TYPE;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendcount <0)) ||
            (recvcount <0)){
    retval = MPI_ERR_COUNT;
  } else if (request == nullptr){
    retval = MPI_ERR_ARG;
  }  else {
    if(sendbuf == MPI_IN_PLACE) {
      sendbuf=static_cast<char*>(recvbuf)+recvtype->get_extent()*recvcount*comm->rank();
      sendcount=recvcount;
      sendtype=recvtype;
    }
    int rank = simgrid::s4u::this_actor::get_pid();

    TRACE_smpi_comm_in(rank, request==MPI_REQUEST_IGNORED?"PMPI_Allgather":"PMPI_Iallggather",
                       new simgrid::instr::CollTIData(
                           request==MPI_REQUEST_IGNORED ? "allgather" : "iallgather", -1, -1.0, sendtype->is_replayable() ? sendcount : sendcount * sendtype->size(),
                           recvtype->is_replayable() ? recvcount : recvcount * recvtype->size(),
                           simgrid::smpi::Datatype::encode(sendtype), simgrid::smpi::Datatype::encode(recvtype)));
    if(request == MPI_REQUEST_IGNORED)
      simgrid::smpi::Colls::allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    else
      simgrid::smpi::Colls::iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
    TRACE_smpi_comm_out(rank);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int *recvcounts, int *displs, MPI_Datatype recvtype, MPI_Comm comm){
  return PMPI_Iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Iallgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int *recvcounts, int *displs, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* request)
{
  if (comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else if ((sendbuf == nullptr && sendcount > 0) || (recvbuf == nullptr)){
    return MPI_ERR_BUFFER;
  } else if (((sendbuf != MPI_IN_PLACE) && (sendtype == MPI_DATATYPE_NULL)) || (recvtype == MPI_DATATYPE_NULL)) {
    return MPI_ERR_TYPE;
  } else if (( sendbuf != MPI_IN_PLACE) && (sendcount <0)){
    return MPI_ERR_COUNT;
  } else if (recvcounts == nullptr || displs == nullptr) {
    return MPI_ERR_ARG;
  } else if (request == nullptr){
    return MPI_ERR_ARG;
  }  else {
    for (int i = 0; i < comm->size(); i++){ // copy data to avoid bad free
      if (recvcounts[i] < 0)
        return MPI_ERR_COUNT;
    }

    smpi_bench_end();
    if(sendbuf == MPI_IN_PLACE) {
      sendbuf=static_cast<char*>(recvbuf)+recvtype->get_extent()*displs[comm->rank()];
      sendcount=recvcounts[comm->rank()];
      sendtype=recvtype;
    }
    int rank               = simgrid::s4u::this_actor::get_pid();
    int dt_size_recv       = recvtype->is_replayable() ? 1 : recvtype->size();

    std::vector<int>* trace_recvcounts = new std::vector<int>;
    for (int i = 0; i < comm->size(); i++){ // copy data to avoid bad free
      trace_recvcounts->push_back(recvcounts[i] * dt_size_recv);
    }

    TRACE_smpi_comm_in(rank, request==MPI_REQUEST_IGNORED?"PMPI_Allgatherv":"PMPI_Iallgatherv",
                       new simgrid::instr::VarCollTIData(
                           request==MPI_REQUEST_IGNORED ? "allgatherv" : "iallgatherv", -1, sendtype->is_replayable() ? sendcount : sendcount * sendtype->size(),
                           nullptr, dt_size_recv, trace_recvcounts, simgrid::smpi::Datatype::encode(sendtype),
                           simgrid::smpi::Datatype::encode(recvtype)));
    if(request == MPI_REQUEST_IGNORED)
      simgrid::smpi::Colls::allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
    else
      simgrid::smpi::Colls::iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, request);
    TRACE_smpi_comm_out(rank);
    smpi_bench_begin();
    return MPI_SUCCESS;
  }
}

int PMPI_Scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm){
  return PMPI_Iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Iscatter(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request* request)
{
  if (comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else if (((comm->rank() == root) && (sendtype == MPI_DATATYPE_NULL || not sendtype->is_valid())) ||
             ((recvbuf != MPI_IN_PLACE) && (recvtype == MPI_DATATYPE_NULL || not recvtype->is_valid()))) {
    return MPI_ERR_TYPE;
  } else if (((comm->rank() == root) && (sendcount < 0)) ||
             ((recvbuf != MPI_IN_PLACE) && (recvcount < 0))) {
    return MPI_ERR_COUNT;
  } else if ((sendbuf == recvbuf) ||
      ((comm->rank()==root) && sendcount>0 && (sendbuf == nullptr)) || 
      (recvcount > 0 && recvbuf == nullptr)){
    return MPI_ERR_BUFFER;
  } else if (root < 0 || root >= comm->size()){
    return MPI_ERR_ROOT;
  } else if (request == nullptr){
    return MPI_ERR_ARG;
  } else {
    smpi_bench_end();
    if (recvbuf == MPI_IN_PLACE) {
      recvtype  = sendtype;
      recvcount = sendcount;
    }
    int rank = simgrid::s4u::this_actor::get_pid();

    TRACE_smpi_comm_in(
        rank, request==MPI_REQUEST_IGNORED?"PMPI_Scatter":"PMPI_Iscatter",
        new simgrid::instr::CollTIData(
            request==MPI_REQUEST_IGNORED ? "scatter" : "iscatter", root, -1.0,
            (comm->rank() != root || sendtype->is_replayable()) ? sendcount : sendcount * sendtype->size(),
            recvtype->is_replayable() ? recvcount : recvcount * recvtype->size(),
            simgrid::smpi::Datatype::encode(sendtype), simgrid::smpi::Datatype::encode(recvtype)));
    if(request == MPI_REQUEST_IGNORED)
      simgrid::smpi::Colls::scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    else
      simgrid::smpi::Colls::iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
    TRACE_smpi_comm_out(rank);
    smpi_bench_begin();
    return MPI_SUCCESS;
  }
}

int PMPI_Scatterv(void *sendbuf, int *sendcounts, int *displs,
                 MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm){
  return PMPI_Iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Iscatterv(void *sendbuf, int *sendcounts, int *displs,
                 MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
  if (comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else if (sendcounts == nullptr || displs == nullptr) {
    return MPI_ERR_ARG;
  } else if (((comm->rank() == root) && (sendtype == MPI_DATATYPE_NULL)) ||
             ((recvbuf != MPI_IN_PLACE) && (recvtype == MPI_DATATYPE_NULL))) {
    return MPI_ERR_TYPE;
  } else if (request == nullptr){
    return MPI_ERR_ARG;
  } else if (recvbuf != MPI_IN_PLACE && recvcount < 0){
    return MPI_ERR_COUNT;
  } else if (root < 0 || root >= comm->size()){
    return MPI_ERR_ROOT;
  } else {
    if (comm->rank() == root){
      if(recvbuf == MPI_IN_PLACE) {
      recvtype  = sendtype;
      recvcount = sendcounts[comm->rank()];
      }
      for (int i = 0; i < comm->size(); i++){
        if(sendcounts[i]<0)
          return MPI_ERR_COUNT;
      }
    }
    smpi_bench_end();

    int rank               = simgrid::s4u::this_actor::get_pid();
    int dt_size_send       = sendtype->is_replayable() ? 1 : sendtype->size();

    std::vector<int>* trace_sendcounts = new std::vector<int>;
    if (comm->rank() == root) {
      for (int i = 0; i < comm->size(); i++){ // copy data to avoid bad free
        trace_sendcounts->push_back(sendcounts[i] * dt_size_send);
      }
    }

    TRACE_smpi_comm_in(rank, request==MPI_REQUEST_IGNORED?"PMPI_Scatterv":"PMPI_Iscatterv",
                       new simgrid::instr::VarCollTIData(
                           request==MPI_REQUEST_IGNORED ? "scatterv":"iscatterv", root, dt_size_send, trace_sendcounts,
                           recvtype->is_replayable() ? recvcount : recvcount * recvtype->size(), nullptr,
                           simgrid::smpi::Datatype::encode(sendtype), simgrid::smpi::Datatype::encode(recvtype)));
    if(request == MPI_REQUEST_IGNORED)
      simgrid::smpi::Colls::scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);
    else
      simgrid::smpi::Colls::iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
    TRACE_smpi_comm_out(rank);
    smpi_bench_begin();
    return MPI_SUCCESS;
  }

}

int PMPI_Reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
  return PMPI_Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Ireduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request* request)
{
  if (comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else if ((sendbuf == nullptr && count > 0) || ((comm->rank() == root) && recvbuf == nullptr)) {
    return MPI_ERR_BUFFER;
  } else if (datatype == MPI_DATATYPE_NULL || not datatype->is_valid()){
    return MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    return MPI_ERR_OP;
  } else if (request == nullptr){
    return MPI_ERR_ARG;
  } else if (root < 0 || root >= comm->size()){
    return MPI_ERR_ROOT;
  } else if (count < 0){
    return MPI_ERR_COUNT;
  } else {
    smpi_bench_end();
    int rank = simgrid::s4u::this_actor::get_pid();

    TRACE_smpi_comm_in(rank, request==MPI_REQUEST_IGNORED ? "PMPI_Reduce":"PMPI_Ireduce",
                       new simgrid::instr::CollTIData(request==MPI_REQUEST_IGNORED ? "reduce":"ireduce", root, 0,
                                                      datatype->is_replayable() ? count : count * datatype->size(), -1,
                                                      simgrid::smpi::Datatype::encode(datatype), ""));
    if(request == MPI_REQUEST_IGNORED)
      simgrid::smpi::Colls::reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
    else
      simgrid::smpi::Colls::ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, request);

    TRACE_smpi_comm_out(rank);
    smpi_bench_begin();
    return MPI_SUCCESS;
  }
}

int PMPI_Reduce_local(void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype, MPI_Op op){
  int retval = 0;

  smpi_bench_end();
  if (datatype == MPI_DATATYPE_NULL || not datatype->is_valid()){
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else if (count < 0){
    retval = MPI_ERR_COUNT;
  }  else {
    op->apply(inbuf, inoutbuf, &count, datatype);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Allreduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return PMPI_Iallreduce(sendbuf, recvbuf, count, datatype, op, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Iallreduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
  if (comm == MPI_COMM_NULL) {
    return MPI_ERR_COMM;
  } else if ((sendbuf == nullptr && count > 0) || (recvbuf == nullptr)) {
    return MPI_ERR_BUFFER;
  } else if (datatype == MPI_DATATYPE_NULL || not datatype->is_valid()) {
    return MPI_ERR_TYPE;
  } else if (count < 0){
    return MPI_ERR_COUNT;
  } else if (op == MPI_OP_NULL) {
    return MPI_ERR_OP;
  } else if (request == nullptr){
    return MPI_ERR_ARG;
  } else {
    smpi_bench_end();
    char* sendtmpbuf = static_cast<char*>(sendbuf);
    if( sendbuf == MPI_IN_PLACE ) {
      sendtmpbuf = static_cast<char*>(xbt_malloc(count*datatype->get_extent()));
      simgrid::smpi::Datatype::copy(recvbuf, count, datatype,sendtmpbuf, count, datatype);
    }
    int rank = simgrid::s4u::this_actor::get_pid();

    TRACE_smpi_comm_in(rank, request==MPI_REQUEST_IGNORED ?"PMPI_Allreduce":"PMPI_Iallreduce",
                       new simgrid::instr::CollTIData(request==MPI_REQUEST_IGNORED ? "allreduce":"iallreduce", -1, 0,
                                                      datatype->is_replayable() ? count : count * datatype->size(), -1,
                                                      simgrid::smpi::Datatype::encode(datatype), ""));

    if(request == MPI_REQUEST_IGNORED)
      simgrid::smpi::Colls::allreduce(sendtmpbuf, recvbuf, count, datatype, op, comm);
    else
      simgrid::smpi::Colls::iallreduce(sendtmpbuf, recvbuf, count, datatype, op, comm, request);

    if( sendbuf == MPI_IN_PLACE )
      xbt_free(sendtmpbuf);

    TRACE_smpi_comm_out(rank);
    smpi_bench_begin();
    return MPI_SUCCESS;
  }
}

int PMPI_Scan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return PMPI_Iscan(sendbuf, recvbuf, count, datatype, op, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Iscan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request* request)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (datatype == MPI_DATATYPE_NULL || not datatype->is_valid()){
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else if (request == nullptr){
    retval = MPI_ERR_ARG;
  } else if (count < 0){
    retval = MPI_ERR_COUNT;
  } else if (sendbuf == nullptr || recvbuf == nullptr){
    retval = MPI_ERR_BUFFER;
  } else {
    int rank = simgrid::s4u::this_actor::get_pid();
    void* sendtmpbuf = sendbuf;
    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(count * datatype->size()));
      memcpy(sendtmpbuf, recvbuf, count * datatype->size());
    }
    TRACE_smpi_comm_in(rank, request==MPI_REQUEST_IGNORED ? "PMPI_Scan" : "PMPI_Iscan", new simgrid::instr::Pt2PtTIData(
                                           request==MPI_REQUEST_IGNORED ? "scan":"iscan", -1, 
                                           datatype->is_replayable() ? count : count * datatype->size(),
                                           simgrid::smpi::Datatype::encode(datatype)));

    if(request == MPI_REQUEST_IGNORED)
      retval = simgrid::smpi::Colls::scan(sendtmpbuf, recvbuf, count, datatype, op, comm);
    else
      retval = simgrid::smpi::Colls::iscan(sendtmpbuf, recvbuf, count, datatype, op, comm, request);

    TRACE_smpi_comm_out(rank);
    if (sendbuf == MPI_IN_PLACE)
      xbt_free(sendtmpbuf);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Exscan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return PMPI_Iexscan(sendbuf, recvbuf, count, datatype, op, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Iexscan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request* request){
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else if (request == nullptr){
    retval = MPI_ERR_ARG;
  } else if (count < 0){
    retval = MPI_ERR_COUNT;
  } else if (sendbuf == nullptr || recvbuf == nullptr){
    retval = MPI_ERR_BUFFER;
  } else {
    int rank         = simgrid::s4u::this_actor::get_pid();
    void* sendtmpbuf = sendbuf;
    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(count * datatype->size()));
      memcpy(sendtmpbuf, recvbuf, count * datatype->size());
    }

    TRACE_smpi_comm_in(rank, request==MPI_REQUEST_IGNORED ? "PMPI_Exscan" : "PMPI_Iexscan", new simgrid::instr::Pt2PtTIData(
                                           request==MPI_REQUEST_IGNORED ? "exscan":"iexscan", -1, datatype->is_replayable() ? count : count * datatype->size(),
                                           simgrid::smpi::Datatype::encode(datatype)));

    if(request == MPI_REQUEST_IGNORED)
      retval = simgrid::smpi::Colls::exscan(sendtmpbuf, recvbuf, count, datatype, op, comm);
    else
      retval = simgrid::smpi::Colls::iexscan(sendtmpbuf, recvbuf, count, datatype, op, comm, request);

    TRACE_smpi_comm_out(rank);
    if (sendbuf == MPI_IN_PLACE)
      xbt_free(sendtmpbuf);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Reduce_scatter(void *sendbuf, void *recvbuf, int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return PMPI_Ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Ireduce_scatter(void *sendbuf, void *recvbuf, int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
  int retval = 0;

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if ((sendbuf == nullptr) || (recvbuf == nullptr)) {
    retval =  MPI_ERR_BUFFER;
  } else if (datatype == MPI_DATATYPE_NULL || not datatype->is_valid()){
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else if (recvcounts == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (request == nullptr){
    retval = MPI_ERR_ARG;
  } else {
    for (int i = 0; i < comm->size(); i++) { // copy data to avoid bad free
      if(recvcounts[i]<0)
        return MPI_ERR_COUNT;
    }
    smpi_bench_end();

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

    TRACE_smpi_comm_in(rank, request==MPI_REQUEST_IGNORED ? "PMPI_Reduce_scatter": "PMPI_Ireduce_scatter", new simgrid::instr::VarCollTIData(
                                           request==MPI_REQUEST_IGNORED ? "reducescatter":"ireducescatter", -1, dt_send_size, nullptr, -1, trace_recvcounts,
                                           simgrid::smpi::Datatype::encode(datatype), ""));

    if(request == MPI_REQUEST_IGNORED)
      simgrid::smpi::Colls::reduce_scatter(sendtmpbuf, recvbuf, recvcounts, datatype, op, comm);
    else
      simgrid::smpi::Colls::ireduce_scatter(sendtmpbuf, recvbuf, recvcounts, datatype, op, comm, request);

    retval = MPI_SUCCESS;
    TRACE_smpi_comm_out(rank);

    if (sendbuf == MPI_IN_PLACE)
      xbt_free(sendtmpbuf);
    smpi_bench_begin();
  }

  return retval;
}

int PMPI_Reduce_scatter_block(void *sendbuf, void *recvbuf, int recvcount,
                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return PMPI_Ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Ireduce_scatter_block(void *sendbuf, void *recvbuf, int recvcount,
                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
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
  } else if (request == nullptr){
    retval = MPI_ERR_ARG;
  }  else {
    int count = comm->size();

    int rank                           = simgrid::s4u::this_actor::get_pid();
    int dt_send_size                   = datatype->is_replayable() ? 1 : datatype->size();
    std::vector<int>* trace_recvcounts = new std::vector<int>(recvcount * dt_send_size); // copy data to avoid bad free

    void* sendtmpbuf       = sendbuf;
    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(recvcount * count * datatype->size()));
      memcpy(sendtmpbuf, recvbuf, recvcount * count * datatype->size());
    }

    TRACE_smpi_comm_in(rank, request==MPI_REQUEST_IGNORED ? "PMPI_Reduce_scatter_block":"PMPI_Ireduce_scatter_block",
                       new simgrid::instr::VarCollTIData(request==MPI_REQUEST_IGNORED ? "reducescatter":"ireducescatter", -1, 0, nullptr, -1, trace_recvcounts,
                                                         simgrid::smpi::Datatype::encode(datatype), ""));

    int* recvcounts = new int[count];
    for (int i      = 0; i < count; i++)
      recvcounts[i] = recvcount;
    if(request == MPI_REQUEST_IGNORED)
      simgrid::smpi::Colls::reduce_scatter(sendtmpbuf, recvbuf, recvcounts, datatype, op, comm);
    else
      simgrid::smpi::Colls::ireduce_scatter(sendtmpbuf, recvbuf, recvcounts, datatype, op, comm, request);
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
                  MPI_Datatype recvtype, MPI_Comm comm){
  return PMPI_Ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, MPI_REQUEST_IGNORED);
}
                  
int PMPI_Ialltoall(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                  MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
  int retval = 0;
  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if ((sendbuf == nullptr && sendcount > 0) || (recvbuf == nullptr && recvcount > 0)) {
    retval = MPI_ERR_BUFFER;
  } else if ((sendbuf != MPI_IN_PLACE && sendtype == MPI_DATATYPE_NULL) || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if ((sendbuf != MPI_IN_PLACE && sendcount < 0) || recvcount < 0){
    retval = MPI_ERR_COUNT;
  } else if (request == nullptr){
    retval = MPI_ERR_ARG;
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

    TRACE_smpi_comm_in(rank, request==MPI_REQUEST_IGNORED?"PMPI_Alltoall":"PMPI_Ialltoall",
                       new simgrid::instr::CollTIData(
                           request==MPI_REQUEST_IGNORED ? "alltoall" : "ialltoall", -1, -1.0,
                           sendtmptype->is_replayable() ? sendtmpcount : sendtmpcount * sendtmptype->size(),
                           recvtype->is_replayable() ? recvcount : recvcount * recvtype->size(),
                           simgrid::smpi::Datatype::encode(sendtmptype), simgrid::smpi::Datatype::encode(recvtype)));
    if(request == MPI_REQUEST_IGNORED)
      retval = simgrid::smpi::Colls::alltoall(sendtmpbuf, sendtmpcount, sendtmptype, recvbuf, recvcount, recvtype, comm);
    else
      retval = simgrid::smpi::Colls::ialltoall(sendtmpbuf, sendtmpcount, sendtmptype, recvbuf, recvcount, recvtype, comm, request);

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
  return PMPI_Ialltoallv(sendbuf, sendcounts, senddisps, sendtype, recvbuf, recvcounts, recvdisps, recvtype, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Ialltoallv(void* sendbuf, int* sendcounts, int* senddisps, MPI_Datatype sendtype, void* recvbuf,
                   int* recvcounts, int* recvdisps, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
  int retval = 0;
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (sendbuf == nullptr || recvbuf == nullptr) {
    retval = MPI_ERR_BUFFER;
  } else if ((sendbuf != MPI_IN_PLACE && sendtype == MPI_DATATYPE_NULL)  || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if ((sendbuf != MPI_IN_PLACE && (sendcounts == nullptr || senddisps == nullptr)) || recvcounts == nullptr ||
             recvdisps == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (request == nullptr){
    retval = MPI_ERR_ARG;
  }  else {
    int rank                           = simgrid::s4u::this_actor::get_pid();
    int size               = comm->size();
    for (int i = 0; i < size; i++) {
      if (recvcounts[i] <0 || (sendbuf != MPI_IN_PLACE && sendcounts[i]<0))
        return MPI_ERR_COUNT;
    }
    smpi_bench_end();
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

    TRACE_smpi_comm_in(rank, request==MPI_REQUEST_IGNORED?"PMPI_Alltoallv":"PMPI_Ialltoallv",
                       new simgrid::instr::VarCollTIData(request==MPI_REQUEST_IGNORED ? "alltoallv":"ialltoallv", -1, send_size, trace_sendcounts, recv_size,
                                                         trace_recvcounts, simgrid::smpi::Datatype::encode(sendtmptype),
                                                         simgrid::smpi::Datatype::encode(recvtype)));

    if(request == MPI_REQUEST_IGNORED)
      retval = simgrid::smpi::Colls::alltoallv(sendtmpbuf, sendtmpcounts, sendtmpdisps, sendtmptype, recvbuf, recvcounts,
                                    recvdisps, recvtype, comm);
    else
      retval = simgrid::smpi::Colls::ialltoallv(sendtmpbuf, sendtmpcounts, sendtmpdisps, sendtmptype, recvbuf, recvcounts,
                                    recvdisps, recvtype, comm, request);
    TRACE_smpi_comm_out(rank);

    if (sendbuf == MPI_IN_PLACE) {
      xbt_free(sendtmpbuf);
      xbt_free(sendtmpcounts);
      xbt_free(sendtmpdisps);
    }
    smpi_bench_begin();
  }
  return retval;
}

int PMPI_Alltoallw(void* sendbuf, int* sendcounts, int* senddisps, MPI_Datatype* sendtypes, void* recvbuf,
                   int* recvcounts, int* recvdisps, MPI_Datatype* recvtypes, MPI_Comm comm)
{
  return PMPI_Ialltoallw(sendbuf, sendcounts, senddisps, sendtypes, recvbuf, recvcounts, recvdisps, recvtypes, comm, MPI_REQUEST_IGNORED);
}

int PMPI_Ialltoallw(void* sendbuf, int* sendcounts, int* senddisps, MPI_Datatype* sendtypes, void* recvbuf,
                   int* recvcounts, int* recvdisps, MPI_Datatype* recvtypes, MPI_Comm comm, MPI_Request *request)
{
  int retval = 0;
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (sendbuf == nullptr || recvbuf == nullptr) {
    retval = MPI_ERR_BUFFER;
  } else if ((sendbuf != MPI_IN_PLACE && sendtypes == nullptr)  || recvtypes == nullptr) {
    retval = MPI_ERR_TYPE;
  } else if ((sendbuf != MPI_IN_PLACE && (sendcounts == nullptr || senddisps == nullptr)) || recvcounts == nullptr ||
             recvdisps == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (request == nullptr){
    retval = MPI_ERR_ARG;
  }  else {
    smpi_bench_end();
    int rank                           = simgrid::s4u::this_actor::get_pid();
    int size                           = comm->size();
    for (int i = 0; i < size; i++) { // copy data to avoid bad free
      if (recvcounts[i] <0 || (sendbuf != MPI_IN_PLACE && sendcounts[i]<0))
        return MPI_ERR_COUNT;
    }
    int send_size                      = 0;
    int recv_size                      = 0;
    std::vector<int>* trace_sendcounts = new std::vector<int>;
    std::vector<int>* trace_recvcounts = new std::vector<int>;

    void* sendtmpbuf           = static_cast<char*>(sendbuf);
    int* sendtmpcounts         = sendcounts;
    int* sendtmpdisps          = senddisps;
    MPI_Datatype* sendtmptypes = sendtypes;
    unsigned long maxsize                = 0;
    for (int i = 0; i < size; i++) { // copy data to avoid bad free
      if(recvtypes[i]==MPI_DATATYPE_NULL){
        delete trace_recvcounts;
        delete trace_sendcounts;
        return MPI_ERR_TYPE;
      }
      recv_size += recvcounts[i] * recvtypes[i]->size();
      trace_recvcounts->push_back(recvcounts[i] * recvtypes[i]->size());
      if ((recvdisps[i] + (recvcounts[i] * recvtypes[i]->size())) > maxsize)
        maxsize = recvdisps[i] + (recvcounts[i] * recvtypes[i]->size());
    }

    if (sendbuf == MPI_IN_PLACE) {
      sendtmpbuf = static_cast<void*>(xbt_malloc(maxsize));
      memcpy(sendtmpbuf, recvbuf, maxsize);
      sendtmpcounts = static_cast<int*>(xbt_malloc(size * sizeof(int)));
      memcpy(sendtmpcounts, recvcounts, size * sizeof(int));
      sendtmpdisps = static_cast<int*>(xbt_malloc(size * sizeof(int)));
      memcpy(sendtmpdisps, recvdisps, size * sizeof(int));
      sendtmptypes = static_cast<MPI_Datatype*>(xbt_malloc(size * sizeof(MPI_Datatype)));
      memcpy(sendtmptypes, recvtypes, size * sizeof(MPI_Datatype));
    }

    for (int i = 0; i < size; i++) { // copy data to avoid bad free
      send_size += sendtmpcounts[i] * sendtmptypes[i]->size();
      trace_sendcounts->push_back(sendtmpcounts[i] * sendtmptypes[i]->size());
    }

    TRACE_smpi_comm_in(rank, request==MPI_REQUEST_IGNORED?"PMPI_Alltoallw":"PMPI_Ialltoallw",
                       new simgrid::instr::VarCollTIData(request==MPI_REQUEST_IGNORED ? "alltoallv":"ialltoallv", -1, send_size, trace_sendcounts, recv_size,
                                                         trace_recvcounts, simgrid::smpi::Datatype::encode(sendtmptypes[0]),
                                                         simgrid::smpi::Datatype::encode(recvtypes[0])));

    if(request == MPI_REQUEST_IGNORED)
      retval = simgrid::smpi::Colls::alltoallw(sendtmpbuf, sendtmpcounts, sendtmpdisps, sendtmptypes, recvbuf, recvcounts,
                                    recvdisps, recvtypes, comm);
    else
      retval = simgrid::smpi::Colls::ialltoallw(sendtmpbuf, sendtmpcounts, sendtmpdisps, sendtmptypes, recvbuf, recvcounts,
                                    recvdisps, recvtypes, comm, request);
    TRACE_smpi_comm_out(rank);

    if (sendbuf == MPI_IN_PLACE) {
      xbt_free(sendtmpbuf);
      xbt_free(sendtmpcounts);
      xbt_free(sendtmpdisps);
      xbt_free(sendtmptypes);
    }
    smpi_bench_begin();
  }
  return retval;
}

/* Copyright (c) 2007-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "smpi_comm.hpp"
#include "smpi_datatype.hpp"
#include "smpi_process.hpp"
#include "smpi_request.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smpi_pmpi);

static int getPid(MPI_Comm, int);
static int getPid(MPI_Comm comm, int id)
{
  simgrid::s4u::ActorPtr actor = comm->group()->actor(id);
  return (actor == nullptr) ? MPI_UNDEFINED : actor->getPid();
}

/* PMPI User level calls */
extern "C" { // Obviously, the C MPI interface should use the C linkage

int PMPI_Send_init(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if (dst == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else {
    *request = simgrid::smpi::Request::send_init(buf, count, datatype, dst, tag, comm);
    retval   = MPI_SUCCESS;
  }
  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request != nullptr)
    *request = MPI_REQUEST_NULL;
  return retval;
}

int PMPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if (src == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else {
    *request = simgrid::smpi::Request::recv_init(buf, count, datatype, src, tag, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request != nullptr)
    *request = MPI_REQUEST_NULL;
  return retval;
}

int PMPI_Ssend_init(void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request* request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if (dst == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else {
    *request = simgrid::smpi::Request::ssend_init(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request != nullptr)
    *request = MPI_REQUEST_NULL;
  return retval;
}

int PMPI_Start(MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == nullptr || *request == MPI_REQUEST_NULL) {
    retval = MPI_ERR_REQUEST;
  } else {
    (*request)->start();
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Startall(int count, MPI_Request * requests)
{
  int retval;
  smpi_bench_end();
  if (requests == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    retval = MPI_SUCCESS;
    for (int i = 0; i < count; i++) {
      if(requests[i] == MPI_REQUEST_NULL) {
        retval = MPI_ERR_REQUEST;
      }
    }
    if(retval != MPI_ERR_REQUEST) {
      simgrid::smpi::Request::startall(count, requests);
    }
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Request_free(MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();
  if (*request == MPI_REQUEST_NULL) {
    retval = MPI_ERR_ARG;
  } else {
    simgrid::smpi::Request::unref(request);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Irecv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();

  if (request == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (src == MPI_PROC_NULL) {
    *request = MPI_REQUEST_NULL;
    retval = MPI_SUCCESS;
  } else if (src!=MPI_ANY_SOURCE && (src >= comm->group()->size() || src <0)){
    retval = MPI_ERR_RANK;
  } else if ((count < 0) || (buf==nullptr && count > 0)) {
    retval = MPI_ERR_COUNT;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if(tag<0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {

    int my_proc_id = simgrid::s4u::Actor::self()->getPid();

    TRACE_smpi_comm_in(my_proc_id, __FUNCTION__,
                       new simgrid::instr::Pt2PtTIData("Irecv", src,
                                                       datatype->is_replayable() ? count : count * datatype->size(),
                                                       encode_datatype(datatype)));

    *request = simgrid::smpi::Request::irecv(buf, count, datatype, src, tag, comm);
    retval = MPI_SUCCESS;

    TRACE_smpi_comm_out(my_proc_id);
  }

  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request != nullptr)
    *request = MPI_REQUEST_NULL;
  return retval;
}


int PMPI_Isend(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (dst == MPI_PROC_NULL) {
    *request = MPI_REQUEST_NULL;
    retval = MPI_SUCCESS;
  } else if (dst >= comm->group()->size() || dst <0){
    retval = MPI_ERR_RANK;
  } else if ((count < 0) || (buf==nullptr && count > 0)) {
    retval = MPI_ERR_COUNT;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if(tag<0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {
    int my_proc_id = simgrid::s4u::Actor::self()->getPid();
    int trace_dst = getPid(comm, dst);
    TRACE_smpi_comm_in(my_proc_id, __FUNCTION__,
                       new simgrid::instr::Pt2PtTIData("Isend", dst,
                                                       datatype->is_replayable() ? count : count * datatype->size(),
                                                       encode_datatype(datatype)));

    TRACE_smpi_send(my_proc_id, my_proc_id, trace_dst, tag, count * datatype->size());

    *request = simgrid::smpi::Request::isend(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;

    TRACE_smpi_comm_out(my_proc_id);
  }

  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request!=nullptr)
    *request = MPI_REQUEST_NULL;
  return retval;
}

int PMPI_Issend(void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request* request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (dst == MPI_PROC_NULL) {
    *request = MPI_REQUEST_NULL;
    retval = MPI_SUCCESS;
  } else if (dst >= comm->group()->size() || dst <0){
    retval = MPI_ERR_RANK;
  } else if ((count < 0)|| (buf==nullptr && count > 0)) {
    retval = MPI_ERR_COUNT;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if(tag<0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {
    int my_proc_id = simgrid::s4u::Actor::self()->getPid();
    int trace_dst = getPid(comm, dst);
    TRACE_smpi_comm_in(my_proc_id, __FUNCTION__,
                       new simgrid::instr::Pt2PtTIData("ISsend", dst,
                                                       datatype->is_replayable() ? count : count * datatype->size(),
                                                       encode_datatype(datatype)));
    TRACE_smpi_send(my_proc_id, my_proc_id, trace_dst, tag, count * datatype->size());

    *request = simgrid::smpi::Request::issend(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;

    TRACE_smpi_comm_out(my_proc_id);
  }

  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request!=nullptr)
    *request = MPI_REQUEST_NULL;
  return retval;
}

int PMPI_Recv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status * status)
{
  int retval = 0;

  smpi_bench_end();
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (src == MPI_PROC_NULL) {
    simgrid::smpi::Status::empty(status);
    status->MPI_SOURCE = MPI_PROC_NULL;
    retval = MPI_SUCCESS;
  } else if (src!=MPI_ANY_SOURCE && (src >= comm->group()->size() || src <0)){
    retval = MPI_ERR_RANK;
  } else if ((count < 0) || (buf==nullptr && count > 0)) {
    retval = MPI_ERR_COUNT;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if(tag<0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {
    int my_proc_id         = simgrid::s4u::Actor::self()->getPid();
    TRACE_smpi_comm_in(my_proc_id, __FUNCTION__,
                       new simgrid::instr::Pt2PtTIData("recv", src,
                                                       datatype->is_replayable() ? count : count * datatype->size(),
                                                       encode_datatype(datatype)));

    simgrid::smpi::Request::recv(buf, count, datatype, src, tag, comm, status);
    retval = MPI_SUCCESS;

    // the src may not have been known at the beginning of the recv (MPI_ANY_SOURCE)
    if (status != MPI_STATUS_IGNORE) {
      int src_traced = getPid(comm, status->MPI_SOURCE);
      if (not TRACE_smpi_view_internals()) {
        TRACE_smpi_recv(src_traced, my_proc_id, tag);
      }
    }
    TRACE_smpi_comm_out(my_proc_id);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Send(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (dst == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else if (dst >= comm->group()->size() || dst <0){
    retval = MPI_ERR_RANK;
  } else if ((count < 0) || (buf == nullptr && count > 0)) {
    retval = MPI_ERR_COUNT;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if(tag < 0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {
    int my_proc_id         = simgrid::s4u::Actor::self()->getPid();
    int dst_traced         = getPid(comm, dst);
    TRACE_smpi_comm_in(my_proc_id, __FUNCTION__,
                       new simgrid::instr::Pt2PtTIData("send", dst,
                                                       datatype->is_replayable() ? count : count * datatype->size(),
                                                       encode_datatype(datatype)));
    if (not TRACE_smpi_view_internals()) {
      TRACE_smpi_send(my_proc_id, my_proc_id, dst_traced, tag, count * datatype->size());
    }

    simgrid::smpi::Request::send(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;

    TRACE_smpi_comm_out(my_proc_id);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Ssend(void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm) {
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (dst == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else if (dst >= comm->group()->size() || dst <0){
    retval = MPI_ERR_RANK;
  } else if ((count < 0) || (buf==nullptr && count > 0)) {
    retval = MPI_ERR_COUNT;
  } else if (not datatype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if(tag<0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {
    int my_proc_id         = simgrid::s4u::Actor::self()->getPid();
    int dst_traced         = getPid(comm, dst);
    TRACE_smpi_comm_in(my_proc_id, __FUNCTION__,
                       new simgrid::instr::Pt2PtTIData("Ssend", dst,
                                                       datatype->is_replayable() ? count : count * datatype->size(),
                                                       encode_datatype(datatype)));
    TRACE_smpi_send(my_proc_id, my_proc_id, dst_traced, tag, count * datatype->size());

    simgrid::smpi::Request::ssend(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;

    TRACE_smpi_comm_out(my_proc_id);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Sendrecv(void* sendbuf, int sendcount, MPI_Datatype sendtype, int dst, int sendtag, void* recvbuf,
                  int recvcount, MPI_Datatype recvtype, int src, int recvtag, MPI_Comm comm, MPI_Status* status)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (not sendtype->is_valid() || not recvtype->is_valid()) {
    retval = MPI_ERR_TYPE;
  } else if (src == MPI_PROC_NULL) {
    if(status!=MPI_STATUS_IGNORE){
      simgrid::smpi::Status::empty(status);
      status->MPI_SOURCE = MPI_PROC_NULL;
    }
    if(dst != MPI_PROC_NULL)
      simgrid::smpi::Request::send(sendbuf, sendcount, sendtype, dst, sendtag, comm);
    retval = MPI_SUCCESS;
  }else if (dst == MPI_PROC_NULL){
    simgrid::smpi::Request::recv(recvbuf, recvcount, recvtype, src, recvtag, comm, status);
    retval = MPI_SUCCESS;
  }else if (dst >= comm->group()->size() || dst <0 ||
      (src!=MPI_ANY_SOURCE && (src >= comm->group()->size() || src <0))){
    retval = MPI_ERR_RANK;
  } else if ((sendcount < 0 || recvcount<0) ||
      (sendbuf==nullptr && sendcount > 0) || (recvbuf==nullptr && recvcount>0)) {
    retval = MPI_ERR_COUNT;
  } else if((sendtag<0 && sendtag !=  MPI_ANY_TAG)||(recvtag<0 && recvtag != MPI_ANY_TAG)){
    retval = MPI_ERR_TAG;
  } else {
    int my_proc_id         = simgrid::s4u::Actor::self()->getPid();
    int dst_traced         = getPid(comm, dst);
    int src_traced         = getPid(comm, src);

    // FIXME: Hack the way to trace this one
    std::vector<int>* dst_hack = new std::vector<int>;
    std::vector<int>* src_hack = new std::vector<int>;
    dst_hack->push_back(dst_traced);
    src_hack->push_back(src_traced);
    TRACE_smpi_comm_in(my_proc_id, __FUNCTION__,
                       new simgrid::instr::VarCollTIData(
                           "sendRecv", -1, sendtype->is_replayable() ? sendcount : sendcount * sendtype->size(),
                           dst_hack, recvtype->is_replayable() ? recvcount : recvcount * recvtype->size(), src_hack,
                           encode_datatype(sendtype), encode_datatype(recvtype)));

    TRACE_smpi_send(my_proc_id, my_proc_id, dst_traced, sendtag, sendcount * sendtype->size());

    simgrid::smpi::Request::sendrecv(sendbuf, sendcount, sendtype, dst, sendtag, recvbuf, recvcount, recvtype, src,
                                     recvtag, comm, status);
    retval = MPI_SUCCESS;

    TRACE_smpi_recv(src_traced, my_proc_id, recvtag);
    TRACE_smpi_comm_out(my_proc_id);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Sendrecv_replace(void* buf, int count, MPI_Datatype datatype, int dst, int sendtag, int src, int recvtag,
                          MPI_Comm comm, MPI_Status* status)
{
  int retval = 0;
  if (not datatype->is_valid()) {
    return MPI_ERR_TYPE;
  } else if (count < 0) {
    return MPI_ERR_COUNT;
  } else {
    int size = datatype->get_extent() * count;
    void* recvbuf = xbt_new0(char, size);
    retval = MPI_Sendrecv(buf, count, datatype, dst, sendtag, recvbuf, count, datatype, src, recvtag, comm, status);
    if(retval==MPI_SUCCESS){
      simgrid::smpi::Datatype::copy(recvbuf, count, datatype, buf, count, datatype);
    }
    xbt_free(recvbuf);

  }
  return retval;
}

int PMPI_Test(MPI_Request * request, int *flag, MPI_Status * status)
{
  int retval = 0;
  smpi_bench_end();
  if (request == nullptr || flag == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (*request == MPI_REQUEST_NULL) {
    *flag= true;
    simgrid::smpi::Status::empty(status);
    retval = MPI_SUCCESS;
  } else {
    int my_proc_id = ((*request)->comm() != MPI_COMM_NULL) ? simgrid::s4u::Actor::self()->getPid() : -1;

    TRACE_smpi_testing_in(my_proc_id);

    *flag = simgrid::smpi::Request::test(request,status);

    TRACE_smpi_testing_out(my_proc_id);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Testany(int count, MPI_Request requests[], int *index, int *flag, MPI_Status * status)
{
  int retval = 0;

  smpi_bench_end();
  if (index == nullptr || flag == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    *flag = simgrid::smpi::Request::testany(count, requests, index, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Testall(int count, MPI_Request* requests, int* flag, MPI_Status* statuses)
{
  int retval = 0;

  smpi_bench_end();
  if (flag == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    *flag = simgrid::smpi::Request::testall(count, requests, statuses);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status* status) {
  int retval = 0;
  smpi_bench_end();

  if (status == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (source == MPI_PROC_NULL) {
    simgrid::smpi::Status::empty(status);
    status->MPI_SOURCE = MPI_PROC_NULL;
    retval = MPI_SUCCESS;
  } else {
    simgrid::smpi::Request::probe(source, tag, comm, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Iprobe(int source, int tag, MPI_Comm comm, int* flag, MPI_Status* status) {
  int retval = 0;
  smpi_bench_end();

  if (flag == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (source == MPI_PROC_NULL) {
    *flag=true;
    simgrid::smpi::Status::empty(status);
    status->MPI_SOURCE = MPI_PROC_NULL;
    retval = MPI_SUCCESS;
  } else {
    simgrid::smpi::Request::iprobe(source, tag, comm, flag, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

// TODO: cheinrich: Move declaration to other file? Rename this function - it's used for PMPI_Wait*?
static void trace_smpi_recv_helper(MPI_Request* request, MPI_Status* status);
static void trace_smpi_recv_helper(MPI_Request* request, MPI_Status* status)
{
  MPI_Request req = *request;
  if (req != MPI_REQUEST_NULL) { // Received requests become null
    int src_traced = req->src();
    // the src may not have been known at the beginning of the recv (MPI_ANY_SOURCE)
    int dst_traced = req->dst();
    if (req->flags() & RECV) { // Is this request a wait for RECV?
      if (src_traced == MPI_ANY_SOURCE)
        src_traced = (status != MPI_STATUSES_IGNORE) ? req->comm()->group()->rank(status->MPI_SOURCE) : req->src();
      TRACE_smpi_recv(src_traced, dst_traced, req->tag());
    }
  }
}

int PMPI_Wait(MPI_Request * request, MPI_Status * status)
{
  int retval = 0;

  smpi_bench_end();

  simgrid::smpi::Status::empty(status);

  if (request == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (*request == MPI_REQUEST_NULL) {
    retval = MPI_SUCCESS;
  } else {
    int my_proc_id = (*request)->comm() != MPI_COMM_NULL
                         ? simgrid::s4u::Actor::self()->getPid()
                         : -1; // TODO: cheinrich: Check if this correct or if it should be MPI_UNDEFINED

    TRACE_smpi_comm_in(my_proc_id, __FUNCTION__, new simgrid::instr::NoOpTIData("wait"));

    simgrid::smpi::Request::wait(request, status);
    retval = MPI_SUCCESS;

    //the src may not have been known at the beginning of the recv (MPI_ANY_SOURCE)
    TRACE_smpi_comm_out(my_proc_id);
    trace_smpi_recv_helper(request, status);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Waitany(int count, MPI_Request requests[], int *index, MPI_Status * status)
{
  if (index == nullptr)
    return MPI_ERR_ARG;

  if (count <= 0)
    return MPI_SUCCESS;

  smpi_bench_end();

  int rank_traced = simgrid::s4u::Actor::self()->getPid(); // FIXME: In PMPI_Wait, we check if the comm is null?
  TRACE_smpi_comm_in(rank_traced, __FUNCTION__, new simgrid::instr::CpuTIData("waitAny", static_cast<double>(count)));

  *index = simgrid::smpi::Request::waitany(count, requests, status);

  if(*index!=MPI_UNDEFINED){
    trace_smpi_recv_helper(&requests[*index], status);
    TRACE_smpi_comm_out(rank_traced);
  }

  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Waitall(int count, MPI_Request requests[], MPI_Status status[])
{
  smpi_bench_end();

  int rank_traced = simgrid::s4u::Actor::self()->getPid(); // FIXME: In PMPI_Wait, we check if the comm is null?
  TRACE_smpi_comm_in(rank_traced, __FUNCTION__, new simgrid::instr::CpuTIData("waitAll", static_cast<double>(count)));

  int retval = simgrid::smpi::Request::waitall(count, requests, status);

  for (int i = 0; i < count; i++) {
    trace_smpi_recv_helper(&requests[i], &status[i]);
  }
  TRACE_smpi_comm_out(rank_traced);

  smpi_bench_begin();
  return retval;
}

int PMPI_Waitsome(int incount, MPI_Request requests[], int *outcount, int *indices, MPI_Status status[])
{
  int retval = 0;

  smpi_bench_end();
  if (outcount == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    *outcount = simgrid::smpi::Request::waitsome(incount, requests, indices, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Testsome(int incount, MPI_Request requests[], int* outcount, int* indices, MPI_Status status[])
{
  int retval = 0;

  smpi_bench_end();
  if (outcount == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    *outcount = simgrid::smpi::Request::testsome(incount, requests, indices, status);
    retval    = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

MPI_Request PMPI_Request_f2c(MPI_Fint request){
  return static_cast<MPI_Request>(simgrid::smpi::Request::f2c(request));
}

MPI_Fint PMPI_Request_c2f(MPI_Request request) {
  return request->c2f();
}

}

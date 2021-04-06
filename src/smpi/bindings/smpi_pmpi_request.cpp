/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "smpi_comm.hpp"
#include "smpi_datatype.hpp"
#include "smpi_request.hpp"
#include "src/smpi/include/smpi_actor.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smpi_pmpi);

static int getPid(MPI_Comm, int);
static int getPid(MPI_Comm comm, int id)
{
  simgrid::s4u::ActorPtr actor = comm->group()->actor(id);
  return (actor == nullptr) ? MPI_UNDEFINED : actor->get_pid();
}

#define CHECK_SEND_INPUTS\
  CHECK_BUFFER(1, buf, count)\
  CHECK_COUNT(2, count)\
  CHECK_TYPE(3, datatype)\
  CHECK_COMM(6)\
  if(dst!= MPI_PROC_NULL)\
    CHECK_RANK(4, dst, comm)\
  CHECK_TAG(5, tag)

#define CHECK_ISEND_INPUTS\
  CHECK_REQUEST(7)\
  *request = MPI_REQUEST_NULL;\
  CHECK_SEND_INPUTS
  
#define CHECK_IRECV_INPUTS\
  CHECK_REQUEST(7)\
  *request = MPI_REQUEST_NULL;\
  CHECK_BUFFER(1, buf, count)\
  CHECK_COUNT(2, count)\
  CHECK_TYPE(3, datatype)\
  CHECK_COMM(6)\
  if(src!=MPI_ANY_SOURCE && src!=MPI_PROC_NULL)\
    CHECK_RANK(4, src, comm)\
  CHECK_TAG(5, tag)
/* PMPI User level calls */

int PMPI_Send_init(const void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request * request)
{
  CHECK_ISEND_INPUTS

  smpi_bench_end();
  *request = simgrid::smpi::Request::send_init(buf, count, datatype, dst, tag, comm);
  smpi_bench_begin();

  return MPI_SUCCESS;
}

int PMPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Request * request)
{
  CHECK_IRECV_INPUTS

  smpi_bench_end();
  *request = simgrid::smpi::Request::recv_init(buf, count, datatype, src, tag, comm);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Rsend_init(const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm,
                    MPI_Request* request)
{
  return PMPI_Send_init(buf, count, datatype, dst, tag, comm, request);
}

int PMPI_Ssend_init(const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request* request)
{
  CHECK_ISEND_INPUTS

  int retval = 0;
  smpi_bench_end();
  *request = simgrid::smpi::Request::ssend_init(buf, count, datatype, dst, tag, comm);
  retval = MPI_SUCCESS;

  smpi_bench_begin();
  return retval;
}

/*
 * This function starts a request returned by init functions such as
 * MPI_Send_init(), MPI_Ssend_init (see above), and friends.
 * They should already have performed sanity checks.
 */
int PMPI_Start(MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();
  CHECK_REQUEST_VALID(1)
  if ( *request == MPI_REQUEST_NULL) {
    retval = MPI_ERR_REQUEST;
  } else {
    MPI_Request req = *request;
    int my_proc_id = (req->comm() != MPI_COMM_NULL) ? simgrid::s4u::this_actor::get_pid() : -1;
    TRACE_smpi_comm_in(my_proc_id, __func__,
                       new simgrid::instr::Pt2PtTIData("Start", req->dst(),
                                                       req->size(),
                                                       req->tag(), 
                                                       simgrid::smpi::Datatype::encode(req->type())));
    if (not TRACE_smpi_view_internals() && req->flags() & MPI_REQ_SEND)
      TRACE_smpi_send(my_proc_id, my_proc_id, getPid(req->comm(), req->dst()), req->tag(), req->size());

    req->start();

    if (not TRACE_smpi_view_internals() && req->flags() & MPI_REQ_RECV)
      TRACE_smpi_recv(getPid(req->comm(), req->src()), my_proc_id, req->tag());
    retval = MPI_SUCCESS;
    TRACE_smpi_comm_out(my_proc_id);
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
      int my_proc_id = simgrid::s4u::this_actor::get_pid();
      TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("Startall"));
      if (not TRACE_smpi_view_internals())
        for (int i = 0; i < count; i++) {
          const simgrid::smpi::Request* req = requests[i];
          if (req->flags() & MPI_REQ_SEND)
            TRACE_smpi_send(my_proc_id, my_proc_id, getPid(req->comm(), req->dst()), req->tag(), req->size());
        }

      simgrid::smpi::Request::startall(count, requests);

      if (not TRACE_smpi_view_internals())
        for (int i = 0; i < count; i++) {
          const simgrid::smpi::Request* req = requests[i];
          if (req->flags() & MPI_REQ_RECV)
            TRACE_smpi_recv(getPid(req->comm(), req->src()), my_proc_id, req->tag());
        }
      TRACE_smpi_comm_out(my_proc_id);
    }
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Request_free(MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();
  if (*request != MPI_REQUEST_NULL) {
    (*request)->mark_as_deleted();
    simgrid::smpi::Request::unref(request);
    *request = MPI_REQUEST_NULL;
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Irecv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Request * request)
{
  CHECK_IRECV_INPUTS

  smpi_bench_end();
  int my_proc_id = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_comm_in(my_proc_id, __func__,
                     new simgrid::instr::Pt2PtTIData("irecv", src,
                                                     datatype->is_replayable() ? count : count * datatype->size(),
                                                     tag, simgrid::smpi::Datatype::encode(datatype)));
  *request = simgrid::smpi::Request::irecv(buf, count, datatype, src, tag, comm);
  TRACE_smpi_comm_out(my_proc_id);
  smpi_bench_begin();
  return MPI_SUCCESS;
}


int PMPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request * request)
{
  CHECK_ISEND_INPUTS

  smpi_bench_end();
  int retval = 0;
  int my_proc_id = simgrid::s4u::this_actor::get_pid();
  int trace_dst = getPid(comm, dst);
  TRACE_smpi_comm_in(my_proc_id, __func__,
                     new simgrid::instr::Pt2PtTIData("isend", dst,
                                                     datatype->is_replayable() ? count : count * datatype->size(),
                                                     tag, simgrid::smpi::Datatype::encode(datatype)));
  TRACE_smpi_send(my_proc_id, my_proc_id, trace_dst, tag, count * datatype->size());
  *request = simgrid::smpi::Request::isend(buf, count, datatype, dst, tag, comm);
  retval = MPI_SUCCESS;
  TRACE_smpi_comm_out(my_proc_id);
  smpi_bench_begin();

  return retval;
}

int PMPI_Irsend(const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm,
                MPI_Request* request)
{
  return PMPI_Isend(buf, count, datatype, dst, tag, comm, request);
}

int PMPI_Issend(const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request* request)
{
  CHECK_ISEND_INPUTS

  smpi_bench_end();
  int my_proc_id = simgrid::s4u::this_actor::get_pid();
  int trace_dst = getPid(comm, dst);
  TRACE_smpi_comm_in(my_proc_id, __func__,
                     new simgrid::instr::Pt2PtTIData("ISsend", dst,
                                                     datatype->is_replayable() ? count : count * datatype->size(),
                                                     tag, simgrid::smpi::Datatype::encode(datatype)));
  TRACE_smpi_send(my_proc_id, my_proc_id, trace_dst, tag, count * datatype->size());
  *request = simgrid::smpi::Request::issend(buf, count, datatype, dst, tag, comm);
  TRACE_smpi_comm_out(my_proc_id);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Recv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status * status)
{
  int retval = 0;

  CHECK_BUFFER(1, buf, count)
  CHECK_COUNT(2, count)
  CHECK_TYPE(3, datatype)
  CHECK_TAG(5, tag)
  CHECK_COMM(6)

  smpi_bench_end();
  if (src == MPI_PROC_NULL) {
    if(status != MPI_STATUS_IGNORE){
      simgrid::smpi::Status::empty(status);
      status->MPI_SOURCE = MPI_PROC_NULL;
    }
    retval = MPI_SUCCESS;
  } else if (src!=MPI_ANY_SOURCE && (src >= comm->group()->size() || src <0)){
    retval = MPI_ERR_RANK;
  } else {
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(my_proc_id, __func__,
                       new simgrid::instr::Pt2PtTIData("recv", src,
                                                       datatype->is_replayable() ? count : count * datatype->size(),
                                                       tag, simgrid::smpi::Datatype::encode(datatype)));

    retval = simgrid::smpi::Request::recv(buf, count, datatype, src, tag, comm, status);

    // the src may not have been known at the beginning of the recv (MPI_ANY_SOURCE)
    int src_traced=0;
    if (status != MPI_STATUS_IGNORE) 
      src_traced = getPid(comm, status->MPI_SOURCE);
    else
      src_traced = getPid(comm, src);
    if (not TRACE_smpi_view_internals()) {
      TRACE_smpi_recv(src_traced, my_proc_id, tag);
    }
    
    TRACE_smpi_comm_out(my_proc_id);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Send(const void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
  CHECK_SEND_INPUTS

  smpi_bench_end();
  int my_proc_id         = simgrid::s4u::this_actor::get_pid();
  int dst_traced         = getPid(comm, dst);
  TRACE_smpi_comm_in(my_proc_id, __func__,
                     new simgrid::instr::Pt2PtTIData("send", dst,
                                                     datatype->is_replayable() ? count : count * datatype->size(),
                                                     tag, simgrid::smpi::Datatype::encode(datatype)));
  if (not TRACE_smpi_view_internals()) {
    TRACE_smpi_send(my_proc_id, my_proc_id, dst_traced, tag, count * datatype->size());
  }
  simgrid::smpi::Request::send(buf, count, datatype, dst, tag, comm);
  TRACE_smpi_comm_out(my_proc_id);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Rsend(const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
  return PMPI_Send(buf, count, datatype, dst, tag, comm);
}

int PMPI_Bsend(const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
  CHECK_SEND_INPUTS

  smpi_bench_end();
  int my_proc_id         = simgrid::s4u::this_actor::get_pid();
  int dst_traced         = getPid(comm, dst);
  int bsend_buf_size = 0;
  void* bsend_buf = nullptr;
  smpi_process()->bsend_buffer(&bsend_buf, &bsend_buf_size);
  int size = datatype->get_extent() * count;
  if(bsend_buf==nullptr || bsend_buf_size < size + MPI_BSEND_OVERHEAD )
    return MPI_ERR_BUFFER;
  TRACE_smpi_comm_in(my_proc_id, __func__,
                     new simgrid::instr::Pt2PtTIData("bsend", dst,
                                                     datatype->is_replayable() ? count : count * datatype->size(),
                                                     tag, simgrid::smpi::Datatype::encode(datatype)));
  if (not TRACE_smpi_view_internals()) {
    TRACE_smpi_send(my_proc_id, my_proc_id, dst_traced, tag, count * datatype->size());
  }
  simgrid::smpi::Request::bsend(buf, count, datatype, dst, tag, comm);
  TRACE_smpi_comm_out(my_proc_id);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Ibsend(const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request* request)
{
  CHECK_ISEND_INPUTS

  smpi_bench_end();
  int my_proc_id = simgrid::s4u::this_actor::get_pid();
  int trace_dst = getPid(comm, dst);
  int bsend_buf_size = 0;
  void* bsend_buf = nullptr;
  smpi_process()->bsend_buffer(&bsend_buf, &bsend_buf_size);
  int size = datatype->get_extent() * count;
  if(bsend_buf==nullptr || bsend_buf_size < size + MPI_BSEND_OVERHEAD )
    return MPI_ERR_BUFFER;
  TRACE_smpi_comm_in(my_proc_id, __func__,
                     new simgrid::instr::Pt2PtTIData("ibsend", dst,
                                                     datatype->is_replayable() ? count : count * datatype->size(),
                                                     tag, simgrid::smpi::Datatype::encode(datatype)));
  TRACE_smpi_send(my_proc_id, my_proc_id, trace_dst, tag, count * datatype->size());
  *request = simgrid::smpi::Request::ibsend(buf, count, datatype, dst, tag, comm);
  TRACE_smpi_comm_out(my_proc_id);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Bsend_init(const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request* request)
{
  CHECK_ISEND_INPUTS

  smpi_bench_end();
  int retval = 0;
  int bsend_buf_size = 0;
  void* bsend_buf = nullptr;
  smpi_process()->bsend_buffer(&bsend_buf, &bsend_buf_size);
  if( bsend_buf==nullptr || bsend_buf_size < datatype->get_extent() * count + MPI_BSEND_OVERHEAD ) {
    retval = MPI_ERR_BUFFER;
  } else {
    *request = simgrid::smpi::Request::bsend_init(buf, count, datatype, dst, tag, comm);
    retval   = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Ssend(const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
  CHECK_SEND_INPUTS

  smpi_bench_end();
  int my_proc_id         = simgrid::s4u::this_actor::get_pid();
  int dst_traced         = getPid(comm, dst);
  TRACE_smpi_comm_in(my_proc_id, __func__,
                     new simgrid::instr::Pt2PtTIData("Ssend", dst,
                                                     datatype->is_replayable() ? count : count * datatype->size(),
                                                     tag, simgrid::smpi::Datatype::encode(datatype)));
  TRACE_smpi_send(my_proc_id, my_proc_id, dst_traced, tag, count * datatype->size());
  simgrid::smpi::Request::ssend(buf, count, datatype, dst, tag, comm);
  TRACE_smpi_comm_out(my_proc_id);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Sendrecv(const void* sendbuf, int sendcount, MPI_Datatype sendtype, int dst, int sendtag, void* recvbuf,
                  int recvcount, MPI_Datatype recvtype, int src, int recvtag, MPI_Comm comm, MPI_Status* status)
{
  int retval = 0;
  CHECK_BUFFER(1, sendbuf, sendcount)
  CHECK_COUNT(2, sendcount)
  CHECK_TYPE(3, sendtype)
  CHECK_TAG(5, sendtag)
  CHECK_BUFFER(6, recvbuf, recvcount)
  CHECK_COUNT(7, recvcount)
  CHECK_TYPE(8, recvtype)
  CHECK_TAG(10, recvtag)
  CHECK_COMM(11)
  smpi_bench_end();

  if (src == MPI_PROC_NULL) {
    if(status!=MPI_STATUS_IGNORE){
      simgrid::smpi::Status::empty(status);
      status->MPI_SOURCE = MPI_PROC_NULL;
    }
    if(dst != MPI_PROC_NULL)
      simgrid::smpi::Request::send(sendbuf, sendcount, sendtype, dst, sendtag, comm);
    retval = MPI_SUCCESS;
  } else if (dst == MPI_PROC_NULL){
    retval = simgrid::smpi::Request::recv(recvbuf, recvcount, recvtype, src, recvtag, comm, status);
  } else if (dst >= comm->group()->size() || dst <0 ||
      (src!=MPI_ANY_SOURCE && (src >= comm->group()->size() || src <0))){
    retval = MPI_ERR_RANK;
  } else {
    int my_proc_id         = simgrid::s4u::this_actor::get_pid();
    int dst_traced         = getPid(comm, dst);
    int src_traced         = getPid(comm, src);

    // FIXME: Hack the way to trace this one
    auto dst_hack = std::make_shared<std::vector<int>>();
    auto src_hack = std::make_shared<std::vector<int>>();
    dst_hack->push_back(dst_traced);
    src_hack->push_back(src_traced);
    TRACE_smpi_comm_in(my_proc_id, __func__,
                       new simgrid::instr::VarCollTIData(
                           "sendRecv", -1, sendtype->is_replayable() ? sendcount : sendcount * sendtype->size(),
                           dst_hack, recvtype->is_replayable() ? recvcount : recvcount * recvtype->size(), src_hack,
                           simgrid::smpi::Datatype::encode(sendtype), simgrid::smpi::Datatype::encode(recvtype)));

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
  CHECK_BUFFER(1, buf, count)
  CHECK_COUNT(2, count)
  CHECK_TYPE(3, datatype)

  int size = datatype->get_extent() * count;
  xbt_assert(size > 0);
  std::vector<char> recvbuf(size);
  retval =
      MPI_Sendrecv(buf, count, datatype, dst, sendtag, recvbuf.data(), count, datatype, src, recvtag, comm, status);
  if(retval==MPI_SUCCESS){
    simgrid::smpi::Datatype::copy(recvbuf.data(), count, datatype, buf, count, datatype);
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
    if (status != MPI_STATUS_IGNORE){
      *flag= true;
      simgrid::smpi::Status::empty(status);
    }
    retval = MPI_SUCCESS;
  } else {
    int my_proc_id = ((*request)->comm() != MPI_COMM_NULL) ? simgrid::s4u::this_actor::get_pid() : -1;

    TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("test"));
    retval = simgrid::smpi::Request::test(request,status, flag);

    TRACE_smpi_comm_out(my_proc_id);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Testany(int count, MPI_Request requests[], int *index, int *flag, MPI_Status * status)
{
  int retval = 0;
  CHECK_COUNT(1, count)
  smpi_bench_end();
  if (index == nullptr || flag == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("testany"));
    retval = simgrid::smpi::Request::testany(count, requests, index, flag, status);
    TRACE_smpi_comm_out(my_proc_id);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Testall(int count, MPI_Request* requests, int* flag, MPI_Status* statuses)
{
  int retval = 0;
  CHECK_COUNT(1, count)
  smpi_bench_end();
  if (flag == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("testall"));
    retval = simgrid::smpi::Request::testall(count, requests, flag, statuses);
    TRACE_smpi_comm_out(my_proc_id);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Testsome(int incount, MPI_Request requests[], int* outcount, int* indices, MPI_Status status[])
{
  int retval = 0;
  CHECK_COUNT(1, incount)
  smpi_bench_end();
  if (outcount == nullptr) {
    retval = MPI_ERR_ARG;
  } else {
    int my_proc_id = simgrid::s4u::this_actor::get_pid();
    TRACE_smpi_comm_in(my_proc_id, __func__, new simgrid::instr::NoOpTIData("testsome"));
    retval = simgrid::smpi::Request::testsome(incount, requests, outcount, indices, status);
    TRACE_smpi_comm_out(my_proc_id);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status* status) {
  int retval = 0;
  smpi_bench_end();

  CHECK_COMM(6)
  if(source!=MPI_ANY_SOURCE && source!=MPI_PROC_NULL)\
    CHECK_RANK(1, source, comm)
  CHECK_TAG(2, tag)
  if (source == MPI_PROC_NULL) {
    if (status != MPI_STATUS_IGNORE){
      simgrid::smpi::Status::empty(status);
      status->MPI_SOURCE = MPI_PROC_NULL;
    }
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
  CHECK_COMM(6)
  if(source!=MPI_ANY_SOURCE && source!=MPI_PROC_NULL)\
    CHECK_RANK(1, source, comm)
  CHECK_TAG(2, tag)
  if (flag == nullptr) {
    retval = MPI_ERR_ARG;
  } else if (source == MPI_PROC_NULL) {
    *flag=true;
    if (status != MPI_STATUS_IGNORE){
      simgrid::smpi::Status::empty(status);
      status->MPI_SOURCE = MPI_PROC_NULL;
    }
    retval = MPI_SUCCESS;
  } else {
    simgrid::smpi::Request::iprobe(source, tag, comm, flag, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

// TODO: cheinrich: Move declaration to other file? Rename this function - it's used for PMPI_Wait*?
static void trace_smpi_recv_helper(MPI_Request* request, MPI_Status* status)
{
  const simgrid::smpi::Request* req = *request;
  if (req != MPI_REQUEST_NULL) { // Received requests become null
    int src_traced = req->src();
    // the src may not have been known at the beginning of the recv (MPI_ANY_SOURCE)
    int dst_traced = req->dst();
    if (req->flags() & MPI_REQ_RECV) { // Is this request a wait for RECV?
      if (src_traced == MPI_ANY_SOURCE)
        src_traced = (status != MPI_STATUS_IGNORE) ? req->comm()->group()->rank(status->MPI_SOURCE) : req->src();
      TRACE_smpi_recv(src_traced, dst_traced, req->tag());
    }
  }
}

int PMPI_Wait(MPI_Request * request, MPI_Status * status)
{
  int retval = 0;

  smpi_bench_end();

  simgrid::smpi::Status::empty(status);

  CHECK_NULL(1, MPI_ERR_ARG, request) 
  if (*request == MPI_REQUEST_NULL) {
    retval = MPI_SUCCESS;
  } else {
    // for tracing, save the handle which might get overridden before we can use the helper on it
    MPI_Request savedreq = *request;
    if (savedreq != MPI_REQUEST_NULL && not(savedreq->flags() & MPI_REQ_FINISHED)
    && not(savedreq->flags() & MPI_REQ_GENERALIZED))
      savedreq->ref();//don't erase the handle in Request::wait, we'll need it later
    else
      savedreq = MPI_REQUEST_NULL;

    int my_proc_id = (*request)->comm() != MPI_COMM_NULL
                         ? simgrid::s4u::this_actor::get_pid()
                         : -1; // TODO: cheinrich: Check if this correct or if it should be MPI_UNDEFINED
    TRACE_smpi_comm_in(my_proc_id, __func__,
                       new simgrid::instr::WaitTIData((*request)->src(), (*request)->dst(), (*request)->tag()));

    retval = simgrid::smpi::Request::wait(request, status);

    //the src may not have been known at the beginning of the recv (MPI_ANY_SOURCE)
    TRACE_smpi_comm_out(my_proc_id);
    trace_smpi_recv_helper(&savedreq, status);
    if (savedreq != MPI_REQUEST_NULL)
      simgrid::smpi::Request::unref(&savedreq);
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
  // for tracing, save the handles which might get overridden before we can use the helper on it
  std::vector<MPI_Request> savedreqs(requests, requests + count);
  for (MPI_Request& req : savedreqs) {
    if (req != MPI_REQUEST_NULL && not(req->flags() & MPI_REQ_FINISHED))
      req->ref();
    else
      req = MPI_REQUEST_NULL;
  }

  int rank_traced = simgrid::s4u::this_actor::get_pid(); // FIXME: In PMPI_Wait, we check if the comm is null?
  TRACE_smpi_comm_in(rank_traced, __func__, new simgrid::instr::CpuTIData("waitAny", count));

  *index = simgrid::smpi::Request::waitany(count, requests, status);

  if(*index!=MPI_UNDEFINED){
    trace_smpi_recv_helper(&savedreqs[*index], status);
    TRACE_smpi_comm_out(rank_traced);
  }

  for (MPI_Request& req : savedreqs)
    if (req != MPI_REQUEST_NULL)
      simgrid::smpi::Request::unref(&req);

  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Waitall(int count, MPI_Request requests[], MPI_Status status[])
{
  smpi_bench_end();
  CHECK_COUNT(1, count)
  // for tracing, save the handles which might get overridden before we can use the helper on it
  std::vector<MPI_Request> savedreqs(requests, requests + count);
  for (MPI_Request& req : savedreqs) {
    if (req != MPI_REQUEST_NULL && not(req->flags() & MPI_REQ_FINISHED))
      req->ref();
    else
      req = MPI_REQUEST_NULL;
  }

  int rank_traced = simgrid::s4u::this_actor::get_pid(); // FIXME: In PMPI_Wait, we check if the comm is null?
  TRACE_smpi_comm_in(rank_traced, __func__, new simgrid::instr::CpuTIData("waitall", count));

  int retval = simgrid::smpi::Request::waitall(count, requests, status);

  for (int i = 0; i < count; i++) {
    trace_smpi_recv_helper(&savedreqs[i], status!=MPI_STATUSES_IGNORE ? &status[i]: MPI_STATUS_IGNORE);
  }
  TRACE_smpi_comm_out(rank_traced);

  for (MPI_Request& req : savedreqs)
    if (req != MPI_REQUEST_NULL)
      simgrid::smpi::Request::unref(&req);

  smpi_bench_begin();
  return retval;
}

int PMPI_Waitsome(int incount, MPI_Request requests[], int *outcount, int *indices, MPI_Status status[])
{
  int retval = 0;
  CHECK_COUNT(1, incount)
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

int PMPI_Cancel(MPI_Request* request)
{
  int retval = 0;

  smpi_bench_end();
  CHECK_REQUEST_VALID(1)
  if (*request == MPI_REQUEST_NULL) {
    retval = MPI_ERR_REQUEST;
  } else {
    (*request)->cancel();
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Test_cancelled(const MPI_Status* status, int* flag){
  if(status==MPI_STATUS_IGNORE){
    *flag=0;
    return MPI_ERR_ARG;
  }
  *flag=simgrid::smpi::Status::cancelled(status);
  return MPI_SUCCESS;  
}

int PMPI_Status_set_cancelled(MPI_Status* status, int flag){
  if(status==MPI_STATUS_IGNORE){
    return MPI_ERR_ARG;
  }
  simgrid::smpi::Status::set_cancelled(status,flag);
  return MPI_SUCCESS;  
}

int PMPI_Status_set_elements(MPI_Status* status, MPI_Datatype datatype, int count){
  if(status==MPI_STATUS_IGNORE){
    return MPI_ERR_ARG;
  }
  simgrid::smpi::Status::set_elements(status,datatype, count);
  return MPI_SUCCESS;
}

int PMPI_Status_set_elements_x(MPI_Status* status, MPI_Datatype datatype, MPI_Count count){
  if(status==MPI_STATUS_IGNORE){
    return MPI_ERR_ARG;
  }
  simgrid::smpi::Status::set_elements(status,datatype, static_cast<int>(count));
  return MPI_SUCCESS;
}

int PMPI_Grequest_start( MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function *free_fn, MPI_Grequest_cancel_function *cancel_fn, void *extra_state, MPI_Request *request){
  return simgrid::smpi::Request::grequest_start(query_fn, free_fn,cancel_fn, extra_state, request);
}

int PMPI_Grequest_complete( MPI_Request request){
  return simgrid::smpi::Request::grequest_complete(request);
}

int PMPI_Request_get_status( MPI_Request request, int *flag, MPI_Status *status){
  if(request==MPI_REQUEST_NULL){
    *flag=1;
    simgrid::smpi::Status::empty(status);
    return MPI_SUCCESS;
  } else if (flag == nullptr) {
    return MPI_ERR_ARG;
  }
  return simgrid::smpi::Request::get_status(request,flag,status);
}

MPI_Request PMPI_Request_f2c(MPI_Fint request){
  if(request==-1)
    return MPI_REQUEST_NULL;
  return simgrid::smpi::Request::f2c(request);
}

MPI_Fint PMPI_Request_c2f(MPI_Request request) {
  if(request==MPI_REQUEST_NULL)
    return -1;
  return request->c2f();
}

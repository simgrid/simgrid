/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/virtu.h"
#include "mc/mc.h"
#include "xbt/replay.h"
#include <errno.h>
#include "simix/smx_private.h"
#include "surf/surf.h"
#include "simgrid/sg_config.h"


XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_base, smpi, "Logging specific to SMPI (base)");


static int match_recv(void* a, void* b, smx_action_t ignored) {
   MPI_Request ref = (MPI_Request)a;
   MPI_Request req = (MPI_Request)b;
   XBT_DEBUG("Trying to match a recv of src %d against %d, tag %d against %d",ref->src,req->src, ref->tag, req->tag);

  xbt_assert(ref, "Cannot match recv against null reference");
  xbt_assert(req, "Cannot match recv against null request");
  if((ref->src == MPI_ANY_SOURCE || req->src == ref->src)
    && (ref->tag == MPI_ANY_TAG || req->tag == ref->tag)){
    //we match, we can transfer some values
    // FIXME : move this to the copy function ?
    if(ref->src == MPI_ANY_SOURCE)ref->real_src = req->src;
    if(ref->tag == MPI_ANY_TAG)ref->real_tag = req->tag;
    if(ref->real_size < req->real_size) ref->truncated = 1;
    if(req->detached==1){
        ref->detached_sender=req; //tie the sender to the receiver, as it is detached and has to be freed in the receiver
    }
    return 1;
  }else return 0;
}

static int match_send(void* a, void* b,smx_action_t ignored) {
   MPI_Request ref = (MPI_Request)a;
   MPI_Request req = (MPI_Request)b;
   XBT_DEBUG("Trying to match a send of src %d against %d, tag %d against %d",ref->src,req->src, ref->tag, req->tag);
   xbt_assert(ref, "Cannot match send against null reference");
   xbt_assert(req, "Cannot match send against null request");

   if((req->src == MPI_ANY_SOURCE || req->src == ref->src)
             && (req->tag == MPI_ANY_TAG || req->tag == ref->tag))
   {
     if(req->src == MPI_ANY_SOURCE)req->real_src = ref->src;
     if(req->tag == MPI_ANY_TAG)req->real_tag = ref->tag;
     if(req->real_size < ref->real_size) req->truncated = 1;
     if(ref->detached==1){
         req->detached_sender=ref; //tie the sender to the receiver, as it is detached and has to be freed in the receiver
     }

     return 1;
   } else return 0;
}


typedef struct s_smpi_factor *smpi_factor_t;
typedef struct s_smpi_factor {
  long factor;
  int nb_values;
  double values[4];//arbitrary set to 4
} s_smpi_factor_t;
xbt_dynar_t smpi_os_values = NULL;
xbt_dynar_t smpi_or_values = NULL;
xbt_dynar_t smpi_ois_values = NULL;

// Methods used to parse and store the values for timing injections in smpi
// These are taken from surf/network.c and generalized to have more factors
// These methods should be merged with those in surf/network.c (moved somewhere in xbt ?)

static int factor_cmp(const void *pa, const void *pb)
{
  return (((s_smpi_factor_t*)pa)->factor > ((s_smpi_factor_t*)pb)->factor);
}


static xbt_dynar_t parse_factor(const char *smpi_coef_string)
{
  char *value = NULL;
  unsigned int iter = 0;
  s_smpi_factor_t fact;
  int i=0;
  xbt_dynar_t smpi_factor, radical_elements, radical_elements2 = NULL;

  smpi_factor = xbt_dynar_new(sizeof(s_smpi_factor_t), NULL);
  radical_elements = xbt_str_split(smpi_coef_string, ";");
  xbt_dynar_foreach(radical_elements, iter, value) {
    fact.nb_values=0;
    radical_elements2 = xbt_str_split(value, ":");
    if (xbt_dynar_length(radical_elements2) <2 || xbt_dynar_length(radical_elements2) > 5)
      xbt_die("Malformed radical for smpi factor!");
    for(i =0; i<xbt_dynar_length(radical_elements2);i++ ){
        if (i==0){
           fact.factor = atol(xbt_dynar_get_as(radical_elements2, i, char *));
        }else{
           fact.values[fact.nb_values] = atof(xbt_dynar_get_as(radical_elements2, i, char *));
           fact.nb_values++;
        }
    }

    xbt_dynar_push_as(smpi_factor, s_smpi_factor_t, fact);
    XBT_DEBUG("smpi_factor:\t%ld : %d values, first: %f", fact.factor, fact.nb_values ,fact.values[0]);
    xbt_dynar_free(&radical_elements2);
  }
  xbt_dynar_free(&radical_elements);
  iter=0;
  xbt_dynar_sort(smpi_factor, &factor_cmp);
  xbt_dynar_foreach(smpi_factor, iter, fact) {
    XBT_DEBUG("smpi_factor:\t%ld : %d values, first: %f", fact.factor, fact.nb_values ,fact.values[0]);
  }
  return smpi_factor;
}

static double smpi_os(double size)
{
  if (!smpi_os_values)
    smpi_os_values =
        parse_factor(sg_cfg_get_string("smpi/os"));

  unsigned int iter = 0;
  s_smpi_factor_t fact;
  double current=0.0;
  xbt_dynar_foreach(smpi_os_values, iter, fact) {
    if (size <= fact.factor) {
        XBT_DEBUG("os : %lf <= %ld return %f", size, fact.factor, current);
      return current;
    }else{
      current=fact.values[0]+fact.values[1]*size;
    }
  }
  XBT_DEBUG("os : %lf > %ld return %f", size, fact.factor, current);

  return current;
}

static double smpi_ois(double size)
{
  if (!smpi_ois_values)
    smpi_ois_values =
        parse_factor(sg_cfg_get_string("smpi/ois"));

  unsigned int iter = 0;
  s_smpi_factor_t fact;
  double current=0.0;
  xbt_dynar_foreach(smpi_ois_values, iter, fact) {
    if (size <= fact.factor) {
        XBT_DEBUG("ois : %lf <= %ld return %f", size, fact.factor, current);
      return current;
    }else{
      current=fact.values[0]+fact.values[1]*size;
    }
  }
  XBT_DEBUG("ois : %lf > %ld return %f", size, fact.factor, current);

  return current;
}

static double smpi_or(double size)
{
  if (!smpi_or_values)
    smpi_or_values =
        parse_factor(sg_cfg_get_string("smpi/or"));

  unsigned int iter = 0;
  s_smpi_factor_t fact;
  double current=0.0;
  xbt_dynar_foreach(smpi_or_values, iter, fact) {
    if (size <= fact.factor) {
        XBT_DEBUG("or : %lf <= %ld return %f", size, fact.factor, current);
      return current;
    }else
      current=fact.values[0]+fact.values[1]*size;
  }
  XBT_DEBUG("or : %lf > %ld return %f", size, fact.factor, current);

  return current;
}

static MPI_Request build_request(void *buf, int count,
                                 MPI_Datatype datatype, int src, int dst,
                                 int tag, MPI_Comm comm, unsigned flags)
{
  MPI_Request request;

  void *old_buf = NULL;

  request = xbt_new(s_smpi_mpi_request_t, 1);

  s_smpi_subtype_t *subtype = datatype->substruct;

  if(datatype->has_subtype == 1){
    // This part handles the problem of non-contiguous memory
    old_buf = buf;
    buf = xbt_malloc(count*smpi_datatype_size(datatype));
    if (flags & SEND) {
      subtype->serialize(old_buf, buf, count, datatype->substruct);
    }
  }

  request->buf = buf;
  // This part handles the problem of non-contiguous memory (for the
  // unserialisation at the reception)
  request->old_buf = old_buf;
  request->old_type = datatype;

  request->size = smpi_datatype_size(datatype) * count;
  request->src = src;
  request->dst = dst;
  request->tag = tag;
  request->comm = comm;
  request->action = NULL;
  request->flags = flags;
  request->detached = 0;
  request->detached_sender = NULL;

  request->truncated = 0;
  request->real_size = 0;
  request->real_tag = 0;

  request->refcount=1;
#ifdef HAVE_TRACING
  request->send = 0;
  request->recv = 0;
#endif
  if (flags & SEND) smpi_datatype_unuse(datatype);

  return request;
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

void smpi_action_trace_run(char *path)
{
  char *name;
  xbt_dynar_t todo;
  xbt_dict_cursor_t cursor;

  action_fp=NULL;
  if (path) {
    action_fp = fopen(path, "r");
    xbt_assert(action_fp != NULL, "Cannot open %s: %s", path,
               strerror(errno));
  }

  if (!xbt_dict_is_empty(action_queues)) {
    XBT_WARN
      ("Not all actions got consumed. If the simulation ended successfully (without deadlock), you may want to add new processes to your deployment file.");


    xbt_dict_foreach(action_queues, cursor, name, todo) {
      XBT_WARN("Still %lu actions for %s", xbt_dynar_length(todo), name);
    }
  }

  if (path)
    fclose(action_fp);
  xbt_dict_free(&action_queues);
  action_queues = xbt_dict_new_homogeneous(NULL);
}

static void smpi_mpi_request_free_voidp(void* request)
{
  MPI_Request req = request;
  smpi_mpi_request_free(&req);
}

/* MPI Low level calls */
MPI_Request smpi_mpi_send_init(void *buf, int count, MPI_Datatype datatype,
                               int dst, int tag, MPI_Comm comm)
{
  MPI_Request request =
    build_request(buf, count, datatype, smpi_comm_rank(comm), dst, tag,
                  comm, PERSISTENT | SEND);
  request->refcount++;
  return request;
}

MPI_Request smpi_mpi_ssend_init(void *buf, int count, MPI_Datatype datatype,
                               int dst, int tag, MPI_Comm comm)
{
  MPI_Request request =
    build_request(buf, count, datatype, smpi_comm_rank(comm), dst, tag,
                  comm, PERSISTENT | SSEND | SEND);
  request->refcount++;
  return request;
}

MPI_Request smpi_mpi_recv_init(void *buf, int count, MPI_Datatype datatype,
                               int src, int tag, MPI_Comm comm)
{
  MPI_Request request =
    build_request(buf, count, datatype, src, smpi_comm_rank(comm), tag,
                  comm, PERSISTENT | RECV);
  request->refcount++;
  return request;
}

void smpi_mpi_start(MPI_Request request)
{
  smx_rdv_t mailbox;

  xbt_assert(!request->action,
             "Cannot (re)start a non-finished communication");
  if(request->flags & RECV) {
    print_request("New recv", request);
    if (request->size < sg_cfg_get_int("smpi/async_small_thres"))
      mailbox = smpi_process_mailbox_small();
    else
      mailbox = smpi_process_mailbox();
    // we make a copy here, as the size is modified by simix, and we may reuse the request in another receive later
    request->real_size=request->size;
    smpi_datatype_use(request->old_type);
    request->action = simcall_comm_irecv(mailbox, request->buf, &request->real_size, &match_recv, request);

    //integrate pseudo-timing for buffering of small messages, do not bother to execute the simcall if 0
    double sleeptime = request->detached ? smpi_or(request->size) : 0.0;
    if(sleeptime!=0.0){
        simcall_process_sleep(sleeptime);
        XBT_DEBUG("receiving size of %zu : sleep %lf ", request->size, smpi_or(request->size));
    }

  } else {

    int receiver = smpi_group_index(smpi_comm_group(request->comm), request->dst);
/*    if(receiver == MPI_UNDEFINED) {*/
/*      XBT_WARN("Trying to send a message to a wrong rank");*/
/*      return;*/
/*    }*/
    print_request("New send", request);
    if (request->size < sg_cfg_get_int("smpi/async_small_thres")) { // eager mode
      mailbox = smpi_process_remote_mailbox_small(receiver);
    }else{
      XBT_DEBUG("Send request %p is not in the permanent receive mailbox (buf: %p)",request,request->buf);
      mailbox = smpi_process_remote_mailbox(receiver);
    }
    if ( (! (request->flags & SSEND)) && (request->size < sg_cfg_get_int("smpi/send_is_detached_thres"))) {
      void *oldbuf = NULL;
      request->detached = 1;
      request->refcount++;
      if(request->old_type->has_subtype == 0){
        oldbuf = request->buf;
        if (oldbuf){
          request->buf = xbt_malloc(request->size);
          memcpy(request->buf,oldbuf,request->size);
        }
      }
      XBT_DEBUG("Send request %p is detached; buf %p copied into %p",request,oldbuf,request->buf);
    }
    // we make a copy here, as the size is modified by simix, and we may reuse the request in another receive later
    request->real_size=request->size;
    smpi_datatype_use(request->old_type);

    //if we are giving back the control to the user without waiting for completion, we have to inject timings
    double sleeptime =0.0;
    if(request->detached || (request->flags & (ISEND|SSEND))){// issend should be treated as isend
      //isend and send timings may be different
      sleeptime = (request->flags & ISEND)? smpi_ois(request->size) : smpi_os(request->size);
    }

    if(sleeptime!=0.0){
        simcall_process_sleep(sleeptime);
        XBT_DEBUG("sending size of %zu : sleep %lf ", request->size, smpi_os(request->size));
    }

    request->action =
      simcall_comm_isend(mailbox, request->size, -1.0,
                         request->buf, request->real_size,
                         &match_send,
                         &smpi_mpi_request_free_voidp, // how to free the userdata if a detached send fails
                         request,
                         // detach if msg size < eager/rdv switch limit
                         request->detached);

#ifdef HAVE_TRACING
    /* FIXME: detached sends are not traceable (request->action == NULL) */
    if (request->action)
      simcall_set_category(request->action, TRACE_internal_smpi_get_category());
#endif

  }

}

void smpi_mpi_startall(int count, MPI_Request * requests)
{
  int i;

  for(i = 0; i < count; i++) {
    smpi_mpi_start(requests[i]);
  }
}

void smpi_mpi_request_free(MPI_Request * request)
{

  if((*request) != MPI_REQUEST_NULL){
    (*request)->refcount--;
    if((*request)->refcount<0) xbt_die("wrong refcount");

    if((*request)->refcount==0){
        xbt_free(*request);
        *request = MPI_REQUEST_NULL;
    }
  }else{
      xbt_die("freeing an already free request");
  }
}

MPI_Request smpi_isend_init(void *buf, int count, MPI_Datatype datatype,
                            int dst, int tag, MPI_Comm comm)
{
  MPI_Request request =
    build_request(buf, count, datatype, smpi_comm_rank(comm), dst, tag,
                  comm, NON_PERSISTENT | SEND);

  return request;
}

MPI_Request smpi_mpi_isend(void *buf, int count, MPI_Datatype datatype,
                           int dst, int tag, MPI_Comm comm)
{
  MPI_Request request =
      build_request(buf, count, datatype, smpi_comm_rank(comm), dst, tag,
                    comm, NON_PERSISTENT | ISEND | SEND);

  smpi_mpi_start(request);
  return request;
}

MPI_Request smpi_mpi_issend(void *buf, int count, MPI_Datatype datatype,
                           int dst, int tag, MPI_Comm comm)
{
  MPI_Request request =
      build_request(buf, count, datatype, smpi_comm_rank(comm), dst, tag,
                    comm, NON_PERSISTENT | ISEND | SSEND | SEND);
  smpi_mpi_start(request);
  return request;
}



MPI_Request smpi_irecv_init(void *buf, int count, MPI_Datatype datatype,
                            int src, int tag, MPI_Comm comm)
{
  MPI_Request request =
    build_request(buf, count, datatype, src, smpi_comm_rank(comm), tag,
                  comm, NON_PERSISTENT | RECV);
  return request;
}

MPI_Request smpi_mpi_irecv(void *buf, int count, MPI_Datatype datatype,
                           int src, int tag, MPI_Comm comm)
{
  MPI_Request request =
      build_request(buf, count, datatype, src, smpi_comm_rank(comm), tag,
                    comm, NON_PERSISTENT | RECV);

  smpi_mpi_start(request);
  return request;
}

void smpi_mpi_recv(void *buf, int count, MPI_Datatype datatype, int src,
                   int tag, MPI_Comm comm, MPI_Status * status)
{
  MPI_Request request;
  request = smpi_mpi_irecv(buf, count, datatype, src, tag, comm);
  smpi_mpi_wait(&request, status);
}



void smpi_mpi_send(void *buf, int count, MPI_Datatype datatype, int dst,
                   int tag, MPI_Comm comm)
{
  MPI_Request request =
      build_request(buf, count, datatype, smpi_comm_rank(comm), dst, tag,
                    comm, NON_PERSISTENT | SEND);

  smpi_mpi_start(request);
  smpi_mpi_wait(&request, MPI_STATUS_IGNORE);

}

void smpi_mpi_ssend(void *buf, int count, MPI_Datatype datatype,
                           int dst, int tag, MPI_Comm comm)
{
  MPI_Request request = smpi_mpi_issend(buf, count, datatype, dst, tag, comm);
  smpi_mpi_wait(&request, MPI_STATUS_IGNORE);
}

void smpi_mpi_sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                       int dst, int sendtag, void *recvbuf, int recvcount,
                       MPI_Datatype recvtype, int src, int recvtag,
                       MPI_Comm comm, MPI_Status * status)
{
  MPI_Request requests[2];
  MPI_Status stats[2];

  requests[0] =
    smpi_isend_init(sendbuf, sendcount, sendtype, dst, sendtag, comm);
  requests[1] =
    smpi_irecv_init(recvbuf, recvcount, recvtype, src, recvtag, comm);
  smpi_mpi_startall(2, requests);
  smpi_mpi_waitall(2, requests, stats);
  if(status != MPI_STATUS_IGNORE) {
    // Copy receive status
    *status = stats[1];
  }
}

int smpi_mpi_get_count(MPI_Status * status, MPI_Datatype datatype)
{
  return status->count / smpi_datatype_size(datatype);
}

static void finish_wait(MPI_Request * request, MPI_Status * status)
{
  MPI_Request req = *request;
  if(status != MPI_STATUS_IGNORE)
    smpi_empty_status(status);

  if(!(req->detached && req->flags & SEND)){
    if(status != MPI_STATUS_IGNORE) {
      status->MPI_SOURCE = req->src == MPI_ANY_SOURCE ? req->real_src : req->src;
      status->MPI_TAG = req->tag == MPI_ANY_TAG ? req->real_tag : req->tag;
      status->MPI_ERROR = req->truncated ? MPI_ERR_TRUNCATE : MPI_SUCCESS;
      // this handles the case were size in receive differs from size in send
      // FIXME: really this should just contain the count of receive-type blocks,
      // right?
      status->count = req->real_size;
    }

    print_request("Finishing", req);
    MPI_Datatype datatype = req->old_type;

    if(datatype->has_subtype == 1){
        // This part handles the problem of non-contignous memory
        // the unserialization at the reception
      s_smpi_subtype_t *subtype = datatype->substruct;
      if(req->flags & RECV) {
        subtype->unserialize(req->buf, req->old_buf, req->real_size/smpi_datatype_size(datatype) , datatype->substruct);
      }
      if(req->detached == 0) free(req->buf);
    }
    smpi_datatype_unuse(datatype);
  }

  if(req->detached_sender!=NULL){
    smpi_mpi_request_free(&(req->detached_sender));
  }

  if(req->flags & NON_PERSISTENT) {
    smpi_mpi_request_free(request);
  } else {
    req->action = NULL;
  }
}

int smpi_mpi_test(MPI_Request * request, MPI_Status * status) {
  int flag;

  //assume that request is not MPI_REQUEST_NULL (filtered in PMPI_Test or smpi_mpi_testall before)
  if ((*request)->action == NULL)
    flag = 1;
  else
    flag = simcall_comm_test((*request)->action);
  if(flag) {
    (*request)->refcount++;
    finish_wait(request, status);
  }else{
    smpi_empty_status(status);
  }
  return flag;
}

int smpi_mpi_testany(int count, MPI_Request requests[], int *index,
                     MPI_Status * status)
{
  xbt_dynar_t comms;
  int i, flag, size;
  int* map;

  *index = MPI_UNDEFINED;
  flag = 0;
  if(count > 0) {
    comms = xbt_dynar_new(sizeof(smx_action_t), NULL);
    map = xbt_new(int, count);
    size = 0;
    for(i = 0; i < count; i++) {
      if((requests[i]!=MPI_REQUEST_NULL) && requests[i]->action) {
         xbt_dynar_push(comms, &requests[i]->action);
         map[size] = i;
         size++;
      }
    }
    if(size > 0) {
      i = simcall_comm_testany(comms);
      // not MPI_UNDEFINED, as this is a simix return code
      if(i != -1) {
        *index = map[i];
        finish_wait(&requests[*index], status);
        flag = 1;
      }
    }else{
        //all requests are null or inactive, return true
        flag=1;
        smpi_empty_status(status);
    }
    xbt_free(map);
    xbt_dynar_free(&comms);
  }

  return flag;
}


int smpi_mpi_testall(int count, MPI_Request requests[],
                     MPI_Status status[])
{
  MPI_Status stat;
  MPI_Status *pstat = status == MPI_STATUSES_IGNORE ? MPI_STATUS_IGNORE : &stat;
  int flag=1;
  int i;
  for(i=0; i<count; i++){
    if(requests[i]!= MPI_REQUEST_NULL){
      if (smpi_mpi_test(&requests[i], pstat)!=1){
        flag=0;
      }
    }else{
      smpi_empty_status(pstat);
    }
    if(status != MPI_STATUSES_IGNORE) {
      status[i] = *pstat;
    }
  }
  return flag;
}

void smpi_mpi_probe(int source, int tag, MPI_Comm comm, MPI_Status* status){
  int flag=0;
  //FIXME find another wait to avoid busy waiting ?
  // the issue here is that we have to wait on a nonexistent comm
  while(flag==0){
    smpi_mpi_iprobe(source, tag, comm, &flag, status);
    XBT_DEBUG("Busy Waiting on probing : %d", flag);
    if(!flag) {
      simcall_process_sleep(0.0001);
    }
  }
}

void smpi_mpi_iprobe(int source, int tag, MPI_Comm comm, int* flag, MPI_Status* status){
  MPI_Request request =build_request(NULL, 0, MPI_CHAR, source, smpi_comm_rank(comm), tag,
            comm, NON_PERSISTENT | RECV);

  // behave like a receive, but don't do it
  smx_rdv_t mailbox;

  print_request("New iprobe", request);
  // We have to test both mailboxes as we don't know if we will receive one one or another
    if (sg_cfg_get_int("smpi/async_small_thres")>0){
        mailbox = smpi_process_mailbox_small();
        XBT_DEBUG("trying to probe the perm recv mailbox");
        request->action = simcall_comm_iprobe(mailbox, request->src, request->tag, &match_recv, (void*)request);
    }
    if (request->action==NULL){
	mailbox = smpi_process_mailbox();
        XBT_DEBUG("trying to probe the other mailbox");
        request->action = simcall_comm_iprobe(mailbox, request->src, request->tag, &match_recv, (void*)request);
    }

  if(request->action){
    MPI_Request req = (MPI_Request)SIMIX_comm_get_src_data(request->action);
    *flag = 1;
    if(status != MPI_STATUS_IGNORE) {
      status->MPI_SOURCE = req->src;
      status->MPI_TAG = req->tag;
      status->MPI_ERROR = MPI_SUCCESS;
      status->count = req->real_size;
    }
  }
  else *flag = 0;
  smpi_mpi_request_free(&request);

  return;
}

void smpi_mpi_wait(MPI_Request * request, MPI_Status * status)
{
  print_request("Waiting", *request);
  if ((*request)->action != NULL) { // this is not a detached send
    simcall_comm_wait((*request)->action, -1.0);
  }
  finish_wait(request, status);

  // FIXME for a detached send, finish_wait is not called:
}

int smpi_mpi_waitany(int count, MPI_Request requests[],
                     MPI_Status * status)
{
  xbt_dynar_t comms;
  int i, size, index;
  int *map;

  index = MPI_UNDEFINED;
  if(count > 0) {
    // Wait for a request to complete
    comms = xbt_dynar_new(sizeof(smx_action_t), NULL);
    map = xbt_new(int, count);
    size = 0;
    XBT_DEBUG("Wait for one of %d", count);
    for(i = 0; i < count; i++) {
      if(requests[i] != MPI_REQUEST_NULL) {
        if (requests[i]->action != NULL) {
          XBT_DEBUG("Waiting any %p ", requests[i]);
          xbt_dynar_push(comms, &requests[i]->action);
          map[size] = i;
          size++;
        }else{
         //This is a finished detached request, let's return this one
         size=0;//so we free the dynar but don't do the waitany call
         index=i;
         finish_wait(&requests[i], status);//cleanup if refcount = 0
         requests[i]=MPI_REQUEST_NULL;//set to null
         break;
         }
      }
    }
    if(size > 0) {
      i = simcall_comm_waitany(comms);

      // not MPI_UNDEFINED, as this is a simix return code
      if (i != -1) {
        index = map[i];
        finish_wait(&requests[index], status);
      }
    }
    xbt_free(map);
    xbt_dynar_free(&comms);
  }

  if (index==MPI_UNDEFINED)
    smpi_empty_status(status);

  return index;
}

int smpi_mpi_waitall(int count, MPI_Request requests[],
                      MPI_Status status[])
{
  int  index, c;
  MPI_Status stat;
  MPI_Status *pstat = status == MPI_STATUSES_IGNORE ? MPI_STATUS_IGNORE : &stat;
  int retvalue = MPI_SUCCESS;
  //tag invalid requests in the set
  if (status != MPI_STATUSES_IGNORE) {
    for (c = 0; c < count; c++) {
      if (requests[c] == MPI_REQUEST_NULL || requests[c]->dst == MPI_PROC_NULL) {
        smpi_empty_status(&status[c]);
      } else if (requests[c]->src == MPI_PROC_NULL) {
        smpi_empty_status(&status[c]);
        status[c].MPI_SOURCE = MPI_PROC_NULL;
      }
    }
  }
  for(c = 0; c < count; c++) {
      if(MC_is_active()) {
        smpi_mpi_wait(&requests[c], pstat);
        index = c;
      } else {
        index = smpi_mpi_waitany(count, requests, pstat);
        if(index == MPI_UNDEFINED) {
          break;
       }
      if (status != MPI_STATUSES_IGNORE) {
        status[index] = *pstat;
        if (status[index].MPI_ERROR == MPI_ERR_TRUNCATE)
          retvalue = MPI_ERR_IN_STATUS;
      }
    }
  }

  return retvalue;
}

int smpi_mpi_waitsome(int incount, MPI_Request requests[], int *indices,
                      MPI_Status status[])
{
  int i, count, index;
  MPI_Status stat;
  MPI_Status *pstat = status == MPI_STATUSES_IGNORE ? MPI_STATUS_IGNORE : &stat;

  count = 0;
  for(i = 0; i < incount; i++)
  {
    index=smpi_mpi_waitany(incount, requests, pstat);
    if(index!=MPI_UNDEFINED){
      indices[count] = index;
      count++;
      if(status != MPI_STATUSES_IGNORE) {
        status[index] = *pstat;
      }
    }else{
      return MPI_UNDEFINED;
    }
  }
  return count;
}

int smpi_mpi_testsome(int incount, MPI_Request requests[], int *indices,
                      MPI_Status status[])
{
  int i, count, count_dead;
  MPI_Status stat;
  MPI_Status *pstat = status == MPI_STATUSES_IGNORE ? MPI_STATUS_IGNORE : &stat;

  count = 0;
  count_dead = 0;
  for(i = 0; i < incount; i++) {
    if((requests[i] != MPI_REQUEST_NULL)) {
      if(smpi_mpi_test(&requests[i], pstat)) {
         indices[count] = i;
         count++;
         if(status != MPI_STATUSES_IGNORE) {
           status[i] = *pstat;
         }
      }
    }else{
      count_dead++;
    }
  }
  if(count_dead==incount)return MPI_UNDEFINED;
  else return count;
}

void smpi_mpi_bcast(void *buf, int count, MPI_Datatype datatype, int root,
                    MPI_Comm comm)
{
  // arity=2: a binary tree, arity=4 seem to be a good setting (see P2P-MPI))
  nary_tree_bcast(buf, count, datatype, root, comm, 4);
}

void smpi_mpi_barrier(MPI_Comm comm)
{
  // arity=2: a binary tree, arity=4 seem to be a good setting (see P2P-MPI))
  nary_tree_barrier(comm, 4);
}

void smpi_mpi_gather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                     void *recvbuf, int recvcount, MPI_Datatype recvtype,
                     int root, MPI_Comm comm)
{
  int system_tag = 666;
  int rank, size, src, index;
  MPI_Aint lb = 0, recvext = 0;
  MPI_Request *requests;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  if(rank != root) {
    // Send buffer to root
    smpi_mpi_send(sendbuf, sendcount, sendtype, root, system_tag, comm);
  } else {
    // FIXME: check for errors
    smpi_datatype_extent(recvtype, &lb, &recvext);
    // Local copy from root
    smpi_datatype_copy(sendbuf, sendcount, sendtype,
                       (char *)recvbuf + root * recvcount * recvext, recvcount, recvtype);
    // Receive buffers from senders
    requests = xbt_new(MPI_Request, size - 1);
    index = 0;
    for(src = 0; src < size; src++) {
      if(src != root) {
        requests[index] = smpi_irecv_init((char *)recvbuf + src * recvcount * recvext,
                                          recvcount, recvtype,
                                          src, system_tag, comm);
        index++;
      }
    }
    // Wait for completion of irecv's.
    smpi_mpi_startall(size - 1, requests);
    smpi_mpi_waitall(size - 1, requests, MPI_STATUS_IGNORE);
    xbt_free(requests);
  }
}

void smpi_mpi_gatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                      void *recvbuf, int *recvcounts, int *displs,
                      MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int system_tag = 666;
  int rank, size, src, index;
  MPI_Aint lb = 0, recvext = 0;
  MPI_Request *requests;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  if(rank != root) {
    // Send buffer to root
    smpi_mpi_send(sendbuf, sendcount, sendtype, root, system_tag, comm);
  } else {
    // FIXME: check for errors
    smpi_datatype_extent(recvtype, &lb, &recvext);
    // Local copy from root
    smpi_datatype_copy(sendbuf, sendcount, sendtype,
                       (char *)recvbuf + displs[root] * recvext,
                       recvcounts[root], recvtype);
    // Receive buffers from senders
    requests = xbt_new(MPI_Request, size - 1);
    index = 0;
    for(src = 0; src < size; src++) {
      if(src != root) {
        requests[index] =
          smpi_irecv_init((char *)recvbuf + displs[src] * recvext,
                          recvcounts[src], recvtype, src, system_tag, comm);
        index++;
      }
    }
    // Wait for completion of irecv's.
    smpi_mpi_startall(size - 1, requests);
    smpi_mpi_waitall(size - 1, requests, MPI_STATUS_IGNORE);
    xbt_free(requests);
  }
}

void smpi_mpi_allgather(void *sendbuf, int sendcount,
                        MPI_Datatype sendtype, void *recvbuf,
                        int recvcount, MPI_Datatype recvtype,
                        MPI_Comm comm)
{
  int system_tag = 666;
  int rank, size, other, index;
  MPI_Aint lb = 0, recvext = 0;
  MPI_Request *requests;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  // FIXME: check for errors
  smpi_datatype_extent(recvtype, &lb, &recvext);
  // Local copy from self
  smpi_datatype_copy(sendbuf, sendcount, sendtype,
                     (char *)recvbuf + rank * recvcount * recvext, recvcount,
                     recvtype);
  // Send/Recv buffers to/from others;
  requests = xbt_new(MPI_Request, 2 * (size - 1));
  index = 0;
  for(other = 0; other < size; other++) {
    if(other != rank) {
      requests[index] =
        smpi_isend_init(sendbuf, sendcount, sendtype, other, system_tag,
                        comm);
      index++;
      requests[index] = smpi_irecv_init((char *)recvbuf + other * recvcount * recvext,
                                        recvcount, recvtype, other,
                                        system_tag, comm);
      index++;
    }
  }
  // Wait for completion of all comms.
  smpi_mpi_startall(2 * (size - 1), requests);
  smpi_mpi_waitall(2 * (size - 1), requests, MPI_STATUS_IGNORE);
  xbt_free(requests);
}

void smpi_mpi_allgatherv(void *sendbuf, int sendcount,
                         MPI_Datatype sendtype, void *recvbuf,
                         int *recvcounts, int *displs,
                         MPI_Datatype recvtype, MPI_Comm comm)
{
  int system_tag = 666;
  int rank, size, other, index;
  MPI_Aint lb = 0, recvext = 0;
  MPI_Request *requests;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  // FIXME: check for errors
  smpi_datatype_extent(recvtype, &lb, &recvext);
  // Local copy from self
  smpi_datatype_copy(sendbuf, sendcount, sendtype,
                     (char *)recvbuf + displs[rank] * recvext,
                     recvcounts[rank], recvtype);
  // Send buffers to others;
  requests = xbt_new(MPI_Request, 2 * (size - 1));
  index = 0;
  for(other = 0; other < size; other++) {
    if(other != rank) {
      requests[index] =
        smpi_isend_init(sendbuf, sendcount, sendtype, other, system_tag,
                        comm);
      index++;
      requests[index] =
        smpi_irecv_init((char *)recvbuf + displs[other] * recvext, recvcounts[other],
                        recvtype, other, system_tag, comm);
      index++;
    }
  }
  // Wait for completion of all comms.
  smpi_mpi_startall(2 * (size - 1), requests);
  smpi_mpi_waitall(2 * (size - 1), requests, MPI_STATUS_IGNORE);
  xbt_free(requests);
}

void smpi_mpi_scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                      void *recvbuf, int recvcount, MPI_Datatype recvtype,
                      int root, MPI_Comm comm)
{
  int system_tag = 666;
  int rank, size, dst, index;
  MPI_Aint lb = 0, sendext = 0;
  MPI_Request *requests;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  if(rank != root) {
    // Recv buffer from root
    smpi_mpi_recv(recvbuf, recvcount, recvtype, root, system_tag, comm,
                  MPI_STATUS_IGNORE);
  } else {
    // FIXME: check for errors
    smpi_datatype_extent(sendtype, &lb, &sendext);
    // Local copy from root
    smpi_datatype_copy((char *)sendbuf + root * sendcount * sendext,
                       sendcount, sendtype, recvbuf, recvcount, recvtype);
    // Send buffers to receivers
    requests = xbt_new(MPI_Request, size - 1);
    index = 0;
    for(dst = 0; dst < size; dst++) {
      if(dst != root) {
        requests[index] = smpi_isend_init((char *)sendbuf + dst * sendcount * sendext,
                                          sendcount, sendtype, dst,
                                          system_tag, comm);
        index++;
      }
    }
    // Wait for completion of isend's.
    smpi_mpi_startall(size - 1, requests);
    smpi_mpi_waitall(size - 1, requests, MPI_STATUS_IGNORE);
    xbt_free(requests);
  }
}

void smpi_mpi_scatterv(void *sendbuf, int *sendcounts, int *displs,
                       MPI_Datatype sendtype, void *recvbuf, int recvcount,
                       MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int system_tag = 666;
  int rank, size, dst, index;
  MPI_Aint lb = 0, sendext = 0;
  MPI_Request *requests;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  if(rank != root) {
    // Recv buffer from root
    smpi_mpi_recv(recvbuf, recvcount, recvtype, root, system_tag, comm,
                  MPI_STATUS_IGNORE);
  } else {
    // FIXME: check for errors
    smpi_datatype_extent(sendtype, &lb, &sendext);
    // Local copy from root
    smpi_datatype_copy((char *)sendbuf + displs[root] * sendext, sendcounts[root],
                       sendtype, recvbuf, recvcount, recvtype);
    // Send buffers to receivers
    requests = xbt_new(MPI_Request, size - 1);
    index = 0;
    for(dst = 0; dst < size; dst++) {
      if(dst != root) {
        requests[index] =
          smpi_isend_init((char *)sendbuf + displs[dst] * sendext, sendcounts[dst],
                          sendtype, dst, system_tag, comm);
        index++;
      }
    }
    // Wait for completion of isend's.
    smpi_mpi_startall(size - 1, requests);
    smpi_mpi_waitall(size - 1, requests, MPI_STATUS_IGNORE);
    xbt_free(requests);
  }
}

void smpi_mpi_reduce(void *sendbuf, void *recvbuf, int count,
                     MPI_Datatype datatype, MPI_Op op, int root,
                     MPI_Comm comm)
{
  int system_tag = 666;
  int rank, size, src, index;
  MPI_Aint lb = 0, dataext = 0;
  MPI_Request *requests;
  void **tmpbufs;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  if(rank != root) {
    // Send buffer to root
    smpi_mpi_send(sendbuf, count, datatype, root, system_tag, comm);
  } else {
    // FIXME: check for errors
    smpi_datatype_extent(datatype, &lb, &dataext);
    // Local copy from root
    if (sendbuf && recvbuf)
      smpi_datatype_copy(sendbuf, count, datatype, recvbuf, count, datatype);
    // Receive buffers from senders
    //TODO: make a MPI_barrier here ?
    requests = xbt_new(MPI_Request, size - 1);
    tmpbufs = xbt_new(void *, size - 1);
    index = 0;
    for(src = 0; src < size; src++) {
      if(src != root) {
        // FIXME: possibly overkill we we have contiguous/noncontiguous data
        //  mapping...
        tmpbufs[index] = xbt_malloc(count * dataext);
        requests[index] =
          smpi_irecv_init(tmpbufs[index], count, datatype, src,
                          system_tag, comm);
        index++;
      }
    }
    // Wait for completion of irecv's.
    smpi_mpi_startall(size - 1, requests);
    for(src = 0; src < size - 1; src++) {
      index = smpi_mpi_waitany(size - 1, requests, MPI_STATUS_IGNORE);
      XBT_DEBUG("finished waiting any request with index %d", index);
      if(index == MPI_UNDEFINED) {
        break;
      }
      if(op) /* op can be MPI_OP_NULL that does nothing */
        smpi_op_apply(op, tmpbufs[index], recvbuf, &count, &datatype);
    }
    for(index = 0; index < size - 1; index++) {
      xbt_free(tmpbufs[index]);
    }
    xbt_free(tmpbufs);
    xbt_free(requests);
  }
}

void smpi_mpi_allreduce(void *sendbuf, void *recvbuf, int count,
                        MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  smpi_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm);
  smpi_mpi_bcast(recvbuf, count, datatype, 0, comm);
}

void smpi_mpi_scan(void *sendbuf, void *recvbuf, int count,
                   MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int system_tag = 666;
  int rank, size, other, index;
  MPI_Aint lb = 0, dataext = 0;
  MPI_Request *requests;
  void **tmpbufs;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);

  // FIXME: check for errors
  smpi_datatype_extent(datatype, &lb, &dataext);

  // Local copy from self
  smpi_datatype_copy(sendbuf, count, datatype, recvbuf, count, datatype);

  // Send/Recv buffers to/from others;
  requests = xbt_new(MPI_Request, size - 1);
  tmpbufs = xbt_new(void *, rank);
  index = 0;
  for(other = 0; other < rank; other++) {
    // FIXME: possibly overkill we we have contiguous/noncontiguous data
    // mapping...
    tmpbufs[index] = xbt_malloc(count * dataext);
    requests[index] =
      smpi_irecv_init(tmpbufs[index], count, datatype, other, system_tag,
                      comm);
    index++;
  }
  for(other = rank + 1; other < size; other++) {
    requests[index] =
      smpi_isend_init(sendbuf, count, datatype, other, system_tag, comm);
    index++;
  }
  // Wait for completion of all comms.
  smpi_mpi_startall(size - 1, requests);
  for(other = 0; other < size - 1; other++) {
    index = smpi_mpi_waitany(size - 1, requests, MPI_STATUS_IGNORE);
    if(index == MPI_UNDEFINED) {
      break;
    }
    if(index < rank) {
      // #Request is below rank: it's a irecv
      smpi_op_apply(op, tmpbufs[index], recvbuf, &count, &datatype);
    }
  }
  for(index = 0; index < rank; index++) {
    xbt_free(tmpbufs[index]);
  }
  xbt_free(tmpbufs);
  xbt_free(requests);
}

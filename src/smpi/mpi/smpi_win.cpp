/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_win.hpp"
#include "private.hpp"
#include "smpi_coll.hpp"
#include "smpi_comm.hpp"
#include "smpi_datatype.hpp"
#include "smpi_info.hpp"
#include "smpi_keyvals.hpp"
#include "smpi_process.hpp"
#include "smpi_request.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_rma, smpi, "Logging specific to SMPI (RMA operations)");

namespace simgrid{
namespace smpi{
std::unordered_map<int, smpi_key_elem> Win::keyvals_;
int Win::keyval_id_=0;

Win::Win(void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, int allocated, int dynamic): base_(base), size_(size), disp_unit_(disp_unit), assert_(0), info_(info), comm_(comm), allocated_(allocated), dynamic_(dynamic){
  int comm_size = comm->size();
  rank_      = comm->rank();
  XBT_DEBUG("Creating window");
  if(info!=MPI_INFO_NULL)
    info->ref();
  name_ = nullptr;
  opened_ = 0;
  group_ = MPI_GROUP_NULL;
  requests_ = new std::vector<MPI_Request>();
  mut_=xbt_mutex_init();
  lock_mut_=xbt_mutex_init();
  atomic_mut_=xbt_mutex_init();
  connected_wins_ = new MPI_Win[comm_size];
  connected_wins_[rank_] = this;
  count_ = 0;
  if(rank_==0){
    bar_ = MSG_barrier_init(comm_size);
  }
  mode_=0;

  comm->add_rma_win(this);

  Colls::allgather(&(connected_wins_[rank_]), sizeof(MPI_Win), MPI_BYTE, connected_wins_, sizeof(MPI_Win),
                         MPI_BYTE, comm);

  Colls::bcast(&(bar_), sizeof(msg_bar_t), MPI_BYTE, 0, comm);

  Colls::barrier(comm);
}

Win::~Win(){
  //As per the standard, perform a barrier to ensure every async comm is finished
  MSG_barrier_wait(bar_);

  int finished = finish_comms();
  XBT_DEBUG("Win destructor - Finished %d RMA calls", finished);

  delete requests_;
  delete[] connected_wins_;
  if (name_ != nullptr){
    xbt_free(name_);
  }
  if(info_!=MPI_INFO_NULL){
    MPI_Info_free(&info_);
  }

  comm_->remove_rma_win(this);

  Colls::barrier(comm_);
  int rank=comm_->rank();
  if(rank == 0)
    MSG_barrier_destroy(bar_);
  xbt_mutex_destroy(mut_);
  xbt_mutex_destroy(lock_mut_);
  xbt_mutex_destroy(atomic_mut_);

  if(allocated_ !=0)
    xbt_free(base_);

  cleanup_attr<Win>();
}

int Win::attach (void *base, MPI_Aint size){
  if (not(base_ == MPI_BOTTOM || base_ == 0))
    return MPI_ERR_ARG;
  base_=0;//actually the address will be given in the RMA calls, as being the disp.
  size_+=size;
  return MPI_SUCCESS;
}

int Win::detach (void *base){
  base_=MPI_BOTTOM;
  size_=-1;
  return MPI_SUCCESS;
}

void Win::get_name(char* name, int* length){
  if(name_==nullptr){
    *length=0;
    name=nullptr;
    return;
  }
  *length = strlen(name_);
  strncpy(name, name_, *length+1);
}

void Win::get_group(MPI_Group* group){
  if(comm_ != MPI_COMM_NULL){
    *group = comm_->group();
  } else {
    *group = MPI_GROUP_NULL;
  }
}

MPI_Info Win::info(){
  if(info_== MPI_INFO_NULL)
    info_ = new Info();
  info_->ref();
  return info_;
}

int Win::rank(){
  return rank_;
}

MPI_Aint Win::size(){
  return size_;
}

void* Win::base(){
  return base_;
}

int Win::disp_unit(){
  return disp_unit_;
}

int Win::dynamic(){
  return dynamic_;
}

void Win::set_info(MPI_Info info){
  if(info_!= MPI_INFO_NULL)
    info->ref();
  info_=info;
}

void Win::set_name(char* name){
  name_ = xbt_strdup(name);
}

int Win::fence(int assert)
{
  XBT_DEBUG("Entering fence");
  if (opened_ == 0)
    opened_=1;
  if (assert != MPI_MODE_NOPRECEDE) {
    // This is not the first fence => finalize what came before
    MSG_barrier_wait(bar_);
    xbt_mutex_acquire(mut_);
    // This (simulated) mutex ensures that no process pushes to the vector of requests during the waitall.
    // Without this, the vector could get redimensionned when another process pushes.
    // This would result in the array used by Request::waitall() to be invalidated.
    // Another solution would be to copy the data and cleanup the vector *before* Request::waitall
    std::vector<MPI_Request> *reqs = requests_;
    int size = static_cast<int>(reqs->size());
    // start all requests that have been prepared by another process
    if (size > 0) {
      MPI_Request* treqs = &(*reqs)[0];
      Request::waitall(size, treqs, MPI_STATUSES_IGNORE);
    }
    count_=0;
    xbt_mutex_release(mut_);
  }

  if(assert==MPI_MODE_NOSUCCEED)//there should be no ops after this one, tell we are closed.
    opened_=0;
  assert_ = assert;

  MSG_barrier_wait(bar_);
  XBT_DEBUG("Leaving fence");

  return MPI_SUCCESS;
}

int Win::put( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Request* request)
{
  //get receiver pointer
  MPI_Win recv_win = connected_wins_[target_rank];

  if(opened_==0){//check that post/start has been done
    // no fence or start .. lock ok ?
    int locked=0;
    for (auto const& it : recv_win->lockers_)
      if (it == comm_->rank())
        locked = 1;
    if(locked != 1)
      return MPI_ERR_WIN;
  }

  if(target_count*target_datatype->get_extent()>recv_win->size_)
    return MPI_ERR_ARG;

  void* recv_addr = static_cast<void*> ( static_cast<char*>(recv_win->base_) + target_disp * recv_win->disp_unit_);
  XBT_DEBUG("Entering MPI_Put to %d", target_rank);

  if(target_rank != comm_->rank()){
    //prepare send_request
    MPI_Request sreq = Request::rma_send_init(origin_addr, origin_count, origin_datatype, smpi_process()->index(),
        comm_->group()->index(target_rank), SMPI_RMA_TAG+1, comm_, MPI_OP_NULL);

    //prepare receiver request
    MPI_Request rreq = Request::rma_recv_init(recv_addr, target_count, target_datatype, smpi_process()->index(),
        comm_->group()->index(target_rank), SMPI_RMA_TAG+1, recv_win->comm_, MPI_OP_NULL);

    //start send
    sreq->start();

    if(request!=nullptr){
      *request=sreq;
    }else{
      xbt_mutex_acquire(mut_);
      requests_->push_back(sreq);
      xbt_mutex_release(mut_);
    }

    //push request to receiver's win
    xbt_mutex_acquire(recv_win->mut_);
    recv_win->requests_->push_back(rreq);
    rreq->start();
    xbt_mutex_release(recv_win->mut_);

  }else{
    Datatype::copy(origin_addr, origin_count, origin_datatype, recv_addr, target_count, target_datatype);
    if(request!=nullptr)
      *request = MPI_REQUEST_NULL;
  }

  return MPI_SUCCESS;
}

int Win::get( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Request* request)
{
  //get sender pointer
  MPI_Win send_win = connected_wins_[target_rank];

  if(opened_==0){//check that post/start has been done
    // no fence or start .. lock ok ?
    int locked=0;
    for (auto const& it : send_win->lockers_)
      if (it == comm_->rank())
        locked = 1;
    if(locked != 1)
      return MPI_ERR_WIN;
  }

  if(target_count*target_datatype->get_extent()>send_win->size_)
    return MPI_ERR_ARG;

  void* send_addr = static_cast<void*>(static_cast<char*>(send_win->base_) + target_disp * send_win->disp_unit_);
  XBT_DEBUG("Entering MPI_Get from %d", target_rank);

  if(target_rank != comm_->rank()){
    //prepare send_request
    MPI_Request sreq = Request::rma_send_init(send_addr, target_count, target_datatype,
        comm_->group()->index(target_rank), smpi_process()->index(), SMPI_RMA_TAG+2, send_win->comm_,
        MPI_OP_NULL);

    //prepare receiver request
    MPI_Request rreq = Request::rma_recv_init(origin_addr, origin_count, origin_datatype,
        comm_->group()->index(target_rank), smpi_process()->index(), SMPI_RMA_TAG+2, comm_,
        MPI_OP_NULL);

    //start the send, with another process than us as sender.
    sreq->start();
    //push request to receiver's win
    xbt_mutex_acquire(send_win->mut_);
    send_win->requests_->push_back(sreq);
    xbt_mutex_release(send_win->mut_);

    //start recv
    rreq->start();

    if(request!=nullptr){
      *request=rreq;
    }else{
      xbt_mutex_acquire(mut_);
      requests_->push_back(rreq);
      xbt_mutex_release(mut_);
    }

  }else{
    Datatype::copy(send_addr, target_count, target_datatype, origin_addr, origin_count, origin_datatype);
    if(request!=nullptr)
      *request=MPI_REQUEST_NULL;
  }

  return MPI_SUCCESS;
}


int Win::accumulate( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Request* request)
{

  //get receiver pointer
  MPI_Win recv_win = connected_wins_[target_rank];

  if(opened_==0){//check that post/start has been done
    // no fence or start .. lock ok ?
    int locked=0;
    for (auto const& it : recv_win->lockers_)
      if (it == comm_->rank())
        locked = 1;
    if(locked != 1)
      return MPI_ERR_WIN;
  }
  //FIXME: local version

  if(target_count*target_datatype->get_extent()>recv_win->size_)
    return MPI_ERR_ARG;

  void* recv_addr = static_cast<void*>(static_cast<char*>(recv_win->base_) + target_disp * recv_win->disp_unit_);
  XBT_DEBUG("Entering MPI_Accumulate to %d", target_rank);
    //As the tag will be used for ordering of the operations, substract count from it (to avoid collisions with other SMPI tags, SMPI_RMA_TAG is set below all the other ones we use )
    //prepare send_request

    MPI_Request sreq = Request::rma_send_init(origin_addr, origin_count, origin_datatype,
        smpi_process()->index(), comm_->group()->index(target_rank), SMPI_RMA_TAG-3-count_, comm_, op);

    //prepare receiver request
    MPI_Request rreq = Request::rma_recv_init(recv_addr, target_count, target_datatype,
        smpi_process()->index(), comm_->group()->index(target_rank), SMPI_RMA_TAG-3-count_, recv_win->comm_, op);

    count_++;

    //start send
    sreq->start();
    //push request to receiver's win
    xbt_mutex_acquire(recv_win->mut_);
    recv_win->requests_->push_back(rreq);
    rreq->start();
    xbt_mutex_release(recv_win->mut_);

    if(request!=nullptr){
      *request=sreq;
    }else{
      xbt_mutex_acquire(mut_);
      requests_->push_back(sreq);
      xbt_mutex_release(mut_);
    }

  return MPI_SUCCESS;
}

int Win::get_accumulate( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, void *result_addr,
              int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, int target_count,
              MPI_Datatype target_datatype, MPI_Op op, MPI_Request* request){

  //get sender pointer
  MPI_Win send_win = connected_wins_[target_rank];

  if(opened_==0){//check that post/start has been done
    // no fence or start .. lock ok ?
    int locked=0;
    for (auto const& it : send_win->lockers_)
      if (it == comm_->rank())
        locked = 1;
    if(locked != 1)
      return MPI_ERR_WIN;
  }

  if(target_count*target_datatype->get_extent()>send_win->size_)
    return MPI_ERR_ARG;

  XBT_DEBUG("Entering MPI_Get_accumulate from %d", target_rank);
  //need to be sure ops are correctly ordered, so finish request here ? slow.
  MPI_Request req;
  xbt_mutex_acquire(send_win->atomic_mut_);
  get(result_addr, result_count, result_datatype, target_rank,
              target_disp, target_count, target_datatype, &req);
  if (req != MPI_REQUEST_NULL)
    Request::wait(&req, MPI_STATUS_IGNORE);
  if(op!=MPI_NO_OP)
    accumulate(origin_addr, origin_count, origin_datatype, target_rank,
              target_disp, target_count, target_datatype, op, &req);
  if (req != MPI_REQUEST_NULL)
    Request::wait(&req, MPI_STATUS_IGNORE);
  xbt_mutex_release(send_win->atomic_mut_);
  return MPI_SUCCESS;

}

int Win::compare_and_swap(void *origin_addr, void *compare_addr,
        void *result_addr, MPI_Datatype datatype, int target_rank,
        MPI_Aint target_disp){
  //get sender pointer
  MPI_Win send_win = connected_wins_[target_rank];

  if(opened_==0){//check that post/start has been done
    // no fence or start .. lock ok ?
    int locked=0;
    for (auto const& it : send_win->lockers_)
      if (it == comm_->rank())
        locked = 1;
    if(locked != 1)
      return MPI_ERR_WIN;
  }

  XBT_DEBUG("Entering MPI_Compare_and_swap with %d", target_rank);
  MPI_Request req = MPI_REQUEST_NULL;
  xbt_mutex_acquire(send_win->atomic_mut_);
  get(result_addr, 1, datatype, target_rank,
              target_disp, 1, datatype, &req);
  if (req != MPI_REQUEST_NULL)
    Request::wait(&req, MPI_STATUS_IGNORE);
  if (not memcmp(result_addr, compare_addr, datatype->get_extent())) {
    put(origin_addr, 1, datatype, target_rank,
              target_disp, 1, datatype);
  }
  xbt_mutex_release(send_win->atomic_mut_);
  return MPI_SUCCESS;
}

int Win::start(MPI_Group group, int assert){
    /* From MPI forum advices
    The call to MPI_WIN_COMPLETE does not return until the put call has completed at the origin; and the target window
    will be accessed by the put operation only after the call to MPI_WIN_START has matched a call to MPI_WIN_POST by
    the target process. This still leaves much choice to implementors. The call to MPI_WIN_START can block until the
    matching call to MPI_WIN_POST occurs at all target processes. One can also have implementations where the call to
    MPI_WIN_START is nonblocking, but the call to MPI_PUT blocks until the matching call to MPI_WIN_POST occurred; or
    implementations where the first two calls are nonblocking, but the call to MPI_WIN_COMPLETE blocks until the call
    to MPI_WIN_POST occurred; or even implementations where all three calls can complete before any target process
    called MPI_WIN_POST --- the data put must be buffered, in this last case, so as to allow the put to complete at the
    origin ahead of its completion at the target. However, once the call to MPI_WIN_POST is issued, the sequence above
    must complete, without further dependencies.  */

  //naive, blocking implementation.
    int i             = 0;
    int j             = 0;
    int size          = group->size();
    MPI_Request* reqs = xbt_new0(MPI_Request, size);

    while (j != size) {
      int src = group->index(j);
      if (src != smpi_process()->index() && src != MPI_UNDEFINED) {
        reqs[i] = Request::irecv_init(nullptr, 0, MPI_CHAR, src, SMPI_RMA_TAG + 4, MPI_COMM_WORLD);
        i++;
      }
      j++;
  }
  size=i;
  Request::startall(size, reqs);
  Request::waitall(size, reqs, MPI_STATUSES_IGNORE);
  for(i=0;i<size;i++){
    Request::unref(&reqs[i]);
  }
  xbt_free(reqs);
  opened_++; //we're open for business !
  group_=group;
  group->ref();
  return MPI_SUCCESS;
}

int Win::post(MPI_Group group, int assert){
  //let's make a synchronous send here
  int i             = 0;
  int j             = 0;
  int size = group->size();
  MPI_Request* reqs = xbt_new0(MPI_Request, size);

  while(j!=size){
    int dst=group->index(j);
    if(dst!=smpi_process()->index() && dst!=MPI_UNDEFINED){
      reqs[i]=Request::send_init(nullptr, 0, MPI_CHAR, dst, SMPI_RMA_TAG+4, MPI_COMM_WORLD);
      i++;
    }
    j++;
  }
  size=i;

  Request::startall(size, reqs);
  Request::waitall(size, reqs, MPI_STATUSES_IGNORE);
  for(i=0;i<size;i++){
    Request::unref(&reqs[i]);
  }
  xbt_free(reqs);
  opened_++; //we're open for business !
  group_=group;
  group->ref();
  return MPI_SUCCESS;
}

int Win::complete(){
  if(opened_==0)
    xbt_die("Complete called on already opened MPI_Win");

  XBT_DEBUG("Entering MPI_Win_Complete");
  int i             = 0;
  int j             = 0;
  int size = group_->size();
  MPI_Request* reqs = xbt_new0(MPI_Request, size);

  while(j!=size){
    int dst=group_->index(j);
    if(dst!=smpi_process()->index() && dst!=MPI_UNDEFINED){
      reqs[i]=Request::send_init(nullptr, 0, MPI_CHAR, dst, SMPI_RMA_TAG+5, MPI_COMM_WORLD);
      i++;
    }
    j++;
  }
  size=i;
  XBT_DEBUG("Win_complete - Sending sync messages to %d processes", size);
  Request::startall(size, reqs);
  Request::waitall(size, reqs, MPI_STATUSES_IGNORE);

  for(i=0;i<size;i++){
    Request::unref(&reqs[i]);
  }
  xbt_free(reqs);

  int finished = finish_comms();
  XBT_DEBUG("Win_complete - Finished %d RMA calls", finished);

  Group::unref(group_);
  opened_--; //we're closed for business !
  return MPI_SUCCESS;
}

int Win::wait(){
  //naive, blocking implementation.
  XBT_DEBUG("Entering MPI_Win_Wait");
  int i             = 0;
  int j             = 0;
  int size          = group_->size();
  MPI_Request* reqs = xbt_new0(MPI_Request, size);

  while(j!=size){
    int src=group_->index(j);
    if(src!=smpi_process()->index() && src!=MPI_UNDEFINED){
      reqs[i]=Request::irecv_init(nullptr, 0, MPI_CHAR, src,SMPI_RMA_TAG+5, MPI_COMM_WORLD);
      i++;
    }
    j++;
  }
  size=i;
  XBT_DEBUG("Win_wait - Receiving sync messages from %d processes", size);
  Request::startall(size, reqs);
  Request::waitall(size, reqs, MPI_STATUSES_IGNORE);
  for(i=0;i<size;i++){
    Request::unref(&reqs[i]);
  }
  xbt_free(reqs);
  int finished = finish_comms();
  XBT_DEBUG("Win_wait - Finished %d RMA calls", finished);

  Group::unref(group_);
  opened_--; //we're opened for business !
  return MPI_SUCCESS;
}

int Win::lock(int lock_type, int rank, int assert){
  MPI_Win target_win = connected_wins_[rank];

  if ((lock_type == MPI_LOCK_EXCLUSIVE && target_win->mode_ != MPI_LOCK_SHARED)|| target_win->mode_ == MPI_LOCK_EXCLUSIVE){
    xbt_mutex_acquire(target_win->lock_mut_);
    target_win->mode_+= lock_type;//add the lock_type to differentiate case when we are switching from EXCLUSIVE to SHARED (no release needed in the unlock)
    if(lock_type == MPI_LOCK_SHARED){//the window used to be exclusive, it's now shared.
      xbt_mutex_release(target_win->lock_mut_);
   }
  } else if (not(target_win->mode_ == MPI_LOCK_SHARED && lock_type == MPI_LOCK_EXCLUSIVE))
    target_win->mode_ += lock_type; // don't set to exclusive if it's already shared

  target_win->lockers_.push_back(comm_->rank());

  int finished = finish_comms(rank);
  XBT_DEBUG("Win_lock %d - Finished %d RMA calls", rank, finished);
  finished = target_win->finish_comms(rank_);
  XBT_DEBUG("Win_lock target %d - Finished %d RMA calls", rank, finished);
  return MPI_SUCCESS;
}

int Win::lock_all(int assert){
  int i=0;
  int retval = MPI_SUCCESS;
  for (i=0; i<comm_->size();i++){
      int ret = this->lock(MPI_LOCK_SHARED, i, assert);
      if(ret != MPI_SUCCESS)
        retval = ret;
  }
  return retval;
}

int Win::unlock(int rank){
  MPI_Win target_win = connected_wins_[rank];
  int target_mode = target_win->mode_;
  target_win->mode_= 0;
  target_win->lockers_.remove(comm_->rank());
  if (target_mode==MPI_LOCK_EXCLUSIVE){
    xbt_mutex_release(target_win->lock_mut_);
  }

  int finished = finish_comms(rank);
  XBT_DEBUG("Win_unlock %d - Finished %d RMA calls", rank, finished);
  finished = target_win->finish_comms(rank_);
  XBT_DEBUG("Win_unlock target %d - Finished %d RMA calls", rank, finished);
  return MPI_SUCCESS;
}

int Win::unlock_all(){
  int i=0;
  int retval = MPI_SUCCESS;
  for (i=0; i<comm_->size();i++){
      int ret = this->unlock(i);
      if(ret != MPI_SUCCESS)
        retval = ret;
  }
  return retval;
}

int Win::flush(int rank){
  MPI_Win target_win = connected_wins_[rank];
  int finished = finish_comms(rank);
  XBT_DEBUG("Win_flush on local %d - Finished %d RMA calls", rank_, finished);
  finished = target_win->finish_comms(rank_);
  XBT_DEBUG("Win_flush on remote %d - Finished %d RMA calls", rank, finished);
  return MPI_SUCCESS;
}

int Win::flush_local(int rank){
  int finished = finish_comms(rank);
  XBT_DEBUG("Win_flush_local for rank %d - Finished %d RMA calls", rank, finished);
  return MPI_SUCCESS;
}

int Win::flush_all(){
  int i=0;
  int finished = 0;
  finished = finish_comms();
  XBT_DEBUG("Win_flush_all on local - Finished %d RMA calls", finished);
  for (i=0; i<comm_->size();i++){
    finished = connected_wins_[i]->finish_comms(rank_);
    XBT_DEBUG("Win_flush_all on %d - Finished %d RMA calls", i, finished);
  }
  return MPI_SUCCESS;
}

int Win::flush_local_all(){
  int finished = finish_comms();
  XBT_DEBUG("Win_flush_local_all - Finished %d RMA calls", finished);
  return MPI_SUCCESS;
}

Win* Win::f2c(int id){
  return static_cast<Win*>(F2C::f2c(id));
}


int Win::finish_comms(){
  xbt_mutex_acquire(mut_);
  //Finish own requests
  std::vector<MPI_Request> *reqqs = requests_;
  int size = static_cast<int>(reqqs->size());
  if (size > 0) {
    MPI_Request* treqs = &(*reqqs)[0];
    Request::waitall(size, treqs, MPI_STATUSES_IGNORE);
    reqqs->clear();
  }
  xbt_mutex_release(mut_);
  return size;
}

int Win::finish_comms(int rank){
  xbt_mutex_acquire(mut_);
  //Finish own requests
  std::vector<MPI_Request> *reqqs = requests_;
  int size = static_cast<int>(reqqs->size());
  if (size > 0) {
    size = 0;
    std::vector<MPI_Request> myreqqs;
    std::vector<MPI_Request>::iterator iter = reqqs->begin();
    while (iter != reqqs->end()){
      if(((*iter)!=MPI_REQUEST_NULL) && (((*iter)->src() == rank) || ((*iter)->dst() == rank))){
        myreqqs.push_back(*iter);
        iter = reqqs->erase(iter);
        size++;
      } else {
        ++iter;
      }
    }
    if(size >0){
      MPI_Request* treqs = &myreqqs[0];
      Request::waitall(size, treqs, MPI_STATUSES_IGNORE);
      myreqqs.clear();
    }
  }
  xbt_mutex_release(mut_);
  return size;
}


}
}

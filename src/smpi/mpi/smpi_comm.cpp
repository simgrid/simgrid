/* Copyright (c) 2010-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_comm.hpp"
#include "smpi_coll.hpp"
#include "smpi_datatype.hpp"
#include "smpi_request.hpp"
#include "smpi_win.hpp"
#include "smpi_info.hpp"
#include "src/smpi/include/smpi_actor.hpp"
#include "src/surf/HostImpl.hpp"

#include <climits>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_comm, smpi, "Logging specific to SMPI (comm)");

simgrid::smpi::Comm mpi_MPI_COMM_UNINITIALIZED;
MPI_Comm MPI_COMM_UNINITIALIZED=&mpi_MPI_COMM_UNINITIALIZED;

/* Support for cartesian topology was added, but there are 2 other types of topology, graph et dist graph. In order to
 * support them, we have to add a field SMPI_Topo_type, and replace the MPI_Topology field by an union. */

namespace simgrid{
namespace smpi{

std::unordered_map<int, smpi_key_elem> Comm::keyvals_;
int Comm::keyval_id_=0;

Comm::Comm(MPI_Group group, MPI_Topology topo, int smp, int in_id) : group_(group), topo_(topo),is_smp_comm_(smp), id_(in_id)
{
  refcount_        = 1;
  topoType_        = MPI_INVALID_TOPO;
  intra_comm_      = MPI_COMM_NULL;
  leaders_comm_    = MPI_COMM_NULL;
  is_uniform_      = 1;
  non_uniform_map_ = nullptr;
  leaders_map_     = nullptr;
  is_blocked_      = 0;
  info_            = MPI_INFO_NULL;
  errhandler_      = MPI_ERRORS_ARE_FATAL;
  //First creation of comm is done before SIMIX_run, so only do comms for others
  if(in_id==MPI_UNDEFINED && smp==0 && this->rank()!=MPI_UNDEFINED ){
    int id;
    if(this->rank()==0){
      static int global_id_ = 0;
      id=global_id_;
      global_id_++;
    }
    Colls::bcast(&id, 1, MPI_INT, 0, this);
    XBT_DEBUG("Communicator %p has id %d", this, id);
    id_=id;//only set here, as we don't want to change it in the middle of the bcast
    Colls::barrier(this);
  }
}

void Comm::destroy(Comm* comm)
{
  if (comm == MPI_COMM_UNINITIALIZED){
    Comm::destroy(smpi_process()->comm_world());
    return;
  }
  delete comm->topo_; // there's no use count on topos
  Comm::unref(comm);
}

int Comm::dup(MPI_Comm* newcomm){
  if (smpi_privatize_global_variables == SmpiPrivStrategies::MMAP) {
    // we need to switch as the called function may silently touch global variables
    smpi_switch_data_segment(s4u::Actor::self());
  }
  MPI_Group cp = new  Group(this->group());
  (*newcomm)   = new  Comm(cp, this->topo());
  int ret      = MPI_SUCCESS;

  if (not attributes()->empty()) {
    int flag=0;
    void* value_out=nullptr;
    for (auto const& it : *attributes()) {
      smpi_key_elem elem = keyvals_.at(it.first);
      if (elem != nullptr){
        if( elem->copy_fn.comm_copy_fn != MPI_NULL_COPY_FN && 
            elem->copy_fn.comm_copy_fn != MPI_COMM_DUP_FN)
          ret = elem->copy_fn.comm_copy_fn(this, it.first, elem->extra_state, it.second, &value_out, &flag);
        else if ( elem->copy_fn.comm_copy_fn_fort != MPI_NULL_COPY_FN &&
                  *(int*)*elem->copy_fn.comm_copy_fn_fort != 1){
          value_out=(int*)xbt_malloc(sizeof(int));
          elem->copy_fn.comm_copy_fn_fort(this, it.first, elem->extra_state, it.second, value_out, &flag,&ret);
        }
        if (ret != MPI_SUCCESS) {
          Comm::destroy(*newcomm);
          *newcomm = MPI_COMM_NULL;
          return ret;
        }
        if (elem->copy_fn.comm_copy_fn == MPI_COMM_DUP_FN || 
           ((elem->copy_fn.comm_copy_fn_fort != MPI_NULL_COPY_FN) && *(int*)*elem->copy_fn.comm_copy_fn_fort == 1)){
          elem->refcount++;
          (*newcomm)->attributes()->insert({it.first, it.second});
        }else if (flag){
          elem->refcount++;
          (*newcomm)->attributes()->insert({it.first, value_out});
        }
      }
    }
  }
  //duplicate info if present
  if(info_!=MPI_INFO_NULL)
    (*newcomm)->info_ = new simgrid::smpi::Info(info_);
  //duplicate errhandler
  (*newcomm)->set_errhandler(errhandler_);
  return ret;
}

int Comm::dup_with_info(MPI_Info info, MPI_Comm* newcomm){
  int ret = dup(newcomm);
  if(ret != MPI_SUCCESS)
    return ret;
  if((*newcomm)->info_!=MPI_INFO_NULL){
    simgrid::smpi::Info::unref((*newcomm)->info_);
    (*newcomm)->info_=MPI_INFO_NULL;
  }
  if(info != MPI_INFO_NULL){
    (*newcomm)->info_=info;
  }
  return ret;
}

MPI_Group Comm::group()
{
  if (this == MPI_COMM_UNINITIALIZED)
    return smpi_process()->comm_world()->group();
  return group_;
}

int Comm::size()
{
  if (this == MPI_COMM_UNINITIALIZED)
    return smpi_process()->comm_world()->size();
  return group_->size();
}

int Comm::rank()
{
  if (this == MPI_COMM_UNINITIALIZED)
    return smpi_process()->comm_world()->rank();
  return group_->rank(s4u::Actor::self());
}

int Comm::id()
{
  return id_;
}

void Comm::get_name (char* name, int* len)
{
  if (this == MPI_COMM_UNINITIALIZED){
    smpi_process()->comm_world()->get_name(name, len);
    return;
  }
  if(this == MPI_COMM_WORLD && name_.empty()) {
    strncpy(name, "MPI_COMM_WORLD", 15);
    *len = 14;
  } else if(this == MPI_COMM_SELF && name_.empty()) {
    strncpy(name, "MPI_COMM_SELF", 14);
    *len = 13;
  } else {
    *len = snprintf(name, MPI_MAX_NAME_STRING+1, "%s", name_.c_str());
  }
}

void Comm::set_name (const char* name)
{
  if (this == MPI_COMM_UNINITIALIZED){
    smpi_process()->comm_world()->set_name(name);
    return;
  }
  name_.replace (0, MPI_MAX_NAME_STRING+1, name);
}


void Comm::set_leaders_comm(MPI_Comm leaders){
  if (this == MPI_COMM_UNINITIALIZED){
    smpi_process()->comm_world()->set_leaders_comm(leaders);
    return;
  }
  leaders_comm_=leaders;
}

int* Comm::get_non_uniform_map(){
  if (this == MPI_COMM_UNINITIALIZED)
    return smpi_process()->comm_world()->get_non_uniform_map();
  return non_uniform_map_;
}

int* Comm::get_leaders_map(){
  if (this == MPI_COMM_UNINITIALIZED)
    return smpi_process()->comm_world()->get_leaders_map();
  return leaders_map_;
}

MPI_Comm Comm::get_leaders_comm(){
  if (this == MPI_COMM_UNINITIALIZED)
    return smpi_process()->comm_world()->get_leaders_comm();
  return leaders_comm_;
}

MPI_Comm Comm::get_intra_comm(){
  if (this == MPI_COMM_UNINITIALIZED || this==MPI_COMM_WORLD)
    return smpi_process()->comm_intra();
  else return intra_comm_;
}

int Comm::is_uniform(){
  if (this == MPI_COMM_UNINITIALIZED)
    return smpi_process()->comm_world()->is_uniform();
  return is_uniform_;
}

int Comm::is_blocked(){
  if (this == MPI_COMM_UNINITIALIZED)
    return smpi_process()->comm_world()->is_blocked();
  return is_blocked_;
}

int Comm::is_smp_comm(){
  if (this == MPI_COMM_UNINITIALIZED)
    return smpi_process()->comm_world()->is_smp_comm();
  return is_smp_comm_;
}

MPI_Comm Comm::split(int color, int key)
{
  if (this == MPI_COMM_UNINITIALIZED)
    return smpi_process()->comm_world()->split(color, key);
  int system_tag = -123;
  int* recvbuf;

  MPI_Group group_root = nullptr;
  MPI_Group group_out  = nullptr;
  MPI_Group group      = this->group();
  int myrank           = this->rank();
  int size             = this->size();
  /* Gather all colors and keys on rank 0 */
  int* sendbuf = xbt_new(int, 2);
  sendbuf[0] = color;
  sendbuf[1] = key;
  if (myrank == 0) {
    recvbuf = xbt_new(int, 2 * size);
  } else {
    recvbuf = nullptr;
  }
  Coll_gather_default::gather(sendbuf, 2, MPI_INT, recvbuf, 2, MPI_INT, 0, this);
  xbt_free(sendbuf);
  /* Do the actual job */
  if (myrank == 0) {
    MPI_Group* group_snd = xbt_new(MPI_Group, size);
    std::vector<std::pair<int, int>> rankmap;
    rankmap.reserve(size);
    for (int i = 0; i < size; i++) {
      if (recvbuf[2 * i] != MPI_UNDEFINED) {
        rankmap.clear();
        for (int j = i + 1; j < size; j++) {
          if(recvbuf[2 * i] == recvbuf[2 * j]) {
            recvbuf[2 * j] = MPI_UNDEFINED;
            rankmap.push_back({recvbuf[2 * j + 1], j});
          }
        }
        /* Add self in the group */
        recvbuf[2 * i] = MPI_UNDEFINED;
        rankmap.push_back({recvbuf[2 * i + 1], i});
        std::sort(begin(rankmap), end(rankmap));
        group_out = new Group(rankmap.size());
        if (i == 0) {
          group_root = group_out; /* Save root's group */
        }
        for (unsigned j = 0; j < rankmap.size(); j++) {
          s4u::Actor* actor = group->actor(rankmap[j].second);
          group_out->set_mapping(actor, j);
        }
        MPI_Request* requests = xbt_new(MPI_Request, rankmap.size());
        int reqs              = 0;
        for (auto const& rank : rankmap) {
          if (rank.second != 0) {
            group_snd[reqs]=new  Group(group_out);
            requests[reqs] = Request::isend(&(group_snd[reqs]), 1, MPI_PTR, rank.second, system_tag, this);
            reqs++;
          }
        }
        if(i != 0 && group_out != MPI_COMM_WORLD->group() && group_out != MPI_GROUP_EMPTY)
          Group::unref(group_out);

        Request::waitall(reqs, requests, MPI_STATUS_IGNORE);
        xbt_free(requests);
      }
    }
    xbt_free(recvbuf);
    xbt_free(group_snd);
    group_out = group_root; /* exit with root's group */
  } else {
    if(color != MPI_UNDEFINED) {
      Request::recv(&group_out, 1, MPI_PTR, 0, system_tag, this, MPI_STATUS_IGNORE);
    } /* otherwise, exit with group_out == nullptr */
  }
  return group_out!=nullptr ? new  Comm(group_out, nullptr) : MPI_COMM_NULL;
}

void Comm::ref(){
  if (this == MPI_COMM_UNINITIALIZED){
    smpi_process()->comm_world()->ref();
    return;
  }
  group_->ref();
  refcount_++;
}

void Comm::cleanup_smp(){
  if (intra_comm_ != MPI_COMM_NULL)
    Comm::unref(intra_comm_);
  if (leaders_comm_ != MPI_COMM_NULL)
    Comm::unref(leaders_comm_);
  xbt_free(non_uniform_map_);
  delete[] leaders_map_;
}

void Comm::unref(Comm* comm){
  if (comm == MPI_COMM_UNINITIALIZED){
    Comm::unref(smpi_process()->comm_world());
    return;
  }
  comm->refcount_--;
  Group::unref(comm->group_);

  if(comm->refcount_==0){
    comm->cleanup_smp();
    comm->cleanup_attr<Comm>();
    delete comm;
  }
}

MPI_Comm Comm::find_intra_comm(int * leader){
  //get the indices of all processes sharing the same simix host
  auto& actor_list        = sg_host_self()->pimpl_->actor_list_;
  int intra_comm_size     = 0;
  int min_index           = INT_MAX; // the minimum index will be the leader
  for (auto& actor : actor_list) {
    int index = actor.get_pid();
    if (this->group()->rank(actor.ciface()) != MPI_UNDEFINED) { // Is this process in the current group?
      intra_comm_size++;
      if (index < min_index)
        min_index = index;
    }
  }
  XBT_DEBUG("number of processes deployed on my node : %d", intra_comm_size);
  MPI_Group group_intra = new  Group(intra_comm_size);
  int i = 0;
  for (auto& actor : actor_list) {
    if (this->group()->rank(actor.ciface()) != MPI_UNDEFINED) {
      group_intra->set_mapping(actor.ciface(), i);
      i++;
    }
  }
  *leader=min_index;
  return new  Comm(group_intra, nullptr, 1);
}

void Comm::init_smp(){
  int leader = -1;
  int i = 0;
  if (this == MPI_COMM_UNINITIALIZED)
    smpi_process()->comm_world()->init_smp();

  int comm_size = this->size();

  // If we are in replay - perform an ugly hack
  // tell SimGrid we are not in replay for a while, because we need the buffers to be copied for the following calls
  bool replaying = false; //cache data to set it back again after
  if(smpi_process()->replaying()){
    replaying = true;
    smpi_process()->set_replaying(false);
  }

  if (smpi_privatize_global_variables == SmpiPrivStrategies::MMAP) {
    // we need to switch as the called function may silently touch global variables
    smpi_switch_data_segment(s4u::Actor::self());
  }
  // identify neighbors in comm
  MPI_Comm comm_intra = find_intra_comm(&leader);


  int* leaders_map = new int[comm_size];
  int* leader_list = new int[comm_size];
  std::fill_n(leaders_map, comm_size, 0);
  std::fill_n(leader_list, comm_size, -1);

  Coll_allgather_ring::allgather(&leader, 1, MPI_INT , leaders_map, 1, MPI_INT, this);

  if (smpi_privatize_global_variables == SmpiPrivStrategies::MMAP) {
    // we need to switch as the called function may silently touch global variables
    smpi_switch_data_segment(s4u::Actor::self());
  }

  if(leaders_map_==nullptr){
    leaders_map_= leaders_map;
  }else{
    delete[] leaders_map;
  }
  int leader_group_size = 0;
  for(i=0; i<comm_size; i++){
    int already_done = 0;
    for (int j = 0; j < leader_group_size; j++) {
      if (leaders_map_[i] == leader_list[j]) {
        already_done = 1;
      }
    }
    if (already_done == 0) {
      leader_list[leader_group_size] = leaders_map_[i];
      leader_group_size++;
    }
  }
  std::sort(leader_list, leader_list + leader_group_size);

  MPI_Group leaders_group = new  Group(leader_group_size);

  MPI_Comm leader_comm = MPI_COMM_NULL;
  if(MPI_COMM_WORLD!=MPI_COMM_UNINITIALIZED && this!=MPI_COMM_WORLD){
    //create leader_communicator
    for (i=0; i< leader_group_size;i++)
      leaders_group->set_mapping(s4u::Actor::by_pid(leader_list[i]).get(), i);
    leader_comm = new  Comm(leaders_group, nullptr,1);
    this->set_leaders_comm(leader_comm);
    this->set_intra_comm(comm_intra);

    // create intracommunicator
  }else{
    for (i=0; i< leader_group_size;i++)
      leaders_group->set_mapping(s4u::Actor::by_pid(leader_list[i]).get(), i);

    if(this->get_leaders_comm()==MPI_COMM_NULL){
      leader_comm = new  Comm(leaders_group, nullptr,1);
      this->set_leaders_comm(leader_comm);
    }else{
      leader_comm=this->get_leaders_comm();
      Group::unref(leaders_group);
    }
    smpi_process()->set_comm_intra(comm_intra);
  }

  // Are the nodes uniform ? = same number of process/node
  int my_local_size=comm_intra->size();
  if(comm_intra->rank()==0) {
    int is_uniform       = 1;
    int* non_uniform_map = xbt_new0(int,leader_group_size);
    Coll_allgather_ring::allgather(&my_local_size, 1, MPI_INT,
        non_uniform_map, 1, MPI_INT, leader_comm);
    for(i=0; i < leader_group_size; i++) {
      if(non_uniform_map[0] != non_uniform_map[i]) {
        is_uniform = 0;
        break;
      }
    }
    if(is_uniform==0 && this->is_uniform()!=0){
      non_uniform_map_ = non_uniform_map;
    }else{
      xbt_free(non_uniform_map);
    }
    is_uniform_=is_uniform;
  }
  Coll_bcast_scatter_LR_allgather::bcast(&(is_uniform_),1, MPI_INT, 0, comm_intra );

  if (smpi_privatize_global_variables == SmpiPrivStrategies::MMAP) {
    // we need to switch as the called function may silently touch global variables
    smpi_switch_data_segment(s4u::Actor::self());
  }
  // Are the ranks blocked ? = allocated contiguously on the SMP nodes
  int is_blocked=1;
  int prev=this->group()->rank(comm_intra->group()->actor(0));
  for (i = 1; i < my_local_size; i++) {
    int that = this->group()->rank(comm_intra->group()->actor(i));
    if (that != prev + 1) {
      is_blocked = 0;
      break;
    }
    prev = that;
  }

  int global_blocked;
  Coll_allreduce_default::allreduce(&is_blocked, &(global_blocked), 1, MPI_INT, MPI_LAND, this);

  if(MPI_COMM_WORLD==MPI_COMM_UNINITIALIZED || this==MPI_COMM_WORLD){
    if(this->rank()==0){
      is_blocked_ = global_blocked;
    }
  }else{
    is_blocked_=global_blocked;
  }
  delete[] leader_list;

  if(replaying)
    smpi_process()->set_replaying(true);
}

MPI_Comm Comm::f2c(int id) {
  if(id == -2) {
    return MPI_COMM_SELF;
  } else if(id==0){
    return MPI_COMM_WORLD;
  } else if(F2C::f2c_lookup() != nullptr && id >= 0) {
    char key[KEY_SIZE];
    const auto& lookup = F2C::f2c_lookup();
    auto comm          = lookup->find(get_key(key, id));
    return comm == lookup->end() ? MPI_COMM_NULL : static_cast<MPI_Comm>(comm->second);
  } else {
    return MPI_COMM_NULL;
  }
}

void Comm::free_f(int id) {
  char key[KEY_SIZE];
  F2C::f2c_lookup()->erase(get_key(key, id));
}

void Comm::add_rma_win(MPI_Win win){
  rma_wins_.push_back(win);
}

void Comm::remove_rma_win(MPI_Win win){
  rma_wins_.remove(win);
}

void Comm::finish_rma_calls(){
  for (auto const& it : rma_wins_) {
    if(it->rank()==this->rank()){//is it ours (for MPI_COMM_WORLD)?
      int finished = it->finish_comms();
      XBT_DEBUG("Barrier for rank %d - Finished %d RMA calls",this->rank(), finished);
    }
  }
}

MPI_Info Comm::info(){
  if(info_== MPI_INFO_NULL)
    info_ = new Info();
  info_->ref();
  return info_;
}

void Comm::set_info(MPI_Info info){
  if(info_!= MPI_INFO_NULL)
    info->ref();
  info_=info;
}

MPI_Errhandler Comm::errhandler(){
  return errhandler_;
}

void Comm::set_errhandler(MPI_Errhandler errhandler){
  errhandler_=errhandler;
  if(errhandler_!= MPI_ERRHANDLER_NULL)
    errhandler->ref();
}

MPI_Comm Comm::split_type(int type, int /*key*/, MPI_Info)
{
  //MPI_UNDEFINED can be given to some nodes... but we need them to still perform the smp part which is collective
  if(type != MPI_COMM_TYPE_SHARED && type != MPI_UNDEFINED){
    return MPI_COMM_NULL;
  }
  int leader=0;
  MPI_Comm res= this->find_intra_comm(&leader);
  if(type != MPI_UNDEFINED)
    return res;
  else{
    Comm::destroy(res);
    return MPI_COMM_NULL;
  }
}

} // namespace smpi
} // namespace simgrid

/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <limits.h>

#include <xbt/dict.h>
#include <xbt/ex.h>
#include <xbt/ex.hpp>

#include <simgrid/s4u/host.hpp>

#include "private.h"
#include "smpi_mpi_dt_private.h"
#include "src/simix/smx_private.h"
#include "colls/colls.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_comm, smpi, "Logging specific to SMPI (comm)");

xbt_dict_t smpi_comm_keyvals = nullptr;
int comm_keyval_id = 0;//avoid collisions


simgrid::smpi::Comm mpi_MPI_COMM_UNINITIALIZED;
MPI_Comm MPI_COMM_UNINITIALIZED=&mpi_MPI_COMM_UNINITIALIZED;

/* Support for cartesian topology was added, but there are 2 other types of topology, graph et dist graph. In order to
 * support them, we have to add a field MPIR_Topo_type, and replace the MPI_Topology field by an union. */

static int smpi_compare_rankmap(const void *a, const void *b)
{
  const int* x = static_cast<const int*>(a);
  const int* y = static_cast<const int*>(b);

  if (x[1] < y[1]) {
    return -1;
  }
  if (x[1] == y[1]) {
    if (x[0] < y[0]) {
      return -1;
    }
    if (x[0] == y[0]) {
      return 0;
    }
    return 1;
  }
  return 1;
}

namespace simgrid{
namespace smpi{

Comm::Comm(){}

Comm::Comm(MPI_Group group, MPI_Topology topo) : group_(group), topo_(topo)
{
  refcount_=1;
  topoType_ = MPI_INVALID_TOPO;
  intra_comm_ = MPI_COMM_NULL;
  leaders_comm_ = MPI_COMM_NULL;
  is_uniform_=1;
  non_uniform_map_ = nullptr;
  leaders_map_ = nullptr;
  is_blocked_=0;
  attributes_=nullptr;
}

void Comm::destroy()
{
  if (this == MPI_COMM_UNINITIALIZED){
    smpi_process_comm_world()->destroy();
    return;
  }
  delete topo_; // there's no use count on topos
  this->unuse();
}

int Comm::dup(MPI_Comm* newcomm){
  if(smpi_privatize_global_variables){ //we need to switch as the called function may silently touch global variables
     smpi_switch_data_segment(smpi_process_index());
   }
  MPI_Group cp = new simgrid::smpi::Group(this->group());
  (*newcomm) = new simgrid::smpi::Comm(cp, this->topo());
  int ret = MPI_SUCCESS;

  if(attributes_ !=nullptr){
    (*newcomm)->attributes_   = xbt_dict_new_homogeneous(nullptr);
    xbt_dict_cursor_t cursor = nullptr;
    char* key;
    int flag;
    void* value_in;
    void* value_out;
    xbt_dict_foreach (attributes_, cursor, key, value_in) {
      smpi_comm_key_elem elem =
          static_cast<smpi_comm_key_elem>(xbt_dict_get_or_null_ext(smpi_comm_keyvals, key, sizeof(int)));
      if (elem != nullptr && elem->copy_fn != MPI_NULL_COPY_FN) {
        ret = elem->copy_fn(this, atoi(key), nullptr, value_in, &value_out, &flag);
        if (ret != MPI_SUCCESS) {
          (*newcomm)->destroy();
          *newcomm = MPI_COMM_NULL;
          xbt_dict_cursor_free(&cursor);
          return ret;
        }
        if (flag)
          xbt_dict_set_ext((*newcomm)->attributes_, key, sizeof(int), value_out, nullptr);
      }
      }
    }
  return ret;
}

MPI_Group Comm::group()
{
  if (this == MPI_COMM_UNINITIALIZED)
    return smpi_process_comm_world()->group();
  return group_;
}

MPI_Topology Comm::topo() {
  return topo_;
}

int Comm::size()
{
  if (this == MPI_COMM_UNINITIALIZED)
    return smpi_process_comm_world()->size();
  return group_->size();
}

int Comm::rank()
{
  if (this == MPI_COMM_UNINITIALIZED)
    return smpi_process_comm_world()->rank();
  return group_->rank(smpi_process_index());
}

void Comm::get_name (char* name, int* len)
{
  if (this == MPI_COMM_UNINITIALIZED){
    smpi_process_comm_world()->get_name(name, len);
    return;
  }
  if(this == MPI_COMM_WORLD) {
    strncpy(name, "WORLD",5);
    *len = 5;
  } else {
    *len = snprintf(name, MPI_MAX_NAME_STRING, "%p", this);
  }
}

void Comm::set_leaders_comm(MPI_Comm leaders){
  if (this == MPI_COMM_UNINITIALIZED){
    smpi_process_comm_world()->set_leaders_comm(leaders);
    return;
  }
  leaders_comm_=leaders;
}

void Comm::set_intra_comm(MPI_Comm leaders){
  intra_comm_=leaders;
}

int* Comm::get_non_uniform_map(){
  if (this == MPI_COMM_UNINITIALIZED)
    return smpi_process_comm_world()->get_non_uniform_map();
  return non_uniform_map_;
}

int* Comm::get_leaders_map(){
  if (this == MPI_COMM_UNINITIALIZED)
    return smpi_process_comm_world()->get_leaders_map();
  return leaders_map_;
}

MPI_Comm Comm::get_leaders_comm(){
  if (this == MPI_COMM_UNINITIALIZED)
    return smpi_process_comm_world()->get_leaders_comm();
  return leaders_comm_;
}

MPI_Comm Comm::get_intra_comm(){
  if (this == MPI_COMM_UNINITIALIZED || this==MPI_COMM_WORLD) 
    return smpi_process_get_comm_intra();
  else return intra_comm_;
}

int Comm::is_uniform(){
  if (this == MPI_COMM_UNINITIALIZED)
    return smpi_process_comm_world()->is_uniform();
  return is_uniform_;
}

int Comm::is_blocked(){
  if (this == MPI_COMM_UNINITIALIZED)
    return smpi_process_comm_world()->is_blocked();
  return is_blocked_;
}

MPI_Comm Comm::split(int color, int key)
{
  if (this == MPI_COMM_UNINITIALIZED)
    return smpi_process_comm_world()->split(color, key);
  int system_tag = 123;
  int* recvbuf;

  MPI_Group group_root = nullptr;
  MPI_Group group_out  = nullptr;
  MPI_Group group      = this->group();
  int rank             = this->rank();
  int size             = this->size();
  /* Gather all colors and keys on rank 0 */
  int* sendbuf = xbt_new(int, 2);
  sendbuf[0] = color;
  sendbuf[1] = key;
  if(rank == 0) {
    recvbuf = xbt_new(int, 2 * size);
  } else {
    recvbuf = nullptr;
  }
  smpi_mpi_gather(sendbuf, 2, MPI_INT, recvbuf, 2, MPI_INT, 0, this);
  xbt_free(sendbuf);
  /* Do the actual job */
  if(rank == 0) {
    MPI_Group* group_snd = xbt_new(MPI_Group, size);
    int* rankmap         = xbt_new(int, 2 * size);
    for (int i = 0; i < size; i++) {
      if (recvbuf[2 * i] != MPI_UNDEFINED) {
        int count = 0;
        for (int j = i + 1; j < size; j++) {
          if(recvbuf[2 * i] == recvbuf[2 * j]) {
            recvbuf[2 * j] = MPI_UNDEFINED;
            rankmap[2 * count] = j;
            rankmap[2 * count + 1] = recvbuf[2 * j + 1];
            count++;
          }
        }
        /* Add self in the group */
        recvbuf[2 * i] = MPI_UNDEFINED;
        rankmap[2 * count] = i;
        rankmap[2 * count + 1] = recvbuf[2 * i + 1];
        count++;
        qsort(rankmap, count, 2 * sizeof(int), &smpi_compare_rankmap);
        group_out = new simgrid::smpi::Group(count);
        if (i == 0) {
          group_root = group_out; /* Save root's group */
        }
        for (int j = 0; j < count; j++) {
          int index = group->index(rankmap[2 * j]);
          group_out->set_mapping(index, j);
        }
        MPI_Request* requests = xbt_new(MPI_Request, count);
        int reqs              = 0;
        for (int j = 0; j < count; j++) {
          if(rankmap[2 * j] != 0) {
            group_snd[reqs]=new simgrid::smpi::Group(group_out);
            requests[reqs] = Request::isend(&(group_snd[reqs]), 1, MPI_PTR, rankmap[2 * j], system_tag, this);
            reqs++;
          }
        }
        if(i != 0) {
          group_out->destroy();
        }
        Request::waitall(reqs, requests, MPI_STATUS_IGNORE);
        xbt_free(requests);
      }
    }
    xbt_free(recvbuf);
    xbt_free(rankmap);
    xbt_free(group_snd);
    group_out = group_root; /* exit with root's group */
  } else {
    if(color != MPI_UNDEFINED) {
      Request::recv(&group_out, 1, MPI_PTR, 0, system_tag, this, MPI_STATUS_IGNORE);
    } /* otherwise, exit with group_out == nullptr */
  }
  return group_out!=nullptr ? new simgrid::smpi::Comm(group_out, nullptr) : MPI_COMM_NULL;
}

void Comm::use(){
  if (this == MPI_COMM_UNINITIALIZED){
    smpi_process_comm_world()->use();
    return;
  }
  group_->use();
  refcount_++;
}

void Comm::cleanup_attributes(){
  if(attributes_ !=nullptr){
    xbt_dict_cursor_t cursor = nullptr;
    char* key;
    void* value;
    int flag;
    xbt_dict_foreach (attributes_, cursor, key, value) {
      smpi_comm_key_elem elem = static_cast<smpi_comm_key_elem>(xbt_dict_get_or_null(smpi_comm_keyvals, key));
      if (elem != nullptr && elem->delete_fn != nullptr)
        elem->delete_fn(this, atoi(key), value, &flag);
    }
    xbt_dict_free(&attributes_);
  }
}

void Comm::cleanup_smp(){
  if (intra_comm_ != MPI_COMM_NULL)
    intra_comm_->unuse();
  if (leaders_comm_ != MPI_COMM_NULL)
    leaders_comm_->unuse();
  if (non_uniform_map_ != nullptr)
    xbt_free(non_uniform_map_);
  if (leaders_map_ != nullptr)
    xbt_free(leaders_map_);
}

void Comm::unuse(){
  if (this == MPI_COMM_UNINITIALIZED){
    smpi_process_comm_world()->unuse();
    return;
  }
  refcount_--;
  group_->unuse();

  if(refcount_==0){
    this->cleanup_smp();
    this->cleanup_attributes();
    delete this;
  }
}

static int compare_ints (const void *a, const void *b)
{
  const int *da = static_cast<const int *>(a);
  const int *db = static_cast<const int *>(b);

  return static_cast<int>(*da > *db) - static_cast<int>(*da < *db);
}

void Comm::init_smp(){
  int leader = -1;

  if (this == MPI_COMM_UNINITIALIZED)
    smpi_process_comm_world()->init_smp();

  int comm_size = this->size();
  
  // If we are in replay - perform an ugly hack  
  // tell SimGrid we are not in replay for a while, because we need the buffers to be copied for the following calls
  bool replaying = false; //cache data to set it back again after
  if(smpi_process_get_replaying()){
   replaying=true;
   smpi_process_set_replaying(false);
  }

  if(smpi_privatize_global_variables){ //we need to switch as the called function may silently touch global variables
     smpi_switch_data_segment(smpi_process_index());
   }
  //identify neighbours in comm
  //get the indexes of all processes sharing the same simix host
  xbt_swag_t process_list = SIMIX_host_self()->processes();
  int intra_comm_size = 0;
  int i =0;
  int min_index=INT_MAX;//the minimum index will be the leader
  smx_actor_t process = nullptr;
  xbt_swag_foreach(process, process_list) {
    int index = process->pid -1;

    if(this->group()->rank(index)!=MPI_UNDEFINED){
        intra_comm_size++;
      //the process is in the comm
      if(index < min_index)
        min_index=index;
      i++;
    }
  }
  XBT_DEBUG("number of processes deployed on my node : %d", intra_comm_size);
  MPI_Group group_intra = new simgrid::smpi::Group(intra_comm_size);
  i=0;
  process = nullptr;
  xbt_swag_foreach(process, process_list) {
    int index = process->pid -1;
    if(this->group()->rank(index)!=MPI_UNDEFINED){
      group_intra->set_mapping(index, i);
      i++;
    }
  }

  MPI_Comm comm_intra = new simgrid::smpi::Comm(group_intra, nullptr);
  leader=min_index;

  int * leaders_map= static_cast<int*>(xbt_malloc0(sizeof(int)*comm_size));
  int * leader_list= static_cast<int*>(xbt_malloc0(sizeof(int)*comm_size));
  for(i=0; i<comm_size; i++){
      leader_list[i]=-1;
  }

  smpi_coll_tuned_allgather_mpich(&leader, 1, MPI_INT , leaders_map, 1, MPI_INT, this);

  if(smpi_privatize_global_variables){ //we need to switch as the called function may silently touch global variables
     smpi_switch_data_segment(smpi_process_index());
   }

  if(leaders_map_==nullptr){
    leaders_map_= leaders_map;
  }else{
    xbt_free(leaders_map);
  }
  int j=0;
  int leader_group_size = 0;
  for(i=0; i<comm_size; i++){
      int already_done=0;
      for(j=0;j<leader_group_size; j++){
        if(leaders_map_[i]==leader_list[j]){
            already_done=1;
        }
      }
      if(already_done==0){
        leader_list[leader_group_size]=leaders_map_[i];
        leader_group_size++;
      }
  }
  qsort(leader_list, leader_group_size, sizeof(int),compare_ints);

  MPI_Group leaders_group = new simgrid::smpi::Group(leader_group_size);

  MPI_Comm leader_comm = MPI_COMM_NULL;
  if(MPI_COMM_WORLD!=MPI_COMM_UNINITIALIZED && this!=MPI_COMM_WORLD){
    //create leader_communicator
    for (i=0; i< leader_group_size;i++)
      leaders_group->set_mapping(leader_list[i], i);
    leader_comm = new simgrid::smpi::Comm(leaders_group, nullptr);
    this->set_leaders_comm(leader_comm);
    this->set_intra_comm(comm_intra);

   //create intracommunicator
  }else{
    for (i=0; i< leader_group_size;i++)
      leaders_group->set_mapping(leader_list[i], i);

    if(this->get_leaders_comm()==MPI_COMM_NULL){
      leader_comm = new simgrid::smpi::Comm(leaders_group, nullptr);
      this->set_leaders_comm(leader_comm);
    }else{
      leader_comm=this->get_leaders_comm();
      leaders_group->unuse();
    }
    smpi_process_set_comm_intra(comm_intra);
  }

  int is_uniform = 1;

  // Are the nodes uniform ? = same number of process/node
  int my_local_size=comm_intra->size();
  if(comm_intra->rank()==0) {
    int* non_uniform_map = xbt_new0(int,leader_group_size);
    smpi_coll_tuned_allgather_mpich(&my_local_size, 1, MPI_INT,
        non_uniform_map, 1, MPI_INT, leader_comm);
    for(i=0; i < leader_group_size; i++) {
      if(non_uniform_map[0] != non_uniform_map[i]) {
        is_uniform = 0;
        break;
      }
    }
    if(is_uniform==0 && this->is_uniform()!=0){
        non_uniform_map_= non_uniform_map;
    }else{
        xbt_free(non_uniform_map);
    }
    is_uniform_=is_uniform;
  }
  smpi_coll_tuned_bcast_mpich(&(is_uniform_),1, MPI_INT, 0, comm_intra );

  if(smpi_privatize_global_variables){ //we need to switch as the called function may silently touch global variables
     smpi_switch_data_segment(smpi_process_index());
   }
  // Are the ranks blocked ? = allocated contiguously on the SMP nodes
  int is_blocked=1;
  int prev=this->group()->rank(comm_intra->group()->index(0));
    for (i=1; i<my_local_size; i++){
      int that=this->group()->rank(comm_intra->group()->index(i));
      if(that!=prev+1){
        is_blocked=0;
        break;
      }
      prev = that;
  }

  int global_blocked;
  smpi_mpi_allreduce(&is_blocked, &(global_blocked), 1, MPI_INT, MPI_LAND, this);

  if(MPI_COMM_WORLD==MPI_COMM_UNINITIALIZED || this==MPI_COMM_WORLD){
    if(this->rank()==0){
        is_blocked_=global_blocked;
    }
  }else{
    is_blocked_=global_blocked;
  }
  xbt_free(leader_list);
  
  if(replaying)
    smpi_process_set_replaying(true); 
}

int Comm::attr_delete(int keyval){
  smpi_comm_key_elem elem =
     static_cast<smpi_comm_key_elem>(xbt_dict_get_or_null_ext(smpi_comm_keyvals, reinterpret_cast<const char*>(&keyval), sizeof(int)));
  if(elem==nullptr)
    return MPI_ERR_ARG;
  if(elem->delete_fn!=MPI_NULL_DELETE_FN){
    void* value = nullptr;
    int flag;
    if(this->attr_get(keyval, &value, &flag)==MPI_SUCCESS){
      int ret = elem->delete_fn(this, keyval, value, &flag);
      if(ret!=MPI_SUCCESS) 
        return ret;
    }
  }
  if(attributes_==nullptr)
    return MPI_ERR_ARG;

  xbt_dict_remove_ext(attributes_, reinterpret_cast<const char*>(&keyval), sizeof(int));
  return MPI_SUCCESS;
}

int Comm::attr_get(int keyval, void* attr_value, int* flag){
  smpi_comm_key_elem elem =
    static_cast<smpi_comm_key_elem>(xbt_dict_get_or_null_ext(smpi_comm_keyvals, reinterpret_cast<const char*>(&keyval), sizeof(int)));
  if(elem==nullptr)
    return MPI_ERR_ARG;
  if(attributes_==nullptr){
    *flag=0;
    return MPI_SUCCESS;
  }
  try {
    *static_cast<void**>(attr_value) =
        xbt_dict_get_ext(attributes_, reinterpret_cast<const char*>(&keyval), sizeof(int));
    *flag=1;
  }
  catch (xbt_ex& ex) {
    *flag=0;
  }
  return MPI_SUCCESS;
}

int Comm::attr_put(int keyval, void* attr_value){
  if(smpi_comm_keyvals==nullptr)
    smpi_comm_keyvals = xbt_dict_new_homogeneous(nullptr);
  smpi_comm_key_elem elem =
    static_cast<smpi_comm_key_elem>(xbt_dict_get_or_null_ext(smpi_comm_keyvals,  reinterpret_cast<const char*>(&keyval), sizeof(int)));
  if(elem==nullptr)
    return MPI_ERR_ARG;
  int flag;
  void* value = nullptr;
  this->attr_get(keyval, &value, &flag);
  if(flag!=0 && elem->delete_fn!=MPI_NULL_DELETE_FN){
    int ret = elem->delete_fn(this, keyval, value, &flag);
    if(ret!=MPI_SUCCESS) 
      return ret;
  }
  if(attributes_==nullptr)
    attributes_ = xbt_dict_new_homogeneous(nullptr);

  xbt_dict_set_ext(attributes_,  reinterpret_cast<const char*>(&keyval), sizeof(int), attr_value, nullptr);
  return MPI_SUCCESS;
}

}
}

int smpi_comm_keyval_create(MPI_Comm_copy_attr_function* copy_fn, MPI_Comm_delete_attr_function* delete_fn, int* keyval,
                            void* extra_state){
  if(smpi_comm_keyvals==nullptr)
    smpi_comm_keyvals = xbt_dict_new_homogeneous(nullptr);

  smpi_comm_key_elem value = static_cast<smpi_comm_key_elem>(xbt_new0(s_smpi_mpi_comm_key_elem_t,1));

  value->copy_fn=copy_fn;
  value->delete_fn=delete_fn;

  *keyval = comm_keyval_id;
  xbt_dict_set_ext(smpi_comm_keyvals, reinterpret_cast<const char*>(keyval), sizeof(int),static_cast<void*>(value), nullptr);
  comm_keyval_id++;
  return MPI_SUCCESS;
}

int smpi_comm_keyval_free(int* keyval){
  smpi_comm_key_elem elem =
     static_cast<smpi_comm_key_elem>(xbt_dict_get_or_null_ext(smpi_comm_keyvals,  reinterpret_cast<const char*>(keyval), sizeof(int)));
  if(elem==nullptr)
    return MPI_ERR_ARG;
  xbt_dict_remove_ext(smpi_comm_keyvals,  reinterpret_cast<const char*>(keyval), sizeof(int));
  xbt_free(elem);
  return MPI_SUCCESS;
}

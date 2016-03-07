/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>

#include "private.h"
#include "xbt/dict.h"
#include "smpi_mpi_dt_private.h"
#include "limits.h"
#include "src/simix/smx_private.h"
#include "colls/colls.h"
#include "xbt/ex.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_comm, smpi, "Logging specific to SMPI (comm)");

xbt_dict_t smpi_comm_keyvals = NULL;
int comm_keyval_id = 0;//avoid collisions

/* Support for cartesian topology was added, but there are 2 other types of topology, graph et dist graph. In order to
 * support them, we have to add a field MPIR_Topo_type, and replace the MPI_Topology field by an union. */

typedef struct s_smpi_mpi_communicator {
  MPI_Group group;
  MPIR_Topo_type topoType; 
  MPI_Topology topo; // to be replaced by an union
  int refcount;
  MPI_Comm leaders_comm;//inter-node communicator
  MPI_Comm intra_comm;//intra-node communicator . For MPI_COMM_WORLD this can't be used, as var is global.
  //use an intracomm stored in the process data instead
  int* leaders_map; //who is the leader of each process
  int is_uniform;
  int* non_uniform_map; //set if smp nodes have a different number of processes allocated
  int is_blocked;// are ranks allocated on the same smp node contiguous ?
  xbt_dict_t attributes;
} s_smpi_mpi_communicator_t;

static int smpi_compare_rankmap(const void *a, const void *b)
{
  const int* x = (const int*)a;
  const int* y = (const int*)b;

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

MPI_Comm smpi_comm_new(MPI_Group group, MPI_Topology topo)
{
  MPI_Comm comm;

  comm = xbt_new(s_smpi_mpi_communicator_t, 1);
  comm->group = group;
  smpi_group_use(comm->group);
  comm->refcount=1;
  comm->topoType = MPI_INVALID_TOPO;
  comm->topo = topo;
  comm->intra_comm = MPI_COMM_NULL;
  comm->leaders_comm = MPI_COMM_NULL;
  comm->is_uniform=1;
  comm->non_uniform_map = NULL;
  comm->leaders_map = NULL;
  comm->is_blocked=0;
  comm->attributes=NULL;
  return comm;
}

void smpi_comm_destroy(MPI_Comm comm)
{
  if (comm == MPI_COMM_UNINITIALIZED)
    comm = smpi_process_comm_world();
  smpi_group_unuse(comm->group);
  smpi_topo_destroy(comm->topo); // there's no use count on topos
  smpi_comm_unuse(comm);
}

int smpi_comm_dup(MPI_Comm comm, MPI_Comm* newcomm){
  if(smpi_privatize_global_variables){ //we need to switch as the called function may silently touch global variables
     smpi_switch_data_segment(smpi_process_index());
   }
  (*newcomm) = smpi_comm_new(smpi_comm_group(comm), smpi_comm_topo(comm));
  int ret = MPI_SUCCESS;
  //todo: faire en sorte que ça fonctionne avec un communicator dupliqué (refaire un init_smp ?)
  
 /* MPI_Comm tmp=smpi_comm_get_intra_comm(comm);
  if( tmp != MPI_COMM_NULL)
    smpi_comm_set_intra_comm((*newcomm), smpi_comm_dup(tmp));
  tmp=smpi_comm_get_leaders_comm(comm);
  if( tmp != MPI_COMM_NULL)
    smpi_comm_set_leaders_comm((*newcomm), smpi_comm_dup(tmp));
  if(comm->non_uniform_map !=NULL){
    (*newcomm)->non_uniform_map= 
      xbt_malloc(smpi_comm_size(comm->leaders_comm)*sizeof(int));
    memcpy((*newcomm)->non_uniform_map,
      comm->non_uniform_map,smpi_comm_size(comm->leaders_comm)*sizeof(int) );
  }
  if(comm->leaders_map !=NULL){
    (*newcomm)->leaders_map=xbt_malloc(smpi_comm_size(comm)*sizeof(int));
    memcpy((*newcomm)->leaders_map, 
      comm->leaders_map,smpi_comm_size(comm)*sizeof(int) );
  }*/
  if(comm->attributes !=NULL){
      (*newcomm)->attributes=xbt_dict_new();
      xbt_dict_cursor_t cursor = NULL;
      int *key;
      int flag;
      void* value_in;
      void* value_out;
      xbt_dict_foreach(comm->attributes, cursor, key, value_in){
        smpi_comm_key_elem elem =
           static_cast<smpi_comm_key_elem>(xbt_dict_get_or_null_ext(smpi_comm_keyvals, (const char*)key, sizeof(int)));
        if(elem && elem->copy_fn!=MPI_NULL_COPY_FN){
          ret = elem->copy_fn(comm, *key, NULL, value_in, &value_out, &flag );
          if(ret!=MPI_SUCCESS){
            smpi_comm_destroy(*newcomm);
            *newcomm=MPI_COMM_NULL;
            return ret;
          }
          if(flag)
            xbt_dict_set_ext((*newcomm)->attributes, (const char*)key, sizeof(int),value_out, NULL);
        }
      }
    }
  return ret;
}

MPI_Group smpi_comm_group(MPI_Comm comm)
{
  if (comm == MPI_COMM_UNINITIALIZED)
    comm = smpi_process_comm_world();
  return comm->group;
}

MPI_Topology smpi_comm_topo(MPI_Comm comm) {
  if (comm != MPI_COMM_NULL)
    return comm->topo;
  return NULL;
}

int smpi_comm_size(MPI_Comm comm)
{
  if (comm == MPI_COMM_UNINITIALIZED)
    comm = smpi_process_comm_world();
  return smpi_group_size(smpi_comm_group(comm));
}

int smpi_comm_rank(MPI_Comm comm)
{
  if (comm == MPI_COMM_UNINITIALIZED)
    comm = smpi_process_comm_world();
  return smpi_group_rank(smpi_comm_group(comm), smpi_process_index());
}

void smpi_comm_get_name (MPI_Comm comm, char* name, int* len)
{
  if (comm == MPI_COMM_UNINITIALIZED)
    comm = smpi_process_comm_world();
  if(comm == MPI_COMM_WORLD) {
    strcpy(name, "WORLD");
    *len = 5;
  } else {
    *len = snprintf(name, MPI_MAX_NAME_STRING, "%p", comm);
  }
}

void smpi_comm_set_leaders_comm(MPI_Comm comm, MPI_Comm leaders){
  if (comm == MPI_COMM_UNINITIALIZED)
    comm = smpi_process_comm_world();
  comm->leaders_comm=leaders;
}

void smpi_comm_set_intra_comm(MPI_Comm comm, MPI_Comm leaders){
  comm->intra_comm=leaders;
}

int* smpi_comm_get_non_uniform_map(MPI_Comm comm){
  if (comm == MPI_COMM_UNINITIALIZED)
    comm = smpi_process_comm_world();
  return comm->non_uniform_map;
}

int* smpi_comm_get_leaders_map(MPI_Comm comm){
  if (comm == MPI_COMM_UNINITIALIZED)
    comm = smpi_process_comm_world();
  return comm->leaders_map;
}

MPI_Comm smpi_comm_get_leaders_comm(MPI_Comm comm){
  if (comm == MPI_COMM_UNINITIALIZED)
    comm = smpi_process_comm_world();
  return comm->leaders_comm;
}

MPI_Comm smpi_comm_get_intra_comm(MPI_Comm comm){
  if (comm == MPI_COMM_UNINITIALIZED || comm==MPI_COMM_WORLD) 
    return smpi_process_get_comm_intra();
  else return comm->intra_comm;
}

int smpi_comm_is_uniform(MPI_Comm comm){
  if (comm == MPI_COMM_UNINITIALIZED)
    comm = smpi_process_comm_world();
  return comm->is_uniform;
}

int smpi_comm_is_blocked(MPI_Comm comm){
  if (comm == MPI_COMM_UNINITIALIZED)
    comm = smpi_process_comm_world();
  return comm->is_blocked;
}

MPI_Comm smpi_comm_split(MPI_Comm comm, int color, int key)
{
  if (comm == MPI_COMM_UNINITIALIZED)
    comm = smpi_process_comm_world();
  int system_tag = 123;
  int index, rank, size, i, j, count, reqs;
  int* sendbuf;
  int* recvbuf;
  int* rankmap;
  MPI_Group group, group_root, group_out;
  MPI_Request* requests;

  group_root = group_out = NULL;
  group = smpi_comm_group(comm);
  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  /* Gather all colors and keys on rank 0 */
  sendbuf = xbt_new(int, 2);
  sendbuf[0] = color;
  sendbuf[1] = key;
  if(rank == 0) {
    recvbuf = xbt_new(int, 2 * size);
  } else {
    recvbuf = NULL;
  }
  smpi_mpi_gather(sendbuf, 2, MPI_INT, recvbuf, 2, MPI_INT, 0, comm);
  xbt_free(sendbuf);
  /* Do the actual job */
  if(rank == 0) {
    rankmap = xbt_new(int, 2 * size);
    for(i = 0; i < size; i++) {
      if(recvbuf[2 * i] == MPI_UNDEFINED) {
        continue;
      }
      count = 0;
      for(j = i + 1; j < size; j++)  {
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
      group_out = smpi_group_new(count);
      if(i == 0) {
        group_root = group_out; /* Save root's group */
      }
      for(j = 0; j < count; j++) {
        //increment refcounter in order to avoid freeing the group too quick before copy
        index = smpi_group_index(group, rankmap[2 * j]);
        smpi_group_set_mapping(group_out, index, j);
      }
      requests = xbt_new(MPI_Request, count);
      reqs = 0;
      for(j = 0; j < count; j++) {
        if(rankmap[2 * j] != 0) {
          requests[reqs] = smpi_isend_init(&group_out, 1, MPI_PTR, rankmap[2 * j], system_tag, comm);
          reqs++;
        }
      }
      smpi_mpi_startall(reqs, requests);
      smpi_mpi_waitall(reqs, requests, MPI_STATUS_IGNORE);
      xbt_free(requests);
    }
    xbt_free(recvbuf);
    group_out = group_root; /* exit with root's group */
  } else {
    if(color != MPI_UNDEFINED) {
      smpi_mpi_recv(&group_out, 1, MPI_PTR, 0, system_tag, comm, MPI_STATUS_IGNORE);
      if(group_out){
        group_out=smpi_group_copy(group_out);
      }
    } /* otherwise, exit with group_out == NULL */
  }
  return group_out ? smpi_comm_new(group_out, NULL) : MPI_COMM_NULL;
}

void smpi_comm_use(MPI_Comm comm){
  if (comm == MPI_COMM_UNINITIALIZED)
    comm = smpi_process_comm_world();
  comm->refcount++;
}

void smpi_comm_unuse(MPI_Comm comm){
  if (comm == MPI_COMM_UNINITIALIZED)
    comm = smpi_process_comm_world();
  comm->refcount--;
  if(comm->refcount==0){
    if(comm->intra_comm != MPI_COMM_NULL)
      smpi_comm_unuse(comm->intra_comm);
    if(comm->leaders_comm != MPI_COMM_NULL)
      smpi_comm_unuse(comm->leaders_comm);
    if(comm->non_uniform_map !=NULL)
      xbt_free(comm->non_uniform_map);
    if(comm->leaders_map !=NULL)
      xbt_free(comm->leaders_map);
    if(comm->attributes !=NULL){
      xbt_dict_cursor_t cursor = NULL;
      int* key;
      void * value;
      int flag;
      xbt_dict_foreach(comm->attributes, cursor, key, value){
        smpi_comm_key_elem elem =
           static_cast<smpi_comm_key_elem>(xbt_dict_get_or_null(smpi_comm_keyvals, (const char*)key));
        if(elem &&  elem->delete_fn)
          elem->delete_fn(comm, *key, value, &flag);
      }
    }
    xbt_free(comm);
  }
}

static int compare_ints (const void *a, const void *b)
{
  const int *da = (const int *) a;
  const int *db = (const int *) b;

  return (*da > *db) - (*da < *db);
}

void smpi_comm_init_smp(MPI_Comm comm){
  int leader = -1;

  if (comm == MPI_COMM_UNINITIALIZED)
    comm = smpi_process_comm_world();

  int comm_size =smpi_comm_size(comm);
  
  // If we are in replay - perform an ugly hack  
  // tell SimGrid we are not in replay for a while, because we need the buffers to be copied for the following calls
  int replaying = 0; //cache data to set it back again after
  if(smpi_process_get_replaying()){
   replaying=1;
   smpi_process_set_replaying(0);
  }

  if(smpi_privatize_global_variables){ //we need to switch as the called function may silently touch global variables
     smpi_switch_data_segment(smpi_process_index());
   }
  //identify neighbours in comm
  //get the indexes of all processes sharing the same simix host
  xbt_swag_t process_list = simcall_host_get_process_list(SIMIX_host_self());
  int intra_comm_size = 0;
  //only one process/node, disable SMP support and return
//  if(intra_comm_size==1){
//      smpi_comm_set_intra_comm(comm, MPI_COMM_SELF);
//      //smpi_comm_set_leaders_comm(comm, comm);
//      smpi_process_set_comm_intra(MPI_COMM_SELF);
//      return;
//  }
  int i =0;
  int min_index=INT_MAX;//the minimum index will be the leader
  smx_process_t process = NULL;
  xbt_swag_foreach(process, process_list) {
    //is_in_comm=0;
    int index = SIMIX_process_get_PID(process) -1;

    if(smpi_group_rank(smpi_comm_group(comm),  index)!=MPI_UNDEFINED){
        intra_comm_size++;
      //the process is in the comm
      if(index < min_index)
        min_index=index;
      i++;
    }
  }
  XBT_DEBUG("number of processes deployed on my node : %d", intra_comm_size);
  MPI_Group group_intra = smpi_group_new(intra_comm_size);
  i=0;
  process = NULL;
  xbt_swag_foreach(process, process_list) {
    //is_in_comm=0;
    int index = SIMIX_process_get_PID(process) -1;
    if(smpi_group_rank(smpi_comm_group(comm),  index)!=MPI_UNDEFINED){
      smpi_group_set_mapping(group_intra, index, i);
      i++;
    }
  }

  MPI_Comm comm_intra = smpi_comm_new(group_intra, NULL);
  //MPI_Comm shmem_comm = smpi_process_comm_intra();
  //int intra_rank = smpi_comm_rank(shmem_comm);

  //if(smpi_process_index()==min_index)
  leader=min_index;

  int * leaders_map= (int*)xbt_malloc0(sizeof(int)*comm_size);
  int * leader_list= (int*)xbt_malloc0(sizeof(int)*comm_size);
  for(i=0; i<comm_size; i++){
      leader_list[i]=-1;
  }

  smpi_coll_tuned_allgather_mpich(&leader, 1, MPI_INT , leaders_map, 1, MPI_INT, comm);

  if(smpi_privatize_global_variables){ //we need to switch as the called function may silently touch global variables
     smpi_switch_data_segment(smpi_process_index());
   }

  if(!comm->leaders_map){
    comm->leaders_map= leaders_map;
  }else{
    xbt_free(leaders_map);
  }
  int j=0;
  int leader_group_size = 0;
  for(i=0; i<comm_size; i++){
      int already_done=0;
      for(j=0;j<leader_group_size; j++){
        if(comm->leaders_map[i]==leader_list[j]){
            already_done=1;
        }
      }
      if(!already_done){
        leader_list[leader_group_size]=comm->leaders_map[i];
        leader_group_size++;
      }
  }
  qsort(leader_list, leader_group_size, sizeof(int),compare_ints);

  MPI_Group leaders_group = smpi_group_new(leader_group_size);

  MPI_Comm leader_comm = MPI_COMM_NULL;
  if(MPI_COMM_WORLD!=MPI_COMM_UNINITIALIZED && comm!=MPI_COMM_WORLD){
    //create leader_communicator
    for (i=0; i< leader_group_size;i++)
      smpi_group_set_mapping(leaders_group, leader_list[i], i);
    leader_comm = smpi_comm_new(leaders_group, NULL);
    smpi_comm_set_leaders_comm(comm, leader_comm);
    smpi_comm_set_intra_comm(comm, comm_intra);

   //create intracommunicator
   // smpi_comm_set_intra_comm(comm, smpi_comm_split(comm, *(int*)SIMIX_host_self(), comm_rank));
  }else{
    for (i=0; i< leader_group_size;i++)
      smpi_group_set_mapping(leaders_group, leader_list[i], i);

    leader_comm = smpi_comm_new(leaders_group, NULL);
    if(smpi_comm_get_leaders_comm(comm)==MPI_COMM_NULL)
      smpi_comm_set_leaders_comm(comm, leader_comm);
    smpi_process_set_comm_intra(comm_intra);
  }

  int is_uniform = 1;

  // Are the nodes uniform ? = same number of process/node
  int my_local_size=smpi_comm_size(comm_intra);
  if(smpi_comm_rank(comm_intra)==0) {
    int* non_uniform_map = xbt_new0(int,leader_group_size);
    smpi_coll_tuned_allgather_mpich(&my_local_size, 1, MPI_INT,
        non_uniform_map, 1, MPI_INT, leader_comm);
    for(i=0; i < leader_group_size; i++) {
      if(non_uniform_map[0] != non_uniform_map[i]) {
        is_uniform = 0;
        break;
      }
    }
    if(!is_uniform && smpi_comm_is_uniform(comm)){
        comm->non_uniform_map= non_uniform_map;
    }else{
        xbt_free(non_uniform_map);
    }
    comm->is_uniform=is_uniform;
  }
  smpi_coll_tuned_bcast_mpich(&(comm->is_uniform),1, MPI_INT, 0, comm_intra );

  if(smpi_privatize_global_variables){ //we need to switch as the called function may silently touch global variables
     smpi_switch_data_segment(smpi_process_index());
   }
  // Are the ranks blocked ? = allocated contiguously on the SMP nodes
  int is_blocked=1;
  int prev=smpi_group_rank(smpi_comm_group(comm), smpi_group_index(smpi_comm_group(comm_intra), 0));
    for (i=1; i<my_local_size; i++){
      int that=smpi_group_rank(smpi_comm_group(comm),smpi_group_index(smpi_comm_group(comm_intra), i));
      if(that!=prev+1){
        is_blocked=0;
        break;
      }
      prev = that;
  }

  int global_blocked;
  smpi_mpi_allreduce(&is_blocked, &(global_blocked), 1, MPI_INT, MPI_LAND, comm);

  if(MPI_COMM_WORLD==MPI_COMM_UNINITIALIZED || comm==MPI_COMM_WORLD){
    if(smpi_comm_rank(comm)==0){
        comm->is_blocked=global_blocked;
    }
  }else{
    comm->is_blocked=global_blocked;
  }
  xbt_free(leader_list);
  
  if(replaying==1)
    smpi_process_set_replaying(1); 
}

int smpi_comm_attr_delete(MPI_Comm comm, int keyval){
  smpi_comm_key_elem elem =
     static_cast<smpi_comm_key_elem>(xbt_dict_get_or_null_ext(smpi_comm_keyvals, (const char*)&keyval, sizeof(int)));
  if(!elem)
    return MPI_ERR_ARG;
  if(elem->delete_fn!=MPI_NULL_DELETE_FN){
    void * value;
    int flag;
    if(smpi_comm_attr_get(comm, keyval, &value, &flag)==MPI_SUCCESS){
      int ret = elem->delete_fn(comm, keyval, value, &flag);
      if(ret!=MPI_SUCCESS) return ret;
    }
  }
  if(comm->attributes==NULL)
    return MPI_ERR_ARG;

  xbt_dict_remove_ext(comm->attributes, (const char*)&keyval, sizeof(int));
  return MPI_SUCCESS;
}

int smpi_comm_attr_get(MPI_Comm comm, int keyval, void* attr_value, int* flag){
  smpi_comm_key_elem elem =
    static_cast<smpi_comm_key_elem>(xbt_dict_get_or_null_ext(smpi_comm_keyvals, (const char*)&keyval, sizeof(int)));
  if(!elem)
    return MPI_ERR_ARG;
  xbt_ex_t ex;
  if(comm->attributes==NULL){
    *flag=0;
    return MPI_SUCCESS;
  }
  TRY {
    *(void**)attr_value = xbt_dict_get_ext(comm->attributes,  (const char*)&keyval, sizeof(int));
    *flag=1;
  } CATCH(ex) {
    *flag=0;
    xbt_ex_free(ex);
  }
  return MPI_SUCCESS;
}

int smpi_comm_attr_put(MPI_Comm comm, int keyval, void* attr_value){
  if(!smpi_comm_keyvals)
  smpi_comm_keyvals = xbt_dict_new();
  smpi_comm_key_elem elem =
    static_cast<smpi_comm_key_elem>(xbt_dict_get_or_null_ext(smpi_comm_keyvals,  (const char*)&keyval, sizeof(int)));
  if(!elem )
    return MPI_ERR_ARG;
  int flag;
  void* value;
  smpi_comm_attr_get(comm, keyval, &value, &flag);
  if(flag && elem->delete_fn!=MPI_NULL_DELETE_FN){
    int ret = elem->delete_fn(comm, keyval, value, &flag);
    if(ret!=MPI_SUCCESS) return ret;
  }
  if(comm->attributes==NULL)
    comm->attributes=xbt_dict_new();

  xbt_dict_set_ext(comm->attributes,  (const char*)&keyval, sizeof(int), attr_value, NULL);
  return MPI_SUCCESS;
}

int smpi_comm_keyval_create(MPI_Comm_copy_attr_function* copy_fn, MPI_Comm_delete_attr_function* delete_fn, int* keyval,
                            void* extra_state){
  if(!smpi_comm_keyvals)
  smpi_comm_keyvals = xbt_dict_new();

  smpi_comm_key_elem value = (smpi_comm_key_elem) xbt_new0(s_smpi_mpi_comm_key_elem_t,1);

  value->copy_fn=copy_fn;
  value->delete_fn=delete_fn;

  *keyval = comm_keyval_id;
  xbt_dict_set_ext(smpi_comm_keyvals, (const char*)keyval, sizeof(int),(void*)value, NULL);
  comm_keyval_id++;
  return MPI_SUCCESS;
}

int smpi_comm_keyval_free(int* keyval){
  smpi_comm_key_elem elem =
     static_cast<smpi_comm_key_elem>(xbt_dict_get_or_null_ext(smpi_comm_keyvals,  (const char*)keyval, sizeof(int)));
  if(!elem){
    return MPI_ERR_ARG;
  }
  xbt_dict_remove_ext(smpi_comm_keyvals,  (const char*)keyval, sizeof(int));
  xbt_free(elem);
  return MPI_SUCCESS;
}

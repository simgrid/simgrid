/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "smpi_comm.hpp"
#include "smpi_errhandler.hpp"
#include "smpi_info.hpp"

#include <string>

extern "C" { // This should really use the C linkage to be usable from Fortran

void mpi_comm_rank_(int* comm, int* rank, int* ierr) {
   *ierr = MPI_Comm_rank(simgrid::smpi::Comm::f2c(*comm), rank);
}

void mpi_comm_size_(int* comm, int* size, int* ierr) {
   *ierr = MPI_Comm_size(simgrid::smpi::Comm::f2c(*comm), size);
}

void mpi_comm_dup_(int* comm, int* newcomm, int* ierr) {
  MPI_Comm tmp;

  *ierr = MPI_Comm_dup(simgrid::smpi::Comm::f2c(*comm), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newcomm = tmp->c2f();
  }
}

void mpi_comm_create_(int* comm, int* group, int* newcomm, int* ierr) {
  MPI_Comm tmp;
  *ierr = MPI_Comm_create(simgrid::smpi::Comm::f2c(*comm),simgrid::smpi::Group::f2c(*group), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newcomm = tmp->c2f();
  }
}

void mpi_comm_free_(int* comm, int* ierr) {
  MPI_Comm tmp = simgrid::smpi::Comm::f2c(*comm);
  if(tmp != MPI_COMM_WORLD && tmp != MPI_COMM_NULL) {
    simgrid::smpi::Comm::destroy(tmp);
    simgrid::smpi::Comm::free_f(*comm);
  }
  *ierr = MPI_SUCCESS;
}

void mpi_comm_split_(int* comm, int* color, int* key, int* comm_out, int* ierr) {
  MPI_Comm tmp;

  *ierr = MPI_Comm_split(simgrid::smpi::Comm::f2c(*comm), *color, *key, &tmp);
  if(*ierr == MPI_SUCCESS) {
    *comm_out = tmp->c2f();
  }
}

void mpi_comm_group_(int* comm, int* group_out,  int* ierr) {
  MPI_Group tmp;
  *ierr = MPI_Comm_group(simgrid::smpi::Comm::f2c(*comm), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *group_out = tmp->c2f();
  }
}

void mpi_comm_create_group_ (int* comm, int* group, int i, int* comm_out, int* ierr){
  MPI_Comm tmp;
 *ierr = MPI_Comm_create_group(simgrid::smpi::Comm::f2c(*comm),simgrid::smpi::Group::f2c(*group), i, &tmp);
  if(*ierr == MPI_SUCCESS) {
    *comm_out = tmp->c2f();
  }
}

void mpi_comm_get_attr_ (int* comm, int* comm_keyval, int *attribute_val, int *flag, int* ierr){
 int* value = nullptr;
 *ierr = MPI_Comm_get_attr (simgrid::smpi::Comm::f2c(*comm), *comm_keyval, &value, flag);
 if (*flag == 1)
   *attribute_val = *value;
}

void mpi_comm_set_attr_ (int* comm, int* comm_keyval, int *attribute_val, int* ierr){
  auto* val = xbt_new(int, 1);
  *val      = *attribute_val;
  *ierr     = MPI_Comm_set_attr(simgrid::smpi::Comm::f2c(*comm), *comm_keyval, val);
}

void mpi_comm_delete_attr_ (int* comm, int* comm_keyval, int* ierr){
 *ierr = MPI_Comm_delete_attr (simgrid::smpi::Comm::f2c(*comm),  *comm_keyval);
}

void mpi_comm_create_keyval_ (void* copy_fn, void* delete_fn, int* keyval, void* extra_state, int* ierr){
  smpi_copy_fn _copy_fn={nullptr,nullptr,nullptr,(*(int*)copy_fn) == 0 ? nullptr : reinterpret_cast<MPI_Comm_copy_attr_function_fort*>(copy_fn),nullptr,nullptr};
  smpi_delete_fn _delete_fn={nullptr,nullptr,nullptr,(*(int*)delete_fn) == 0 ? nullptr : reinterpret_cast<MPI_Comm_delete_attr_function_fort*>(delete_fn),nullptr,nullptr};
  *ierr = simgrid::smpi::Keyval::keyval_create<simgrid::smpi::Comm>(_copy_fn, _delete_fn, keyval, extra_state, true);
}

void mpi_comm_free_keyval_ (int* keyval, int* ierr) {
 *ierr = MPI_Comm_free_keyval( keyval);
}

void mpi_comm_get_name_ (int* comm, char* name, int* len, int* ierr){
 *ierr = MPI_Comm_get_name(simgrid::smpi::Comm::f2c(*comm), name, len);
  for(int i = *len; i<MPI_MAX_OBJECT_NAME+1; i++)
    name[i]=' ';
}

void mpi_comm_compare_ (int* comm1, int* comm2, int *result, int* ierr){
 *ierr = MPI_Comm_compare(simgrid::smpi::Comm::f2c(*comm1), simgrid::smpi::Comm::f2c(*comm2), result);
}

void mpi_comm_disconnect_ (int* comm, int* ierr){
 MPI_Comm tmp = simgrid::smpi::Comm::f2c(*comm);
 *ierr = MPI_Comm_disconnect(&tmp);
 if(*ierr == MPI_SUCCESS) {
   simgrid::smpi::Comm::free_f(*comm);
 }
}

void mpi_comm_set_errhandler_ (int* comm, int* errhandler, int* ierr) {
 *ierr = MPI_Errhandler_set(simgrid::smpi::Comm::f2c(*comm), simgrid::smpi::Errhandler::f2c(*errhandler));
}

void mpi_comm_get_errhandler_ (int* comm, int* errhandler, int* ierr) {
 MPI_Errhandler tmp;
 *ierr = MPI_Errhandler_get(simgrid::smpi::Comm::f2c(*comm), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *errhandler = tmp->c2f();
 }
}

void mpi_comm_test_inter_ (int* comm, int* flag, int* ierr) {
 *ierr = MPI_Comm_test_inter(simgrid::smpi::Comm::f2c(*comm), flag);
}

void mpi_comm_remote_group_ (int* comm, int*  group, int* ierr) {
  MPI_Group tmp;
 *ierr = MPI_Comm_remote_group(simgrid::smpi::Comm::f2c(*comm), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *group = tmp->c2f();
 }
}

void mpi_comm_remote_size_ (int* comm, int* size, int* ierr) {
 *ierr = MPI_Comm_remote_size(simgrid::smpi::Comm::f2c(*comm), size);
}

void mpi_comm_set_name_ (int* comm, char* name, int* ierr){
 int count;
 for(count=MPI_MAX_OBJECT_NAME-1; count>=0 && name[count]==' '; count--);
 count+=1;
 std::string tname(name, count);
 *ierr = MPI_Comm_set_name(simgrid::smpi::Comm::f2c(*comm), tname.c_str());
}

void mpi_comm_dup_with_info_ (int* comm, int* info, int* newcomm, int* ierr){
  MPI_Comm tmp;
  *ierr = MPI_Comm_dup_with_info(simgrid::smpi::Comm::f2c(*comm), simgrid::smpi::Info::f2c(*info),&tmp);
  if(*ierr == MPI_SUCCESS) {
    *newcomm = tmp->c2f();
  }
}

void mpi_comm_split_type_ (int* comm, int* split_type, int* key, int* info, int* newcomm, int* ierr){
  MPI_Comm tmp;
  *ierr = MPI_Comm_split_type(simgrid::smpi::Comm::f2c(*comm), *split_type, *key, simgrid::smpi::Info::f2c(*info), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newcomm = tmp->c2f();
  }
}

void mpi_comm_set_info_ (int* comm, int* info, int* ierr){
 *ierr = MPI_Comm_set_info (simgrid::smpi::Comm::f2c(*comm), simgrid::smpi::Info::f2c(*info));
}

void mpi_comm_get_info_ (int* comm, int* info, int* ierr){
 MPI_Info tmp;
 *ierr = MPI_Comm_get_info (simgrid::smpi::Comm::f2c(*comm), &tmp);
 if(*ierr==MPI_SUCCESS){
   *info = tmp->c2f();
 }
}

void mpi_comm_create_errhandler_ ( void *function, int *errhandler, int* ierr){
 MPI_Errhandler tmp;
 *ierr = MPI_Comm_create_errhandler( reinterpret_cast<MPI_Comm_errhandler_fn*>(function), &tmp);
 if(*ierr==MPI_SUCCESS){
   *errhandler = tmp->c2f();
 }
}

void mpi_comm_call_errhandler_ (int* comm,int* errorcode, int* ierr){
 *ierr = MPI_Comm_call_errhandler(simgrid::smpi::Comm::f2c(*comm), *errorcode);
}

void mpi_comm_connect_(char* port_name, int* info, int* root, int* comm, int* newcomm, int* ierr)
{
  MPI_Comm tmp;
  *ierr = MPI_Comm_connect(port_name, simgrid::smpi::Info::f2c(*info), *root, simgrid::smpi::Comm::f2c(*comm), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newcomm = tmp->c2f();
  }
}

void mpi_comm_join_ ( int* fd, int* intercomm, int* ierr){
  MPI_Comm tmp;
  *ierr = MPI_Comm_join( *fd, &tmp);
  if(*ierr == MPI_SUCCESS) {
    *intercomm = tmp->c2f();
  }
}


void mpi_comm_accept_ ( char *port_name, int* info, int* root, int* comm, int*newcomm, int* ierr){
  MPI_Comm tmp;
  *ierr = MPI_Comm_accept( port_name, simgrid::smpi::Info::f2c(*info), *root, simgrid::smpi::Comm::f2c(*comm), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newcomm = tmp->c2f();
  }
}

void mpi_comm_spawn_ ( char *command, char *argv, int* maxprocs, int* info, int* root, int* comm, int* intercomm,
                       int* array_of_errcodes, int* ierr){
  MPI_Comm tmp;
  *ierr = MPI_Comm_spawn( command, &argv, *maxprocs, simgrid::smpi::Info::f2c(*info), *root, simgrid::smpi::Comm::f2c(*comm), &tmp,
                          array_of_errcodes);
  if(*ierr == MPI_SUCCESS) {
    *intercomm = tmp->c2f();
  }
}

void mpi_comm_spawn_multiple_ ( int* count, char *array_of_commands, char** array_of_argv, int* array_of_maxprocs,
                                int* array_of_info, int* root,
 int* comm, int* intercomm, int* array_of_errcodes, int* ierr){
 MPI_Comm tmp;
 *ierr = MPI_Comm_spawn_multiple(* count, &array_of_commands, &array_of_argv, array_of_maxprocs,
 reinterpret_cast<MPI_Info*>(array_of_info), *root, simgrid::smpi::Comm::f2c(*comm), &tmp, array_of_errcodes);
 if(*ierr == MPI_SUCCESS) {
   *intercomm = tmp->c2f();
 }
}

void mpi_comm_get_parent_ ( int* parent, int* ierr){
  MPI_Comm tmp;
  *ierr = MPI_Comm_get_parent( &tmp);
  if(*ierr == MPI_SUCCESS) {
    *parent = tmp->c2f();
  }
}

}

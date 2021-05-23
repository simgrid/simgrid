/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "smpi_comm.hpp"
#include "smpi_datatype.hpp"

#include <string>

extern "C" { // This should really use the C linkage to be usable from Fortran

void mpi_type_extent_(int* datatype, MPI_Aint * extent, int* ierr){
  *ierr= MPI_Type_extent(simgrid::smpi::Datatype::f2c(*datatype),  extent);
}

void mpi_type_free_(int* datatype, int* ierr){
  MPI_Datatype tmp= simgrid::smpi::Datatype::f2c(*datatype);
  *ierr= MPI_Type_free (&tmp);
  if(*ierr == MPI_SUCCESS) {
    simgrid::smpi::F2C::free_f(*datatype);
  }
}

void mpi_type_ub_(int* datatype, MPI_Aint * disp, int* ierr){
  *ierr= MPI_Type_ub(simgrid::smpi::Datatype::f2c(*datatype), disp);
}

void mpi_type_lb_(int* datatype, MPI_Aint * extent, int* ierr){
  *ierr= MPI_Type_extent(simgrid::smpi::Datatype::f2c(*datatype), extent);
}

void mpi_type_size_(int* datatype, int *size, int* ierr)
{
  *ierr = MPI_Type_size(simgrid::smpi::Datatype::f2c(*datatype), size);
}

void mpi_type_dup_ (int*  datatype, int* newdatatype, int* ierr){
 MPI_Datatype tmp;
 *ierr = MPI_Type_dup(simgrid::smpi::Datatype::f2c(*datatype), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *newdatatype = tmp->c2f();
 }
}

void mpi_type_set_name_(int* datatype, char* name, int* ierr, int size)
{
  std::string tname(name, size);
  *ierr = MPI_Type_set_name(simgrid::smpi::Datatype::f2c(*datatype), tname.c_str());
}

void mpi_type_get_name_ (int*  datatype, char * name, int* len, int* ierr){
 *ierr = MPI_Type_get_name(simgrid::smpi::Datatype::f2c(*datatype),name,len);
  for(int i = *len; i<MPI_MAX_OBJECT_NAME+1; i++)
    name[i]=' ';
}

void mpi_type_get_attr_ (int* type, int* type_keyval, int *attribute_val, int* flag, int* ierr){
 int* value = nullptr;
 *ierr = MPI_Type_get_attr ( simgrid::smpi::Datatype::f2c(*type), *type_keyval, &value, flag);
 if (*flag == 1)
   *attribute_val = *value;
}

void mpi_type_set_attr_ (int* type, int* type_keyval, int *attribute_val, int* ierr){
  auto* val = xbt_new(int, 1);
  *val      = *attribute_val;
  *ierr     = MPI_Type_set_attr(simgrid::smpi::Datatype::f2c(*type), *type_keyval, val);
}

void mpi_type_delete_attr_ (int* type, int* type_keyval, int* ierr){
 *ierr = MPI_Type_delete_attr ( simgrid::smpi::Datatype::f2c(*type),  *type_keyval);
}

void mpi_type_create_keyval_ (void* copy_fn, void*  delete_fn, int* keyval, void* extra_state, int* ierr){
  smpi_copy_fn _copy_fn={nullptr,nullptr,nullptr,nullptr,(*(int*)copy_fn) == 0 ? nullptr : reinterpret_cast<MPI_Type_copy_attr_function_fort*>(copy_fn),nullptr};
  smpi_delete_fn _delete_fn={nullptr,nullptr,nullptr,nullptr,(*(int*)delete_fn) == 0 ? nullptr : reinterpret_cast<MPI_Type_delete_attr_function_fort*>(delete_fn),nullptr};
  *ierr = simgrid::smpi::Keyval::keyval_create<simgrid::smpi::Datatype>(_copy_fn, _delete_fn, keyval, extra_state);
}

void mpi_type_free_keyval_ (int* keyval, int* ierr) {
 *ierr = MPI_Type_free_keyval( keyval);
}

void mpi_type_get_extent_(int* datatype, MPI_Aint* lb, MPI_Aint* extent, int* ierr)
{
  *ierr = MPI_Type_get_extent(simgrid::smpi::Datatype::f2c(*datatype), lb, extent);
}

void mpi_type_get_true_extent_(int* datatype, MPI_Aint* lb, MPI_Aint* extent, int* ierr)
{
  *ierr = MPI_Type_get_true_extent(simgrid::smpi::Datatype::f2c(*datatype), lb, extent);
}

void mpi_type_commit_(int* datatype,  int* ierr){
  MPI_Datatype tmp= simgrid::smpi::Datatype::f2c(*datatype);
  *ierr= MPI_Type_commit(&tmp);
}

void mpi_type_contiguous_ (int* count, int* old_type, int*  newtype, int* ierr) {
  MPI_Datatype tmp;
  *ierr = MPI_Type_contiguous(*count, simgrid::smpi::Datatype::f2c(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->c2f();
  }
}

void mpi_type_vector_(int* count, int* blocklen, int* stride, int* old_type, int* newtype,  int* ierr){
  MPI_Datatype tmp;
  *ierr= MPI_Type_vector(*count, *blocklen, *stride, simgrid::smpi::Datatype::f2c(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->c2f();
  }
}

void mpi_type_hvector_(int* count, int* blocklen, MPI_Aint* stride, int* old_type, int* newtype,  int* ierr){
  MPI_Datatype tmp;
  *ierr= MPI_Type_hvector (*count, *blocklen, *stride, simgrid::smpi::Datatype::f2c(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->c2f();
  }
}

void mpi_type_create_hvector_(int* count, int* blocklen, MPI_Aint* stride, int* old_type, int* newtype,  int* ierr){
  MPI_Datatype tmp;
  *ierr= MPI_Type_hvector(*count, *blocklen, *stride, simgrid::smpi::Datatype::f2c(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->c2f();
  }
}

void mpi_type_hindexed_ (int* count, int* blocklens, int* indices, int* old_type, int*  newtype, int* ierr) {
  MPI_Datatype tmp;
  std::vector<MPI_Aint> indices_aint(*count);
  for(int i=0; i<*count; i++)
    indices_aint[i]=indices[i];
  *ierr = MPI_Type_hindexed(*count, blocklens, indices_aint.data(), simgrid::smpi::Datatype::f2c(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->c2f();
  }
}

void mpi_type_create_hindexed_(int* count, int* blocklens, MPI_Aint* indices, int* old_type, int*  newtype, int* ierr){
  MPI_Datatype tmp;
  *ierr = MPI_Type_create_hindexed(*count, blocklens, indices, simgrid::smpi::Datatype::f2c(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->c2f();
  }
}

void mpi_type_create_hindexed_block_ (int* count, int* blocklength, MPI_Aint* indices, int* old_type, int*  newtype,
                                      int* ierr) {
  MPI_Datatype tmp;
  *ierr = MPI_Type_create_hindexed_block(*count, *blocklength, indices, simgrid::smpi::Datatype::f2c(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->c2f();
  }
}

void mpi_type_indexed_ (int* count, int* blocklens, int* indices, int* old_type, int*  newtype, int* ierr) {
  MPI_Datatype tmp;
  *ierr = MPI_Type_indexed(*count, blocklens, indices, simgrid::smpi::Datatype::f2c(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->c2f();
  }
}

void mpi_type_create_indexed_(int* count, int* blocklens, int* indices, int* old_type, int*  newtype, int* ierr){
  MPI_Datatype tmp;
  *ierr = MPI_Type_create_indexed(*count, blocklens, indices, simgrid::smpi::Datatype::f2c(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->c2f();
  }
}

void mpi_type_create_indexed_block_ (int* count, int* blocklength, int* indices,  int* old_type,  int*newtype,
                                     int* ierr){
  MPI_Datatype tmp;
  *ierr = MPI_Type_create_indexed_block(*count, *blocklength, indices, simgrid::smpi::Datatype::f2c(*old_type), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->c2f();
  }
}

void mpi_type_struct_ (int* count, int* blocklens, int* indices, int* old_types, int*  newtype, int* ierr) {
  MPI_Datatype tmp;
  std::vector<MPI_Aint> indices_aint(*count);
  std::vector<MPI_Datatype> types(*count);
  for (int i = 0; i < *count; i++) {
    indices_aint[i]=indices[i];
    types[i] = simgrid::smpi::Datatype::f2c(old_types[i]);
  }
  *ierr = MPI_Type_struct(*count, blocklens, indices_aint.data(), types.data(), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->c2f();
  }
}

void mpi_type_create_struct_(int* count, int* blocklens, MPI_Aint* indices, int*  old_types, int*  newtype, int* ierr){
  MPI_Datatype tmp;
  std::vector<MPI_Datatype> types(*count);
  for (int i = 0; i < *count; i++) {
    types[i] = simgrid::smpi::Datatype::f2c(old_types[i]);
  }
  *ierr = MPI_Type_create_struct(*count, blocklens, indices, types.data(), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->c2f();
  }
}

void mpi_pack_ (void* inbuf, int* incount, int* type, void* outbuf, int* outcount, int* position, int* comm, int* ierr) {
 *ierr = MPI_Pack(inbuf, *incount, simgrid::smpi::Datatype::f2c(*type), outbuf, *outcount, position, simgrid::smpi::Comm::f2c(*comm));
}

void mpi_unpack_ (void* inbuf, int* insize, int* position, void* outbuf, int* outcount, int* type, int* comm,
                  int* ierr) {
 *ierr = MPI_Unpack(inbuf, *insize, position, outbuf, *outcount, simgrid::smpi::Datatype::f2c(*type), simgrid::smpi::Comm::f2c(*comm));
}

void mpi_pack_external_size_ (char *datarep, int* incount, int* datatype, MPI_Aint *size, int* ierr){
 *ierr = MPI_Pack_external_size(datarep, *incount, simgrid::smpi::Datatype::f2c(*datatype), size);
}

void mpi_pack_external_ (char *datarep, void *inbuf, int* incount, int* datatype, void *outbuf, MPI_Aint* outcount,
                         MPI_Aint *position, int* ierr){
 *ierr = MPI_Pack_external(datarep, inbuf, *incount, simgrid::smpi::Datatype::f2c(*datatype), outbuf, *outcount, position);
}

void mpi_unpack_external_ ( char *datarep, void *inbuf, MPI_Aint* insize, MPI_Aint *position, void *outbuf,
                            int* outcount, int* datatype, int* ierr){
 *ierr = MPI_Unpack_external( datarep, inbuf, *insize, position, outbuf, *outcount, simgrid::smpi::Datatype::f2c(*datatype));
}


void mpi_type_get_envelope_ ( int* datatype, int *num_integers, int *num_addresses, int *num_datatypes, int *combiner,
                              int* ierr){
 *ierr = MPI_Type_get_envelope(  simgrid::smpi::Datatype::f2c(*datatype), num_integers,
 num_addresses, num_datatypes, combiner);
}

void mpi_type_get_contents_ (int* datatype, int* max_integers, int* max_addresses, int* max_datatypes,
                             int* array_of_integers, MPI_Aint* array_of_addresses,
 int* array_of_datatypes, int* ierr){
 *ierr = MPI_Type_get_contents(simgrid::smpi::Datatype::f2c(*datatype), *max_integers, *max_addresses,*max_datatypes,
                               array_of_integers, array_of_addresses, reinterpret_cast<MPI_Datatype*>(array_of_datatypes));
}

void mpi_type_create_darray_ (int* size, int* rank, int* ndims, int* array_of_gsizes, int* array_of_distribs,
                              int* array_of_dargs, int* array_of_psizes,
 int* order, int* oldtype, int*newtype, int* ierr) {
  MPI_Datatype tmp;
  *ierr = MPI_Type_create_darray(*size, *rank, *ndims,  array_of_gsizes,
  array_of_distribs,  array_of_dargs,  array_of_psizes,
  *order,  simgrid::smpi::Datatype::f2c(*oldtype), &tmp) ;
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->c2f();
  }
}

void mpi_type_create_resized_ (int* oldtype,MPI_Aint* lb, MPI_Aint* extent, int*newtype, int* ierr){
  MPI_Datatype tmp;
  *ierr = MPI_Type_create_resized(simgrid::smpi::Datatype::f2c(*oldtype),*lb, *extent, &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->c2f();
  }
}

void mpi_type_create_subarray_ (int* ndims,int *array_of_sizes, int *array_of_subsizes, int *array_of_starts,
                                int* order, int* oldtype, int*newtype, int* ierr){
  MPI_Datatype tmp;
  *ierr = MPI_Type_create_subarray(*ndims,array_of_sizes, array_of_subsizes, array_of_starts, *order,
                                   simgrid::smpi::Datatype::f2c(*oldtype), &tmp);
  if(*ierr == MPI_SUCCESS) {
    *newtype = tmp->c2f();
  }
}

void mpi_type_match_size_ (int* typeclass,int* size,int* datatype, int* ierr){
  MPI_Datatype tmp;
  *ierr = MPI_Type_match_size(*typeclass,*size,&tmp);
  if(*ierr == MPI_SUCCESS) {
    *datatype = tmp->c2f();
  }
}
}

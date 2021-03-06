/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "smpi_datatype_derived.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smpi_pmpi);

/* PMPI User level calls */

int PMPI_Type_free(MPI_Datatype * datatype)
{
  /* Free a predefined datatype is an error according to the standard, and should be checked for */
  if (*datatype == MPI_DATATYPE_NULL || (*datatype)->flags() & DT_FLAG_PREDEFINED) {
    return MPI_ERR_TYPE;
  } else {
    simgrid::smpi::Datatype::unref(*datatype);
    *datatype=MPI_DATATYPE_NULL;
    return MPI_SUCCESS;
  }
}

int PMPI_Type_size(MPI_Datatype datatype, int *size)
{
  CHECK_MPI_NULL(1, MPI_DATATYPE_NULL, MPI_ERR_TYPE, datatype)
  CHECK_NULL(2, MPI_ERR_ARG, size)
  *size = static_cast<int>(datatype->size());
  return MPI_SUCCESS;
}

int PMPI_Type_size_x(MPI_Datatype datatype, MPI_Count *size)
{
  CHECK_MPI_NULL(1, MPI_DATATYPE_NULL, MPI_ERR_TYPE, datatype)
  CHECK_NULL(2, MPI_ERR_ARG, size)
  *size = static_cast<MPI_Count>(datatype->size());
  return MPI_SUCCESS;
}

int PMPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint * lb, MPI_Aint * extent)
{
  CHECK_MPI_NULL(1, MPI_DATATYPE_NULL, MPI_ERR_TYPE, datatype)
  CHECK_NULL(2, MPI_ERR_ARG, lb)
  CHECK_NULL(3, MPI_ERR_ARG, extent)
  return datatype->extent(lb, extent);
}

int PMPI_Type_get_extent_x(MPI_Datatype datatype, MPI_Count * lb, MPI_Count * extent)
{
  MPI_Aint tmplb, tmpext;
  int ret = PMPI_Type_get_extent(datatype, &tmplb, &tmpext);
  *lb = static_cast<MPI_Count>(tmplb);
  *extent = static_cast<MPI_Count>(tmpext);
  return ret;
}

int PMPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint * lb, MPI_Aint * extent)
{
  return PMPI_Type_get_extent(datatype, lb, extent);
}

int PMPI_Type_get_true_extent_x(MPI_Datatype datatype, MPI_Count * lb, MPI_Count * extent)
{
  return PMPI_Type_get_extent_x(datatype, lb, extent);
}

int PMPI_Type_extent(MPI_Datatype datatype, MPI_Aint * extent)
{
  CHECK_MPI_NULL(1, MPI_DATATYPE_NULL, MPI_ERR_TYPE, datatype)
  CHECK_NULL(2, MPI_ERR_ARG, extent)
  *extent = datatype->get_extent();
  return MPI_SUCCESS;
}

int PMPI_Type_lb(MPI_Datatype datatype, MPI_Aint * disp)
{
  CHECK_MPI_NULL(1, MPI_DATATYPE_NULL, MPI_ERR_TYPE, datatype)
  CHECK_NULL(2, MPI_ERR_ARG, disp)
  *disp = datatype->lb();
  return MPI_SUCCESS;
}

int PMPI_Type_ub(MPI_Datatype datatype, MPI_Aint * disp)
{
  CHECK_MPI_NULL(1, MPI_DATATYPE_NULL, MPI_ERR_TYPE, datatype)
  CHECK_NULL(2, MPI_ERR_ARG, disp)
  *disp = datatype->ub();
  return MPI_SUCCESS;
}

int PMPI_Type_dup(MPI_Datatype datatype, MPI_Datatype *newtype){
  int retval = MPI_SUCCESS;
  CHECK_MPI_NULL(1, MPI_DATATYPE_NULL, MPI_ERR_TYPE, datatype)
  retval = datatype->clone(newtype);
  //error when duplicating, free the new datatype
  if(retval!=MPI_SUCCESS){
    simgrid::smpi::Datatype::unref(*newtype);
    *newtype = MPI_DATATYPE_NULL;
  }
  return retval;
}

int PMPI_Type_contiguous(int count, MPI_Datatype old_type, MPI_Datatype* new_type) {
  CHECK_COUNT(1, count)
  CHECK_MPI_NULL(2, MPI_DATATYPE_NULL, MPI_ERR_TYPE, old_type)
  CHECK_NULL(3, MPI_ERR_ARG, new_type)
  return simgrid::smpi::Datatype::create_contiguous(count, old_type, 0, new_type);
}

int PMPI_Type_commit(MPI_Datatype* datatype) {
  CHECK_NULL(1, MPI_ERR_ARG, datatype)
  CHECK_MPI_NULL(1, MPI_DATATYPE_NULL, MPI_ERR_TYPE, (*datatype))
  (*datatype)->commit();
  return MPI_SUCCESS;
}

int PMPI_Type_vector(int count, int blocklen, int stride, MPI_Datatype old_type, MPI_Datatype* new_type) {
  CHECK_COUNT(1, count)
  CHECK_NEGATIVE(2, MPI_ERR_ARG, blocklen)
  CHECK_MPI_NULL(4, MPI_DATATYPE_NULL, MPI_ERR_TYPE, old_type)
  return simgrid::smpi::Datatype::create_vector(count, blocklen, stride, old_type, new_type);
}

int PMPI_Type_hvector(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* new_type) {
  CHECK_COUNT(1, count)
  CHECK_NEGATIVE(2, MPI_ERR_ARG, blocklen)
  CHECK_MPI_NULL(4, MPI_DATATYPE_NULL, MPI_ERR_TYPE, old_type)
  return simgrid::smpi::Datatype::create_hvector(count, blocklen, stride, old_type, new_type);
}

int PMPI_Type_create_hvector(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* new_type) {
  return MPI_Type_hvector(count, blocklen, stride, old_type, new_type);
}

int PMPI_Type_indexed(int count, const int* blocklens, const int* indices, MPI_Datatype old_type, MPI_Datatype* new_type) {
  CHECK_COUNT(1, count)
  CHECK_MPI_NULL(4, MPI_DATATYPE_NULL, MPI_ERR_TYPE, old_type)
  return simgrid::smpi::Datatype::create_indexed(count, blocklens, indices, old_type, new_type);
}

int PMPI_Type_create_indexed(int count, const int* blocklens, const int* indices, MPI_Datatype old_type, MPI_Datatype* new_type) {
  CHECK_COUNT(1, count)
  CHECK_MPI_NULL(4, MPI_DATATYPE_NULL, MPI_ERR_TYPE, old_type)
  return simgrid::smpi::Datatype::create_indexed(count, blocklens, indices, old_type, new_type);
}

int PMPI_Type_create_indexed_block(int count, int blocklength, const int* indices, MPI_Datatype old_type,
                                   MPI_Datatype* new_type)
{
  CHECK_COUNT(1, count)
  CHECK_MPI_NULL(4, MPI_DATATYPE_NULL, MPI_ERR_TYPE, old_type)
  auto* blocklens = static_cast<int*>(xbt_malloc(blocklength * count * sizeof(int)));
  for (int i    = 0; i < count; i++)
    blocklens[i]=blocklength;
  int retval    = simgrid::smpi::Datatype::create_indexed(count, blocklens, indices, old_type, new_type);
  xbt_free(blocklens);
  return retval;
}

int PMPI_Type_hindexed(int count, const int* blocklens, const MPI_Aint* indices, MPI_Datatype old_type,
                       MPI_Datatype* new_type)
{
  CHECK_COUNT(1, count)
  CHECK_MPI_NULL(4, MPI_DATATYPE_NULL, MPI_ERR_TYPE, old_type)
  return simgrid::smpi::Datatype::create_hindexed(count, blocklens, indices, old_type, new_type);
}

int PMPI_Type_create_hindexed(int count, const int* blocklens, const MPI_Aint* indices, MPI_Datatype old_type,
                              MPI_Datatype* new_type) {
  return PMPI_Type_hindexed(count, blocklens, indices, old_type, new_type);
}

int PMPI_Type_create_hindexed_block(int count, int blocklength, const MPI_Aint* indices, MPI_Datatype old_type,
                                    MPI_Datatype* new_type) {
  CHECK_COUNT(1, count)
  CHECK_MPI_NULL(4, MPI_DATATYPE_NULL, MPI_ERR_TYPE, old_type)
  auto* blocklens = static_cast<int*>(xbt_malloc(blocklength * count * sizeof(int)));
  for (int i     = 0; i < count; i++)
    blocklens[i] = blocklength;
  int retval     = simgrid::smpi::Datatype::create_hindexed(count, blocklens, indices, old_type, new_type);
  xbt_free(blocklens);
  return retval;
}

int PMPI_Type_struct(int count, const int* blocklens, const MPI_Aint* indices, const MPI_Datatype* old_types,
                     MPI_Datatype* new_type)
{
  CHECK_COUNT(1, count)
  for(int i=0; i<count; i++)
    CHECK_MPI_NULL(4, MPI_DATATYPE_NULL, MPI_ERR_TYPE, old_types[i])
  return simgrid::smpi::Datatype::create_struct(count, blocklens, indices, old_types, new_type);
}

int PMPI_Type_create_struct(int count, const int* blocklens, const MPI_Aint* indices, const MPI_Datatype* old_types,
                            MPI_Datatype* new_type) {
  return PMPI_Type_struct(count, blocklens, indices, old_types, new_type);
}


int PMPI_Type_create_subarray(int ndims, const int* array_of_sizes,
                             const int* array_of_subsizes, const int* array_of_starts,
                             int order, MPI_Datatype oldtype, MPI_Datatype *newtype) {
  CHECK_NEGATIVE(1, MPI_ERR_COUNT, ndims)
  if (ndims==0){
    *newtype = MPI_DATATYPE_NULL;
    return MPI_SUCCESS;
  } else if (ndims==1){
    simgrid::smpi::Datatype::create_contiguous( array_of_subsizes[0], oldtype, array_of_starts[0]*oldtype->get_extent(), newtype);
    return MPI_SUCCESS;
  } else if (oldtype == MPI_DATATYPE_NULL || not oldtype->is_valid() ) {
    return MPI_ERR_TYPE;
  } else if (order != MPI_ORDER_FORTRAN && order != MPI_ORDER_C){
    return MPI_ERR_ARG;
  } else {
    return simgrid::smpi::Datatype::create_subarray(ndims, array_of_sizes, array_of_subsizes, array_of_starts, order, oldtype, newtype);
  }
}

int PMPI_Type_create_resized(MPI_Datatype oldtype,MPI_Aint lb, MPI_Aint extent, MPI_Datatype *newtype){
  CHECK_MPI_NULL(1, MPI_DATATYPE_NULL, MPI_ERR_TYPE, oldtype)
  return simgrid::smpi::Datatype::create_resized(oldtype, lb, extent, newtype);
}


int PMPI_Type_set_name(MPI_Datatype  datatype, const char * name)
{
  CHECK_MPI_NULL(1, MPI_DATATYPE_NULL, MPI_ERR_TYPE, datatype)
  CHECK_NULL(2, MPI_ERR_ARG, name)
  datatype->set_name(name);
  return MPI_SUCCESS;
}

int PMPI_Type_get_name(MPI_Datatype  datatype, char * name, int* len)
{
  CHECK_MPI_NULL(1, MPI_DATATYPE_NULL, MPI_ERR_TYPE, datatype)
  CHECK_NULL(2, MPI_ERR_ARG, name)
  datatype->get_name(name, len);
  return MPI_SUCCESS;
}

MPI_Datatype PMPI_Type_f2c(MPI_Fint datatype){
  if(datatype==-1)
    return MPI_DATATYPE_NULL;
  return static_cast<MPI_Datatype>(simgrid::smpi::F2C::f2c(datatype));
}

MPI_Fint PMPI_Type_c2f(MPI_Datatype datatype){
  if(datatype==MPI_DATATYPE_NULL)
    return -1;
  return datatype->c2f();
}

int PMPI_Type_get_attr (MPI_Datatype type, int type_keyval, void *attribute_val, int* flag)
{
  CHECK_MPI_NULL(1, MPI_DATATYPE_NULL, MPI_ERR_TYPE, type)
  return type->attr_get<simgrid::smpi::Datatype>(type_keyval, attribute_val, flag);
}

int PMPI_Type_set_attr (MPI_Datatype type, int type_keyval, void *attribute_val)
{
  CHECK_MPI_NULL(1, MPI_DATATYPE_NULL, MPI_ERR_TYPE, type)
  return type->attr_put<simgrid::smpi::Datatype>(type_keyval, attribute_val);
}

int PMPI_Type_get_contents (MPI_Datatype type, int max_integers, int max_addresses, 
                            int max_datatypes, int* array_of_integers, MPI_Aint* array_of_addresses, 
                            MPI_Datatype *array_of_datatypes)
{
  CHECK_MPI_NULL(1, MPI_DATATYPE_NULL, MPI_ERR_TYPE, type)
  CHECK_NEGATIVE(2, MPI_ERR_COUNT, max_integers)
  CHECK_NEGATIVE(3, MPI_ERR_COUNT, max_addresses)
  CHECK_NEGATIVE(4, MPI_ERR_COUNT, max_datatypes)
  if(max_integers>0)
    CHECK_NULL(5, MPI_ERR_ARG, array_of_integers)
  if(max_addresses!=0)
    CHECK_NULL(6, MPI_ERR_ARG, array_of_addresses)
  if(max_datatypes!=0)
    CHECK_NULL(7, MPI_ERR_ARG, array_of_datatypes)
  return type->get_contents(max_integers, max_addresses, max_datatypes,
                            array_of_integers, array_of_addresses, array_of_datatypes);
}

int PMPI_Type_get_envelope (MPI_Datatype type, int *num_integers, int *num_addresses, 
                            int *num_datatypes, int *combiner)
{
  CHECK_MPI_NULL(1, MPI_DATATYPE_NULL, MPI_ERR_TYPE, type)
  CHECK_NULL(2, MPI_ERR_ARG, num_integers)
  CHECK_NULL(3, MPI_ERR_ARG, num_addresses)
  CHECK_NULL(4, MPI_ERR_ARG, num_datatypes)
  CHECK_NULL(5, MPI_ERR_ARG, combiner)
  return type->get_envelope(num_integers, num_addresses, num_datatypes, combiner);
}

int PMPI_Type_delete_attr (MPI_Datatype type, int type_keyval)
{
  CHECK_MPI_NULL(1, MPI_DATATYPE_NULL, MPI_ERR_TYPE, type)
  return type->attr_delete<simgrid::smpi::Datatype>(type_keyval);
}

int PMPI_Type_create_keyval(MPI_Type_copy_attr_function* copy_fn, MPI_Type_delete_attr_function* delete_fn, int* keyval,
                            void* extra_state)
{
  smpi_copy_fn _copy_fn={nullptr,copy_fn,nullptr,nullptr,nullptr,nullptr};
  smpi_delete_fn _delete_fn={nullptr,delete_fn,nullptr,nullptr,nullptr,nullptr};
  return simgrid::smpi::Keyval::keyval_create<simgrid::smpi::Datatype>(_copy_fn, _delete_fn, keyval, extra_state);
}

int PMPI_Type_free_keyval(int* keyval) {
  return simgrid::smpi::Keyval::keyval_free<simgrid::smpi::Datatype>(keyval);
}

int PMPI_Unpack(const void* inbuf, int incount, int* position, void* outbuf, int outcount, MPI_Datatype type, MPI_Comm comm) {
  CHECK_NEGATIVE(2, MPI_ERR_COUNT, incount)
  CHECK_NEGATIVE(5, MPI_ERR_COUNT, outcount)
  CHECK_BUFFER(1, inbuf, incount)
  CHECK_BUFFER(4, outbuf, outcount)
  CHECK_TYPE(6, type)
  CHECK_COMM(7)
  return type->unpack(inbuf, incount, position, outbuf,outcount, comm);
}

int PMPI_Pack(const void* inbuf, int incount, MPI_Datatype type, void* outbuf, int outcount, int* position, MPI_Comm comm) {
  CHECK_NEGATIVE(2, MPI_ERR_COUNT, incount)
  CHECK_NEGATIVE(5, MPI_ERR_COUNT, outcount)
  CHECK_BUFFER(1, inbuf, incount)
  CHECK_BUFFER(4, outbuf, outcount)
  CHECK_TYPE(6, type)
  CHECK_COMM(7)
  return type->pack(inbuf == MPI_BOTTOM ? nullptr : inbuf, incount, outbuf, outcount, position, comm);
}

int PMPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int* size) {
  CHECK_NEGATIVE(1, MPI_ERR_COUNT, incount)
  CHECK_TYPE(2, datatype)
  CHECK_COMM(3)
  *size=incount*datatype->size();
  return MPI_SUCCESS;
}

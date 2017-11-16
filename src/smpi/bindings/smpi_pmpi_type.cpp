/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "smpi_datatype_derived.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smpi_pmpi);

/* PMPI User level calls */
extern "C" { // Obviously, the C MPI interface should use the C linkage

int PMPI_Type_free(MPI_Datatype * datatype)
{
  /* Free a predefined datatype is an error according to the standard, and should be checked for */
  if (*datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_ARG;
  } else {
    simgrid::smpi::Datatype::unref(*datatype);
    return MPI_SUCCESS;
  }
}

int PMPI_Type_size(MPI_Datatype datatype, int *size)
{
  if (datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (size == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *size = static_cast<int>(datatype->size());
    return MPI_SUCCESS;
  }
}

int PMPI_Type_size_x(MPI_Datatype datatype, MPI_Count *size)
{
  if (datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (size == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *size = static_cast<MPI_Count>(datatype->size());
    return MPI_SUCCESS;
  }
}

int PMPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint * lb, MPI_Aint * extent)
{
  if (datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (lb == nullptr || extent == nullptr) {
    return MPI_ERR_ARG;
  } else {
    return datatype->extent(lb, extent);
  }
}

int PMPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint * lb, MPI_Aint * extent)
{
  return PMPI_Type_get_extent(datatype, lb, extent);
}

int PMPI_Type_extent(MPI_Datatype datatype, MPI_Aint * extent)
{
  if (datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (extent == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *extent = datatype->get_extent();
    return MPI_SUCCESS;
  }
}

int PMPI_Type_lb(MPI_Datatype datatype, MPI_Aint * disp)
{
  if (datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (disp == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *disp = datatype->lb();
    return MPI_SUCCESS;
  }
}

int PMPI_Type_ub(MPI_Datatype datatype, MPI_Aint * disp)
{
  if (datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (disp == nullptr) {
    return MPI_ERR_ARG;
  } else {
    *disp = datatype->ub();
    return MPI_SUCCESS;
  }
}

int PMPI_Type_dup(MPI_Datatype datatype, MPI_Datatype *newtype){
  int retval = MPI_SUCCESS;
  if (datatype == MPI_DATATYPE_NULL) {
    retval=MPI_ERR_TYPE;
  } else {
    *newtype = new simgrid::smpi::Datatype(datatype, &retval);
    //error when duplicating, free the new datatype
    if(retval!=MPI_SUCCESS){
      simgrid::smpi::Datatype::unref(*newtype);
      *newtype = MPI_DATATYPE_NULL;
    }
  }
  return retval;
}

int PMPI_Type_contiguous(int count, MPI_Datatype old_type, MPI_Datatype* new_type) {
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0){
    return MPI_ERR_COUNT;
  } else {
    return simgrid::smpi::Datatype::create_contiguous(count, old_type, 0, new_type);
  }
}

int PMPI_Type_commit(MPI_Datatype* datatype) {
  if (datatype == nullptr || *datatype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else {
    (*datatype)->commit();
    return MPI_SUCCESS;
  }
}

int PMPI_Type_vector(int count, int blocklen, int stride, MPI_Datatype old_type, MPI_Datatype* new_type) {
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0 || blocklen<0){
    return MPI_ERR_COUNT;
  } else {
    return simgrid::smpi::Datatype::create_vector(count, blocklen, stride, old_type, new_type);
  }
}

int PMPI_Type_hvector(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* new_type) {
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0 || blocklen<0){
    return MPI_ERR_COUNT;
  } else {
    return simgrid::smpi::Datatype::create_hvector(count, blocklen, stride, old_type, new_type);
  }
}

int PMPI_Type_create_hvector(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* new_type) {
  return MPI_Type_hvector(count, blocklen, stride, old_type, new_type);
}

int PMPI_Type_indexed(int count, int* blocklens, int* indices, MPI_Datatype old_type, MPI_Datatype* new_type) {
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0){
    return MPI_ERR_COUNT;
  } else {
    return simgrid::smpi::Datatype::create_indexed(count, blocklens, indices, old_type, new_type);
  }
}

int PMPI_Type_create_indexed(int count, int* blocklens, int* indices, MPI_Datatype old_type, MPI_Datatype* new_type) {
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0){
    return MPI_ERR_COUNT;
  } else {
    return simgrid::smpi::Datatype::create_indexed(count, blocklens, indices, old_type, new_type);
  }
}

int PMPI_Type_create_indexed_block(int count, int blocklength, int* indices, MPI_Datatype old_type,
                                   MPI_Datatype* new_type)
{
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0){
    return MPI_ERR_COUNT;
  } else {
    int* blocklens=static_cast<int*>(xbt_malloc(blocklength*count*sizeof(int)));
    for (int i    = 0; i < count; i++)
      blocklens[i]=blocklength;
    int retval    = simgrid::smpi::Datatype::create_indexed(count, blocklens, indices, old_type, new_type);
    xbt_free(blocklens);
    return retval;
  }
}

int PMPI_Type_hindexed(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype old_type, MPI_Datatype* new_type)
{
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0){
    return MPI_ERR_COUNT;
  } else {
    return simgrid::smpi::Datatype::create_hindexed(count, blocklens, indices, old_type, new_type);
  }
}

int PMPI_Type_create_hindexed(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype old_type,
                              MPI_Datatype* new_type) {
  return PMPI_Type_hindexed(count, blocklens,indices,old_type,new_type);
}

int PMPI_Type_create_hindexed_block(int count, int blocklength, MPI_Aint* indices, MPI_Datatype old_type,
                                    MPI_Datatype* new_type) {
  if (old_type == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  } else if (count<0){
    return MPI_ERR_COUNT;
  } else {
    int* blocklens=(int*)xbt_malloc(blocklength*count*sizeof(int));
    for (int i     = 0; i < count; i++)
      blocklens[i] = blocklength;
    int retval     = simgrid::smpi::Datatype::create_hindexed(count, blocklens, indices, old_type, new_type);
    xbt_free(blocklens);
    return retval;
  }
}

int PMPI_Type_struct(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype* old_types, MPI_Datatype* new_type) {
  if (count<0){
    return MPI_ERR_COUNT;
  } else {
    return simgrid::smpi::Datatype::create_struct(count, blocklens, indices, old_types, new_type);
  }
}

int PMPI_Type_create_struct(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype* old_types,
                            MPI_Datatype* new_type) {
  return PMPI_Type_struct(count, blocklens, indices, old_types, new_type);
}

int PMPI_Type_create_resized(MPI_Datatype oldtype,MPI_Aint lb, MPI_Aint extent, MPI_Datatype *newtype){
  if (oldtype == MPI_DATATYPE_NULL) {
    return MPI_ERR_TYPE;
  }
  int blocks[3]         = {1, 1, 1};
  MPI_Aint disps[3]     = {lb, 0, lb + extent};
  MPI_Datatype types[3] = {MPI_LB, oldtype, MPI_UB};

  *newtype = new simgrid::smpi::Type_Struct(oldtype->size(), lb, lb + extent, DT_FLAG_DERIVED, 3, blocks, disps, types);

  (*newtype)->addflag(~DT_FLAG_COMMITED);
  return MPI_SUCCESS;
}


int PMPI_Type_set_name(MPI_Datatype  datatype, char * name)
{
  if (datatype == MPI_DATATYPE_NULL)  {
    return MPI_ERR_TYPE;
  } else if (name == nullptr)  {
    return MPI_ERR_ARG;
  } else {
    datatype->set_name(name);
    return MPI_SUCCESS;
  }
}

int PMPI_Type_get_name(MPI_Datatype  datatype, char * name, int* len)
{
  if (datatype == MPI_DATATYPE_NULL)  {
    return MPI_ERR_TYPE;
  } else if (name == nullptr)  {
    return MPI_ERR_ARG;
  } else {
    datatype->get_name(name, len);
    return MPI_SUCCESS;
  }
}

MPI_Datatype PMPI_Type_f2c(MPI_Fint datatype){
  return static_cast<MPI_Datatype>(simgrid::smpi::F2C::f2c(datatype));
}

MPI_Fint PMPI_Type_c2f(MPI_Datatype datatype){
  return datatype->c2f();
}

int PMPI_Type_get_attr (MPI_Datatype type, int type_keyval, void *attribute_val, int* flag)
{
  if (type==MPI_DATATYPE_NULL)
    return MPI_ERR_TYPE;
  else
    return type->attr_get<simgrid::smpi::Datatype>(type_keyval, attribute_val, flag);
}

int PMPI_Type_set_attr (MPI_Datatype type, int type_keyval, void *attribute_val)
{
  if (type==MPI_DATATYPE_NULL)
    return MPI_ERR_TYPE;
  else
    return type->attr_put<simgrid::smpi::Datatype>(type_keyval, attribute_val);
}

int PMPI_Type_delete_attr (MPI_Datatype type, int type_keyval)
{
  if (type==MPI_DATATYPE_NULL)
    return MPI_ERR_TYPE;
  else
    return type->attr_delete<simgrid::smpi::Datatype>(type_keyval);
}

int PMPI_Type_create_keyval(MPI_Type_copy_attr_function* copy_fn, MPI_Type_delete_attr_function* delete_fn, int* keyval,
                            void* extra_state)
{
  smpi_copy_fn _copy_fn={nullptr,copy_fn,nullptr};
  smpi_delete_fn _delete_fn={nullptr,delete_fn,nullptr};
  return simgrid::smpi::Keyval::keyval_create<simgrid::smpi::Datatype>(_copy_fn, _delete_fn, keyval, extra_state);
}

int PMPI_Type_free_keyval(int* keyval) {
  return simgrid::smpi::Keyval::keyval_free<simgrid::smpi::Datatype>(keyval);
}

int PMPI_Unpack(void* inbuf, int incount, int* position, void* outbuf, int outcount, MPI_Datatype type, MPI_Comm comm) {
  if(incount<0 || outcount < 0 || inbuf==nullptr || outbuf==nullptr)
    return MPI_ERR_ARG;
  if (not type->is_valid())
    return MPI_ERR_TYPE;
  if(comm==MPI_COMM_NULL)
    return MPI_ERR_COMM;
  return type->unpack(inbuf, incount, position, outbuf,outcount, comm);
}

int PMPI_Pack(void* inbuf, int incount, MPI_Datatype type, void* outbuf, int outcount, int* position, MPI_Comm comm) {
  if(incount<0 || outcount < 0|| inbuf==nullptr || outbuf==nullptr)
    return MPI_ERR_ARG;
  if (not type->is_valid())
    return MPI_ERR_TYPE;
  if(comm==MPI_COMM_NULL)
    return MPI_ERR_COMM;
  return type->pack(inbuf == MPI_BOTTOM ? nullptr : inbuf, incount, outbuf, outcount, position, comm);
}

int PMPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int* size) {
  if(incount<0)
    return MPI_ERR_ARG;
  if (not datatype->is_valid())
    return MPI_ERR_TYPE;
  if(comm==MPI_COMM_NULL)
    return MPI_ERR_COMM;

  *size=incount*datatype->size();

  return MPI_SUCCESS;
}

}

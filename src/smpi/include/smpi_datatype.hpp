/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_DATATYPE_HPP
#define SMPI_DATATYPE_HPP

#include "smpi_f2c.hpp"
#include "smpi_keyvals.hpp"

#define DT_FLAG_DESTROYED     0x0001  /**< user destroyed but some other layers still have a reference */
#define DT_FLAG_COMMITED      0x0002  /**< ready to be used for a send/recv operation */
#define DT_FLAG_CONTIGUOUS    0x0004  /**< contiguous datatype */
#define DT_FLAG_OVERLAP       0x0008  /**< datatype is unpropper for a recv operation */
#define DT_FLAG_USER_LB       0x0010  /**< has a user defined LB */
#define DT_FLAG_USER_UB       0x0020  /**< has a user defined UB */
#define DT_FLAG_PREDEFINED    0x0040  /**< cannot be removed: initial and predefined datatypes */
#define DT_FLAG_NO_GAPS       0x0080  /**< no gaps around the datatype */
#define DT_FLAG_DATA          0x0100  /**< data or control structure */
#define DT_FLAG_ONE_SIDED     0x0200  /**< datatype can be used for one sided operations */
#define DT_FLAG_UNAVAILABLE   0x0400  /**< datatypes unavailable on the build (OS or compiler dependant) */
#define DT_FLAG_DERIVED       0x0800  /**< is the datatype derived ? */
/*
 * We should make the difference here between the predefined contiguous and non contiguous
 * datatypes. The DT_FLAG_BASIC is held by all predefined contiguous datatypes.
 */
#define DT_FLAG_BASIC      (DT_FLAG_PREDEFINED | DT_FLAG_CONTIGUOUS | DT_FLAG_NO_GAPS | DT_FLAG_DATA | DT_FLAG_COMMITED)

extern const MPI_Datatype MPI_PTR;

//The following are datatypes for the MPI functions MPI_MAXLOC and MPI_MINLOC.
struct float_int {
  float value;
  int index;
};
struct float_float {
  float value;
  float index;
};
struct long_long {
  long value;
  long index;
};
struct double_double {
  double value;
  double index;
};
struct long_int {
  long value;
  int index;
};
struct double_int {
  double value;
  int index;
};
struct short_int {
  short value;
  int index;
};
struct int_int {
  int value;
  int index;
};
struct long_double_int {
  long double value;
  int index;
};
struct integer128_t {
  int64_t value;
  int64_t index;
};

namespace simgrid{
namespace smpi{

class Datatype : public F2C, public Keyval{
  char* name_;
  size_t size_;
  MPI_Aint lb_;
  MPI_Aint ub_;
  int flags_;
  int refcount_;

public:
  static std::unordered_map<int, smpi_key_elem> keyvals_;
  static int keyval_id_;

  Datatype(int size, MPI_Aint lb, MPI_Aint ub, int flags);
  Datatype(char* name, int size, MPI_Aint lb, MPI_Aint ub, int flags);
  Datatype(Datatype* datatype, int* ret);
  virtual ~Datatype();

  char* name();
  size_t size();
  MPI_Aint lb();
  MPI_Aint ub();
  int flags();
  int refcount();

  void ref();
  static void unref(MPI_Datatype datatype);
  void commit();
  bool is_valid();
  bool is_basic();
  bool is_replayable();
  void addflag(int flag);
  int extent(MPI_Aint* lb, MPI_Aint* extent);
  MPI_Aint get_extent();
  void get_name(char* name, int* length);
  void set_name(char* name);
  static int copy(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                  MPI_Datatype recvtype);
  virtual void serialize(void* noncontiguous, void* contiguous, int count);
  virtual void unserialize(void* contiguous, void* noncontiguous, int count, MPI_Op op);
  static int keyval_create(MPI_Type_copy_attr_function* copy_fn, MPI_Type_delete_attr_function* delete_fn, int* keyval,
                           void* extra_state);
  static int keyval_free(int* keyval);
  int pack(void* inbuf, int incount, void* outbuf, int outcount, int* position, MPI_Comm comm);
  int unpack(void* inbuf, int insize, int* position, void* outbuf, int outcount, MPI_Comm comm);

  static int create_contiguous(int count, MPI_Datatype old_type, MPI_Aint lb, MPI_Datatype* new_type);
  static int create_vector(int count, int blocklen, int stride, MPI_Datatype old_type, MPI_Datatype* new_type);
  static int create_hvector(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* new_type);
  static int create_indexed(int count, int* blocklens, int* indices, MPI_Datatype old_type, MPI_Datatype* new_type);
  static int create_hindexed(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype old_type,
                             MPI_Datatype* new_type);
  static int create_struct(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype* old_types,
                           MPI_Datatype* new_type);

  static Datatype* f2c(int id);
};

}
}

#endif

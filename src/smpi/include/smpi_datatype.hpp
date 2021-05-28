/* Copyright (c) 2009-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_DATATYPE_HPP
#define SMPI_DATATYPE_HPP

#include "smpi_f2c.hpp"
#include "smpi_keyvals.hpp"
#include <string>
#include <vector>

constexpr unsigned DT_FLAG_DESTROYED   = 0x00001; /**< user destroyed but some other layers still have a reference */
constexpr unsigned DT_FLAG_COMMITED    = 0x00002; /**< ready to be used for a send/recv operation */
constexpr unsigned DT_FLAG_CONTIGUOUS  = 0x00004; /**< contiguous datatype */
constexpr unsigned DT_FLAG_OVERLAP     = 0x00008; /**< datatype is unproper for a recv operation */
constexpr unsigned DT_FLAG_USER_LB     = 0x00010; /**< has a user defined LB */
constexpr unsigned DT_FLAG_USER_UB     = 0x00020; /**< has a user defined UB */
constexpr unsigned DT_FLAG_PREDEFINED  = 0x00040; /**< cannot be removed: initial and predefined datatypes */
constexpr unsigned DT_FLAG_NO_GAPS     = 0x00080; /**< no gaps around the datatype */
constexpr unsigned DT_FLAG_DATA        = 0x00100; /**< data or control structure */
constexpr unsigned DT_FLAG_ONE_SIDED   = 0x00200; /**< datatype can be used for one sided operations */
constexpr unsigned DT_FLAG_UNAVAILABLE = 0x00400; /**< datatypes unavailable on the build (OS or compiler dependent) */
constexpr unsigned DT_FLAG_DERIVED     = 0x00800; /**< is the datatype derived ? */
constexpr unsigned DT_FLAG_C_INTEGER   = 0x01000; /**< Family: C ints */
constexpr unsigned DT_FLAG_F_INTEGER   = 0x02000; /**< Family: F ints */
constexpr unsigned DT_FLAG_FP          = 0x04000; /**< Family: Floating point */
constexpr unsigned DT_FLAG_LOGICAL     = 0x08000; /**< Family: Logical */
constexpr unsigned DT_FLAG_COMPLEX     = 0x10000; /**< Family: Complex */
constexpr unsigned DT_FLAG_BYTE        = 0x20000; /**< Family: Poor lonesome byte */
constexpr unsigned DT_FLAG_MULTILANG   = 0x40000; /**< Family: Multi-language */
constexpr unsigned DT_FLAG_REDUCTION   = 0x80000; /**< Family: Dual types for maxloc/minloc */
/*
 * We should make the difference here between the predefined contiguous and non contiguous
 * datatypes. The DT_FLAG_BASIC is held by all predefined contiguous datatypes.
 */
constexpr unsigned DT_FLAG_BASIC =
    (DT_FLAG_PREDEFINED | DT_FLAG_CONTIGUOUS | DT_FLAG_NO_GAPS | DT_FLAG_DATA | DT_FLAG_COMMITED);

extern MPI_Datatype MPI_PTR;

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
class Datatype_contents {
  public:
  int combiner_;
  std::vector<int> integers_;
  std::vector<MPI_Aint> addresses_;
  std::vector<MPI_Datatype> datatypes_;
  Datatype_contents(int combiner, 
                    int number_of_integers, const int* integers, 
                    int number_of_addresses, const MPI_Aint* addresses, 
                    int number_of_datatypes, const MPI_Datatype* datatypes);
  ~Datatype_contents();
};

class Datatype : public F2C, public Keyval{
  std::string name_ = "";
  /* The id here is the (unique) datatype id used for this datastructure.
   * It's default value is set to -1 since some code expects this return value
   * when no other id has been assigned
   */
  std::string id = "-1";
  size_t size_;
  MPI_Aint lb_;
  MPI_Aint ub_;
  int flags_;
  int refcount_ = 1;
  std::unique_ptr<Datatype_contents> contents_ = nullptr;
  MPI_Datatype duplicated_datatype_ = MPI_DATATYPE_NULL;

protected:
  template <typename... Args> void set_contents(Args&&... args)
  {
    contents_ = std::make_unique<Datatype_contents>(std::forward<Args>(args)...);
  }

public:
  static std::unordered_map<int, smpi_key_elem> keyvals_;
  static int keyval_id_;

  Datatype(int ident, int size, MPI_Aint lb, MPI_Aint ub, int flags);
  Datatype(const char* name, int ident, int size, MPI_Aint lb, MPI_Aint ub, int flags);
  Datatype(int size, MPI_Aint lb, MPI_Aint ub, int flags);
  Datatype(Datatype* datatype, int* ret);
  Datatype(const Datatype&) = delete;
  Datatype& operator=(const Datatype&) = delete;
  ~Datatype() override;
  std::string name() const override {return name_.empty() ? std::string("MPI_Datatype") : name_;}
  size_t size() const { return size_; }
  MPI_Aint lb() const { return lb_; }
  MPI_Aint ub() const { return ub_; }
  int flags() const { return flags_; }
  int refcount() const { return refcount_; }
  MPI_Datatype duplicated_datatype() const { return duplicated_datatype_; }

  void ref();
  static void unref(MPI_Datatype datatype);
  void commit();
  int copy_attrs(Datatype* datatype);
  bool is_valid() const;
  bool is_basic() const;
  static const char* encode(const Datatype* dt) { return dt->id.c_str(); }
  static MPI_Datatype decode(const std::string& datatype_id);
  bool is_replayable() const;
  void addflag(int flag);
  int extent(MPI_Aint* lb, MPI_Aint* extent) const;
  MPI_Aint get_extent() const { return ub_ - lb_; };
  void get_name(char* name, int* length) const;
  void set_name(const char* name);
  static int copy(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                  MPI_Datatype recvtype);
  virtual int clone(MPI_Datatype* type);
  virtual void serialize(const void* noncontiguous, void* contiguous, int count);
  virtual void unserialize(const void* contiguous, void* noncontiguous, int count, MPI_Op op);
  static int keyval_create(MPI_Type_copy_attr_function* copy_fn, MPI_Type_delete_attr_function* delete_fn, int* keyval,
                           void* extra_state);
  static int keyval_free(int* keyval);
  int pack(const void* inbuf, int incount, void* outbuf, int outcount, int* position, const Comm* comm);
  int unpack(const void* inbuf, int insize, int* position, void* outbuf, int outcount, const Comm* comm);
  int get_contents(int max_integers, int max_addresses, int max_datatypes, int* array_of_integers,
                   MPI_Aint* array_of_addresses, MPI_Datatype* array_of_datatypes) const;
  int get_envelope(int* num_integers, int* num_addresses, int* num_datatypes, int* combiner) const;
  static int create_contiguous(int count, MPI_Datatype old_type, MPI_Aint lb, MPI_Datatype* new_type);
  static int create_vector(int count, int blocklen, int stride, MPI_Datatype old_type, MPI_Datatype* new_type);
  static int create_hvector(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* new_type);
  static int create_indexed(int count, const int* blocklens, const int* indices, MPI_Datatype old_type, MPI_Datatype* new_type);
  static int create_hindexed(int count, const int* blocklens, const MPI_Aint* indices, MPI_Datatype old_type,
                             MPI_Datatype* new_type);
  static int create_struct(int count, const int* blocklens, const MPI_Aint* indices, const MPI_Datatype* old_types,
                           MPI_Datatype* new_type);
  static int create_subarray(int ndims, const int* array_of_sizes,
                             const int* array_of_subsizes, const int* array_of_starts,
                             int order, MPI_Datatype oldtype, MPI_Datatype *newtype);
  static int create_resized(MPI_Datatype oldtype,MPI_Aint lb, MPI_Aint extent,
                             MPI_Datatype *newtype);
  static Datatype* f2c(int id);
};

}
}

#endif

/* smpi_datatype.cpp -- MPI primitives to handle datatypes                  */
/* Copyright (c) 2009-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "simgrid/modelchecker.h"
#include "smpi_datatype_derived.hpp"
#include "smpi_op.hpp"
#include "src/instr/instr_private.hpp"
#include "src/smpi/include/smpi_actor.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <string>
#include <complex>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_datatype, smpi, "Logging specific to SMPI (datatype)");

static std::unordered_map<std::string, simgrid::smpi::Datatype*> id2type_lookup;

#define CREATE_MPI_DATATYPE(name, id, type, flag)                                                                            \
  simgrid::smpi::Datatype _XBT_CONCAT(smpi_MPI_, name)((char*)"MPI_"#name, (id), sizeof(type), /* size */   \
                                                         0,                                               /* lb */     \
                                                         sizeof(type), /* ub = lb + size */                            \
                                                         DT_FLAG_BASIC | flag /* flags */                                     \
                                                         );

#define CREATE_MPI_DATATYPE_NULL(name, id)                                                                             \
  simgrid::smpi::Datatype _XBT_CONCAT(smpi_MPI_, name)((char*)"MPI_"#name, (id), 0, /* size */              \
                                                         0,                                    /* lb */                \
                                                         0,                                    /* ub = lb + size */    \
                                                         DT_FLAG_BASIC                         /* flags */             \
                                                         );

// Predefined data types
CREATE_MPI_DATATYPE_NULL(DATATYPE_NULL, -1)
CREATE_MPI_DATATYPE(DOUBLE, 0, double, DT_FLAG_FP)
CREATE_MPI_DATATYPE(INT, 1, int, DT_FLAG_C_INTEGER)
CREATE_MPI_DATATYPE(CHAR, 2, char, DT_FLAG_C_INTEGER)
CREATE_MPI_DATATYPE(SHORT, 3, short, DT_FLAG_C_INTEGER)
CREATE_MPI_DATATYPE(LONG, 4, long, DT_FLAG_C_INTEGER)
CREATE_MPI_DATATYPE(FLOAT, 5, float, DT_FLAG_FP)
CREATE_MPI_DATATYPE(BYTE, 6, int8_t, DT_FLAG_BYTE)
CREATE_MPI_DATATYPE(LONG_LONG, 7, long long, DT_FLAG_C_INTEGER)
CREATE_MPI_DATATYPE(SIGNED_CHAR, 8, signed char, DT_FLAG_C_INTEGER)
CREATE_MPI_DATATYPE(UNSIGNED_CHAR, 9, unsigned char, DT_FLAG_C_INTEGER)
CREATE_MPI_DATATYPE(UNSIGNED_SHORT, 10, unsigned short, DT_FLAG_C_INTEGER)
CREATE_MPI_DATATYPE(UNSIGNED, 11, unsigned int, DT_FLAG_C_INTEGER)
CREATE_MPI_DATATYPE(UNSIGNED_LONG, 12, unsigned long, DT_FLAG_C_INTEGER)
CREATE_MPI_DATATYPE(UNSIGNED_LONG_LONG, 13, unsigned long long, DT_FLAG_C_INTEGER)
CREATE_MPI_DATATYPE(LONG_DOUBLE, 14, long double, DT_FLAG_FP)
CREATE_MPI_DATATYPE(WCHAR, 15, wchar_t, DT_FLAG_BASIC)
CREATE_MPI_DATATYPE(C_BOOL, 16, bool, DT_FLAG_LOGICAL)
CREATE_MPI_DATATYPE(INT8_T, 17, int8_t, DT_FLAG_C_INTEGER)
CREATE_MPI_DATATYPE(INT16_T, 18, int16_t, DT_FLAG_C_INTEGER)
CREATE_MPI_DATATYPE(INT32_T, 19, int32_t, DT_FLAG_C_INTEGER)
CREATE_MPI_DATATYPE(INT64_T, 20, int64_t, DT_FLAG_C_INTEGER)
CREATE_MPI_DATATYPE(UINT8_T, 21, uint8_t, DT_FLAG_C_INTEGER)
CREATE_MPI_DATATYPE(UINT16_T, 22, uint16_t, DT_FLAG_C_INTEGER)
CREATE_MPI_DATATYPE(UINT32_T, 23, uint32_t, DT_FLAG_C_INTEGER)
CREATE_MPI_DATATYPE(UINT64_T, 24, uint64_t, DT_FLAG_C_INTEGER)
CREATE_MPI_DATATYPE(C_FLOAT_COMPLEX, 25, float _Complex, DT_FLAG_COMPLEX)
CREATE_MPI_DATATYPE(C_DOUBLE_COMPLEX, 26, double _Complex, DT_FLAG_COMPLEX)
CREATE_MPI_DATATYPE(C_LONG_DOUBLE_COMPLEX, 27, long double _Complex, DT_FLAG_COMPLEX)
CREATE_MPI_DATATYPE(AINT, 28, MPI_Aint, DT_FLAG_MULTILANG)
CREATE_MPI_DATATYPE(OFFSET, 29, MPI_Offset, DT_FLAG_MULTILANG)

CREATE_MPI_DATATYPE(FLOAT_INT, 30, float_int, DT_FLAG_REDUCTION)
CREATE_MPI_DATATYPE(LONG_INT, 31, long_int, DT_FLAG_REDUCTION)
CREATE_MPI_DATATYPE(DOUBLE_INT, 32, double_int, DT_FLAG_REDUCTION)
CREATE_MPI_DATATYPE(SHORT_INT, 33, short_int, DT_FLAG_REDUCTION)
CREATE_MPI_DATATYPE(2INT, 34, int_int, DT_FLAG_REDUCTION)
CREATE_MPI_DATATYPE(2FLOAT, 35, float_float, DT_FLAG_REDUCTION)
CREATE_MPI_DATATYPE(2DOUBLE, 36, double_double, DT_FLAG_REDUCTION)
CREATE_MPI_DATATYPE(2LONG, 37, long_long, DT_FLAG_REDUCTION)

CREATE_MPI_DATATYPE(REAL, 38, float, DT_FLAG_FP)
CREATE_MPI_DATATYPE(REAL4, 39, float, DT_FLAG_FP)
CREATE_MPI_DATATYPE(REAL8, 40, double, DT_FLAG_FP)
CREATE_MPI_DATATYPE(REAL16, 41, long double, DT_FLAG_FP)
CREATE_MPI_DATATYPE(COMPLEX8, 42, float_float, DT_FLAG_COMPLEX)
CREATE_MPI_DATATYPE(COMPLEX16, 43, double_double, DT_FLAG_COMPLEX)
CREATE_MPI_DATATYPE(COMPLEX32, 44, double_double, DT_FLAG_COMPLEX)
CREATE_MPI_DATATYPE(INTEGER1, 45, int, DT_FLAG_F_INTEGER)
CREATE_MPI_DATATYPE(INTEGER2, 46, int16_t, DT_FLAG_F_INTEGER)
CREATE_MPI_DATATYPE(INTEGER4, 47, int32_t, DT_FLAG_F_INTEGER)
CREATE_MPI_DATATYPE(INTEGER8, 48, int64_t, DT_FLAG_F_INTEGER)
CREATE_MPI_DATATYPE(INTEGER16, 49, integer128_t, DT_FLAG_F_INTEGER)

CREATE_MPI_DATATYPE(LONG_DOUBLE_INT, 50, long_double_int, DT_FLAG_REDUCTION)
CREATE_MPI_DATATYPE(CXX_BOOL, 51, bool, DT_FLAG_LOGICAL)
CREATE_MPI_DATATYPE(CXX_FLOAT_COMPLEX, 52, std::complex<float>, DT_FLAG_COMPLEX)
CREATE_MPI_DATATYPE(CXX_DOUBLE_COMPLEX, 53, std::complex<double>, DT_FLAG_COMPLEX)
CREATE_MPI_DATATYPE(CXX_LONG_DOUBLE_COMPLEX, 54, std::complex<long double>, DT_FLAG_COMPLEX)

CREATE_MPI_DATATYPE_NULL(UB, 55)
CREATE_MPI_DATATYPE_NULL(LB, 56)
CREATE_MPI_DATATYPE(PACKED, 57, char, DT_FLAG_PREDEFINED)
// Internal use only
CREATE_MPI_DATATYPE(PTR, 58, void*, DT_FLAG_PREDEFINED)
CREATE_MPI_DATATYPE(COUNT, 59, long long, DT_FLAG_MULTILANG)
MPI_Datatype MPI_PTR = &smpi_MPI_PTR;


namespace simgrid{
namespace smpi{

std::unordered_map<int, smpi_key_elem> Datatype::keyvals_; // required by the Keyval class implementation
int Datatype::keyval_id_=0; // required by the Keyval class implementation
Datatype::Datatype(int ident, int size, MPI_Aint lb, MPI_Aint ub, int flags) : Datatype(size, lb, ub, flags)
{
  id = std::to_string(ident);
}

Datatype::Datatype(int size, MPI_Aint lb, MPI_Aint ub, int flags) : size_(size), lb_(lb), ub_(ub), flags_(flags)
{
  this->add_f();
#if SIMGRID_HAVE_MC
  if(MC_is_active())
    MC_ignore(&refcount_, sizeof refcount_);
#endif
}

// for predefined types, so refcount_ = 0.
Datatype::Datatype(const char* name, int ident, int size, MPI_Aint lb, MPI_Aint ub, int flags)
    : name_(name), id(std::to_string(ident)), size_(size), lb_(lb), ub_(ub), flags_(flags), refcount_(0)
{
  id2type_lookup.insert({id, this});
#if SIMGRID_HAVE_MC
  if(MC_is_active())
    MC_ignore(&refcount_, sizeof refcount_);
#endif
}

Datatype::Datatype(Datatype* datatype, int* ret)
    : size_(datatype->size_), lb_(datatype->lb_), ub_(datatype->ub_), flags_(datatype->flags_)
{
  this->add_f();
  *ret = this->copy_attrs(datatype);
}

Datatype::~Datatype()
{
  xbt_assert(refcount_ >= 0);

  if(flags_ & DT_FLAG_PREDEFINED)
    return;
  //prevent further usage
  flags_ &= ~ DT_FLAG_COMMITED;
  F2C::free_f(this->f2c_id());
  //if still used, mark for deletion
  if(refcount_!=0){
      flags_ |=DT_FLAG_DESTROYED;
      return;
  }
  cleanup_attr<Datatype>();
}

int Datatype::copy_attrs(Datatype* datatype){
  flags_ &= ~DT_FLAG_PREDEFINED;
  int ret = MPI_SUCCESS;

  for (auto const& it : datatype->attributes()) {
    auto elem_it = keyvals_.find(it.first);
    if (elem_it != keyvals_.end()) {
      smpi_key_elem& elem = elem_it->second;
      int flag            = 0;
      void* value_out     = nullptr;
      if (elem.copy_fn.type_copy_fn == MPI_TYPE_DUP_FN) {
        value_out = it.second;
        flag      = 1;
      } else if (elem.copy_fn.type_copy_fn != MPI_NULL_COPY_FN) {
        ret = elem.copy_fn.type_copy_fn(datatype, it.first, elem.extra_state, it.second, &value_out, &flag);
      }
      if (elem.copy_fn.type_copy_fn_fort != MPI_NULL_COPY_FN) {
        value_out = xbt_new(int, 1);
        if (*(int*)*elem.copy_fn.type_copy_fn_fort == 1) { // MPI_TYPE_DUP_FN
          memcpy(value_out, it.second, sizeof(int));
          flag = 1;
        } else { // not null, nor dup
          elem.copy_fn.type_copy_fn_fort(datatype, it.first, elem.extra_state, it.second, value_out, &flag, &ret);
          if (ret != MPI_SUCCESS)
            xbt_free(value_out);
        }
      }
      if (ret != MPI_SUCCESS) {
        break;
      }
      if (flag) {
        elem.refcount++;
        attributes().emplace(it.first, value_out);
      }
    }
  }
  set_contents(MPI_COMBINER_DUP, 0, nullptr, 0, nullptr, 1, &datatype);
  return ret;
}

int Datatype::clone(MPI_Datatype* type){
  int ret;
  *type = new Datatype(this, &ret);
  return ret;
}

void Datatype::ref()
{
  refcount_++;

#if SIMGRID_HAVE_MC
  if(MC_is_active())
    MC_ignore(&refcount_, sizeof refcount_);
#endif
}

void Datatype::unref(MPI_Datatype datatype)
{
  if (datatype->refcount_ > 0)
    datatype->refcount_--;

#if SIMGRID_HAVE_MC
  if(MC_is_active())
    MC_ignore(&datatype->refcount_, sizeof datatype->refcount_);
#endif

  if (datatype->refcount_ == 0 && not(datatype->flags_ & DT_FLAG_PREDEFINED))
    delete datatype;
}

void Datatype::commit()
{
  flags_ |= DT_FLAG_COMMITED;
}

bool Datatype::is_valid() const
{
  return (flags_ & DT_FLAG_COMMITED);
}

bool Datatype::is_basic() const
{
  return (flags_ & DT_FLAG_BASIC);
}

bool Datatype::is_replayable() const
{
  return (simgrid::instr::trace_format == simgrid::instr::TraceFormat::Ti) &&
         ((this == MPI_BYTE) || (this == MPI_DOUBLE) || (this == MPI_INT) || (this == MPI_CHAR) ||
          (this == MPI_SHORT) || (this == MPI_LONG) || (this == MPI_FLOAT));
}

MPI_Datatype Datatype::decode(const std::string& datatype_id)
{
  return id2type_lookup.find(datatype_id)->second;
}

void Datatype::addflag(int flag){
  flags_ &= flag;
}

int Datatype::extent(MPI_Aint* lb, MPI_Aint* extent) const
{
  *lb = lb_;
  *extent = ub_ - lb_;
  return MPI_SUCCESS;
}

void Datatype::get_name(char* name, int* length) const
{
  *length = static_cast<int>(name_.length());
  if (not name_.empty()) {
    name_.copy(name, *length);
    name[*length] = '\0';
  }
}

void Datatype::set_name(const char* name)
{
  name_ = name;
}

int Datatype::pack(const void* inbuf, int incount, void* outbuf, int outcount, int* position, const Comm*)
{
  if (outcount - *position < incount*static_cast<int>(size_))
    return MPI_ERR_OTHER;
  Datatype::copy(inbuf, incount, this, static_cast<char*>(outbuf) + *position, outcount, MPI_CHAR);
  *position += incount * size_;
  return MPI_SUCCESS;
}

int Datatype::unpack(const void* inbuf, int insize, int* position, void* outbuf, int outcount, const Comm*)
{
  if (outcount*static_cast<int>(size_)> insize)
    return MPI_ERR_OTHER;
  Datatype::copy(static_cast<const char*>(inbuf) + *position, insize, MPI_CHAR, outbuf, outcount, this);
  *position += outcount * size_;
  return MPI_SUCCESS;
}

int Datatype::get_contents(int max_integers, int max_addresses, int max_datatypes, int* array_of_integers,
                           MPI_Aint* array_of_addresses, MPI_Datatype* array_of_datatypes) const
{
  if(contents_==nullptr)
    return MPI_ERR_ARG;
  if (static_cast<unsigned>(max_integers) < contents_->integers_.size())
    return MPI_ERR_COUNT;
  std::copy(begin(contents_->integers_), end(contents_->integers_), array_of_integers);
  if (static_cast<unsigned>(max_addresses) < contents_->addresses_.size())
    return MPI_ERR_COUNT;
  std::copy(begin(contents_->addresses_), end(contents_->addresses_), array_of_addresses);
  if (static_cast<unsigned>(max_datatypes) < contents_->datatypes_.size())
    return MPI_ERR_COUNT;
  std::copy(begin(contents_->datatypes_), end(contents_->datatypes_), array_of_datatypes);
  std::for_each(begin(contents_->datatypes_), end(contents_->datatypes_), std::mem_fn(&Datatype::ref));
  return MPI_SUCCESS;
}

int Datatype::get_envelope(int* num_integers, int* num_addresses, int* num_datatypes, int* combiner) const
{
  if(contents_==nullptr){
    *num_integers = 0;
    *num_addresses = 0;
    *num_datatypes = 0;
    *combiner = MPI_COMBINER_NAMED;
  }else{
    *num_integers  = contents_->integers_.size();
    *num_addresses = contents_->addresses_.size();
    *num_datatypes = contents_->datatypes_.size();
    *combiner = contents_->combiner_;
  }
  return MPI_SUCCESS;
}

int Datatype::copy(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                   MPI_Datatype recvtype)
{
  // FIXME Handle the case of a partial shared malloc.

  if (smpi_cfg_privatization() == SmpiPrivStrategies::MMAP) {
    smpi_switch_data_segment(simgrid::s4u::Actor::self());
  }
  /* First check if we really have something to do */
  size_t offset = 0;
  std::vector<std::pair<size_t, size_t>> private_blocks;
  if(smpi_is_shared(sendbuf,private_blocks,&offset)
       && (private_blocks.size()==1
       && (private_blocks[0].second - private_blocks[0].first)==(unsigned long)(sendcount * sendtype->get_extent()))){
    XBT_VERB("sendbuf is shared. Ignoring copies");
    return 0;
  }
  if(smpi_is_shared(recvbuf,private_blocks,&offset)
       && (private_blocks.size()==1
       && (private_blocks[0].second - private_blocks[0].first)==(unsigned long)(recvcount * recvtype->get_extent()))){
    XBT_VERB("recvbuf is shared. Ignoring copies");
    return 0;
  }

  if (recvcount > 0 && recvbuf != sendbuf) {
    sendcount *= sendtype->size();
    recvcount *= recvtype->size();
    int count = sendcount < recvcount ? sendcount : recvcount;
    XBT_DEBUG("Copying %d bytes from %p to %p", count, sendbuf, recvbuf);
    if (not(sendtype->flags() & DT_FLAG_DERIVED) && not(recvtype->flags() & DT_FLAG_DERIVED)) {
      if (not smpi_process()->replaying() && count > 0)
        memcpy(recvbuf, sendbuf, count);
    } else if (not(sendtype->flags() & DT_FLAG_DERIVED)) {
      recvtype->unserialize(sendbuf, recvbuf, count / recvtype->size(), MPI_REPLACE);
    } else if (not(recvtype->flags() & DT_FLAG_DERIVED)) {
      sendtype->serialize(sendbuf, recvbuf, count / sendtype->size());
    } else {
      void * buf_tmp = xbt_malloc(count);

      sendtype->serialize( sendbuf, buf_tmp,count/sendtype->size());
      recvtype->unserialize( buf_tmp, recvbuf,count/recvtype->size(), MPI_REPLACE);

      xbt_free(buf_tmp);
    }
  }

  return sendcount > recvcount ? MPI_ERR_TRUNCATE : MPI_SUCCESS;
}

//Default serialization method : memcpy.
void Datatype::serialize(const void* noncontiguous_buf, void* contiguous_buf, int count)
{
  auto* contiguous_buf_char          = static_cast<char*>(contiguous_buf);
  const auto* noncontiguous_buf_char = static_cast<const char*>(noncontiguous_buf) + lb_;
  memcpy(contiguous_buf_char, noncontiguous_buf_char, count*size_);
}

void Datatype::unserialize(const void* contiguous_buf, void *noncontiguous_buf, int count, MPI_Op op){
  const auto* contiguous_buf_char = static_cast<const char*>(contiguous_buf);
  auto* noncontiguous_buf_char    = static_cast<char*>(noncontiguous_buf) + lb_;
  int n=count;
  if(op!=MPI_OP_NULL)
    op->apply( contiguous_buf_char, noncontiguous_buf_char, &n, this);
}

int Datatype::create_contiguous(int count, MPI_Datatype old_type, MPI_Aint lb, MPI_Datatype* new_type){
  if(old_type->flags_ & DT_FLAG_DERIVED){
    //handle this case as a hvector with stride equals to the extent of the datatype
    return create_hvector(count, 1, old_type->get_extent(), old_type, new_type);
  }
  if(count>0)
    *new_type = new Type_Contiguous(count * old_type->size(), lb, lb + count * old_type->size(),
                                   DT_FLAG_DERIVED, count, old_type);
  else
    *new_type = new Datatype(count * old_type->size(), lb, lb + count * old_type->size(),0);
  return MPI_SUCCESS;
}

int Datatype::create_vector(int count, int block_length, int stride, MPI_Datatype old_type, MPI_Datatype* new_type)
{
  int retval;
  if (block_length<0)
    return MPI_ERR_ARG;
  MPI_Aint lb = 0;
  MPI_Aint ub = 0;
  if(count>0){
    lb=old_type->lb();
    ub=((count-1)*stride+block_length-1)*old_type->get_extent()+old_type->ub();
  }
  if(old_type->flags() & DT_FLAG_DERIVED || stride != block_length){
    *new_type = new Type_Vector(count * block_length * old_type->size(), lb, ub, DT_FLAG_DERIVED, count, block_length,
                                stride, old_type);
    retval=MPI_SUCCESS;
  }else{
    /* in this situation the data are contiguous thus it's not required to serialize and unserialize it*/
    *new_type = new Datatype(count * block_length * old_type->size(), 0, ((count -1) * stride + block_length)*
                         old_type->size(), DT_FLAG_CONTIGUOUS);
    const std::array<int, 3> ints = {{count, block_length, stride}};
    (*new_type)->set_contents(MPI_COMBINER_VECTOR, 3, ints.data(), 0, nullptr, 1, &old_type);
    retval=MPI_SUCCESS;
  }
  return retval;
}


int Datatype::create_hvector(int count, int block_length, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* new_type)
{
  int retval;
  if (block_length<0)
    return MPI_ERR_ARG;
  MPI_Aint lb = 0;
  MPI_Aint ub = 0;
  if(count>0){
    lb=old_type->lb();
    ub=((count-1)*stride)+(block_length-1)*old_type->get_extent()+old_type->ub();
  }
  if(old_type->flags() & DT_FLAG_DERIVED || stride != block_length*old_type->get_extent()){
    *new_type = new Type_Hvector(count * block_length * old_type->size(), lb, ub, DT_FLAG_DERIVED, count, block_length,
                                 stride, old_type);
    retval=MPI_SUCCESS;
  }else{
    /* in this situation the data are contiguous thus it's not required to serialize and unserialize it*/
    *new_type = new Datatype(count * block_length * old_type->size(), 0, count * block_length * old_type->size(), DT_FLAG_CONTIGUOUS);
    const std::array<int, 2> ints = {{count, block_length}};
    (*new_type)->set_contents(MPI_COMBINER_HVECTOR, 2, ints.data(), 1, &stride, 1, &old_type);
    retval=MPI_SUCCESS;
  }
  return retval;
}

int Datatype::create_indexed(int count, const int* block_lengths, const int* indices, MPI_Datatype old_type, MPI_Datatype* new_type){
  int size = 0;
  bool contiguous=true;
  MPI_Aint lb = 0;
  MPI_Aint ub = 0;
  if(count>0){
    lb=indices[0]*old_type->get_extent();
    ub=indices[0]*old_type->get_extent() + block_lengths[0]*old_type->ub();
  }

  for (int i = 0; i < count; i++) {
    if (block_lengths[i] < 0)
      return MPI_ERR_ARG;
    size += block_lengths[i];

    if(indices[i]*old_type->get_extent()+old_type->lb()<lb)
      lb = indices[i]*old_type->get_extent()+old_type->lb();
    if(indices[i]*old_type->get_extent()+block_lengths[i]*old_type->ub()>ub)
      ub = indices[i]*old_type->get_extent()+block_lengths[i]*old_type->ub();

    if ( (i< count -1) && (indices[i]+block_lengths[i] != indices[i+1]) )
      contiguous=false;
  }
  if(old_type->flags_ & DT_FLAG_DERIVED)
    contiguous=false;

  if (not contiguous) {
    *new_type = new Type_Indexed(size * old_type->size(),lb,ub,
                                 DT_FLAG_DERIVED|DT_FLAG_DATA, count, block_lengths, indices, old_type);
  }else{
    Datatype::create_contiguous(size, old_type, lb, new_type);
  }
  return MPI_SUCCESS;
}

int Datatype::create_hindexed(int count, const int* block_lengths, const MPI_Aint* indices, MPI_Datatype old_type, MPI_Datatype* new_type){
  int size = 0;
  bool contiguous=true;
  MPI_Aint lb = 0;
  MPI_Aint ub = 0;
  if(count>0){
    lb=indices[0] + old_type->lb();
    ub=indices[0] + block_lengths[0]*old_type->ub();
  }
  for (int i = 0; i < count; i++) {
    if (block_lengths[i] < 0)
      return MPI_ERR_ARG;
    size += block_lengths[i];

    if(indices[i]+old_type->lb()<lb)
      lb = indices[i]+old_type->lb();
    if(indices[i]+block_lengths[i]*old_type->ub()>ub)
      ub = indices[i]+block_lengths[i]*old_type->ub();

    if ( (i< count -1) && (indices[i]+block_lengths[i]*(static_cast<int>(old_type->size())) != indices[i+1]) )
      contiguous=false;
  }
  if (old_type->flags_ & DT_FLAG_DERIVED || lb!=0)
    contiguous=false;

  if (not contiguous) {
    *new_type = new Type_Hindexed(size * old_type->size(),lb,ub,
                                   DT_FLAG_DERIVED|DT_FLAG_DATA, count, block_lengths, indices, old_type);
  }else{
    Datatype::create_contiguous(size, old_type, lb, new_type);
  }
  return MPI_SUCCESS;
}

int Datatype::create_struct(int count, const int* block_lengths, const MPI_Aint* indices, const MPI_Datatype* old_types, MPI_Datatype* new_type){
  size_t size = 0;
  bool contiguous=true;
  size = 0;
  MPI_Aint lb = 0;
  MPI_Aint ub = 0;
  if(count>0){
    lb=indices[0] + old_types[0]->lb();
    ub=indices[0] + block_lengths[0]*old_types[0]->ub();
  }
  bool forced_lb=false;
  bool forced_ub=false;
  for (int i = 0; i < count; i++) {
    if (block_lengths[i]<0)
      return MPI_ERR_ARG;
    if (old_types[i]->flags_ & DT_FLAG_DERIVED)
      contiguous=false;

    size += block_lengths[i]*old_types[i]->size();
    if (old_types[i]==MPI_LB){
      lb=indices[i];
      forced_lb=true;
    }
    if (old_types[i]==MPI_UB){
      ub=indices[i];
      forced_ub=true;
    }

    if (not forced_lb && indices[i] + old_types[i]->lb() < lb)
      lb = indices[i];
    if (not forced_ub && indices[i] + block_lengths[i] * old_types[i]->ub() > ub)
      ub = indices[i]+block_lengths[i]*old_types[i]->ub();

    if ( (i< count -1) && (indices[i]+block_lengths[i]*static_cast<int>(old_types[i]->size()) != indices[i+1]) )
      contiguous=false;
  }
  if (not contiguous) {
    *new_type = new Type_Struct(size, lb,ub, DT_FLAG_DERIVED|DT_FLAG_DATA,
                                count, block_lengths, indices, old_types);
  }else{
    Datatype::create_contiguous(size, MPI_CHAR, lb, new_type);
  }
  return MPI_SUCCESS;
}

int Datatype::create_subarray(int ndims, const int* array_of_sizes,
                             const int* array_of_subsizes, const int* array_of_starts,
                             int order, MPI_Datatype oldtype, MPI_Datatype *newtype){
  MPI_Datatype tmp;

  for (int i = 0; i < ndims; i++) {
    if (array_of_subsizes[i] > array_of_sizes[i]){
      XBT_WARN("subarray : array_of_subsizes > array_of_sizes for dim %d",i);
      return MPI_ERR_ARG;
    }
    if (array_of_starts[i] + array_of_subsizes[i] > array_of_sizes[i]){
      XBT_WARN("subarray : array_of_starts + array_of_subsizes > array_of_sizes for dim %d",i);
      return MPI_ERR_ARG;
    }
  }

  MPI_Aint extent = oldtype->get_extent();

  int i;
  int step;
  int end;
  if( order==MPI_ORDER_C ) {
      i = ndims - 1;
      step = -1;
      end = -1;
  } else {
      i = 0;
      step = 1;
      end = ndims;
  }

  MPI_Aint size = (MPI_Aint)array_of_sizes[i] * (MPI_Aint)array_of_sizes[i+step];
  MPI_Aint lb = (MPI_Aint)array_of_starts[i] + (MPI_Aint)array_of_starts[i+step] *(MPI_Aint)array_of_sizes[i];

  create_vector( array_of_subsizes[i+step], array_of_subsizes[i], array_of_sizes[i],
                               oldtype, newtype );

  tmp = *newtype;

  for( i += 2 * step; i != end; i += step ) {
      create_hvector( array_of_subsizes[i], 1, size * extent,
                                    tmp, newtype );
      unref(tmp);
      lb += size * array_of_starts[i];
      size *= array_of_sizes[i];
      tmp = *newtype;
  }

  const MPI_Aint lbs = lb * extent;
  const int sizes    = 1;
  //handle LB and UB with a resized call
  create_hindexed(1, &sizes, &lbs, tmp, newtype);
  unref(tmp);

  tmp = *newtype;
  create_resized(tmp, 0, extent, newtype);

  unref(tmp);
  return MPI_SUCCESS;
}

int Datatype::create_resized(MPI_Datatype oldtype,MPI_Aint lb, MPI_Aint extent, MPI_Datatype *newtype){
  const std::array<int, 3> blocks         = {{1, 1, 1}};
  const std::array<MPI_Aint, 3> disps     = {{lb, 0, lb + extent}};
  const std::array<MPI_Datatype, 3> types = {{MPI_LB, oldtype, MPI_UB}};

  *newtype = new simgrid::smpi::Type_Struct(oldtype->size(), lb, lb + extent, DT_FLAG_DERIVED, 3, blocks.data(),
                                            disps.data(), types.data());

  (*newtype)->addflag(~DT_FLAG_COMMITED);
  return MPI_SUCCESS;
}

Datatype* Datatype::f2c(int id)
{
  return static_cast<Datatype*>(F2C::f2c(id));
}

} // namespace smpi
} // namespace simgrid

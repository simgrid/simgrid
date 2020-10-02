/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "smpi_comm.hpp"
#include "smpi_datatype.hpp"
#include "smpi_errhandler.hpp"
#include "smpi_file.hpp"
#include "smpi_op.hpp"
#include "smpi_request.hpp"
#include "smpi_win.hpp"
#include "src/smpi/include/smpi_actor.hpp"

static int running_processes = 0;

void smpi_init_fortran_types()
{
  if (simgrid::smpi::F2C::lookup() == nullptr) {
    MPI_COMM_WORLD->add_f();
    MPI_BYTE->add_f(); // MPI_BYTE
    MPI_CHAR->add_f(); // MPI_CHARACTER
    if (sizeof(void*) == 8) {
      MPI_C_BOOL->add_f(); // MPI_LOGICAL
      MPI_INT->add_f();    // MPI_INTEGER
    } else {
      MPI_C_BOOL->add_f(); // MPI_LOGICAL
      MPI_LONG->add_f();   // MPI_INTEGER
    }
    MPI_INT8_T->add_f();    // MPI_INTEGER1
    MPI_INT16_T->add_f();   // MPI_INTEGER2
    MPI_INT32_T->add_f();   // MPI_INTEGER4
    MPI_INT64_T->add_f();   // MPI_INTEGER8
    MPI_REAL->add_f();      // MPI_REAL
    MPI_REAL4->add_f();     // MPI_REAL4
    MPI_REAL8->add_f();     // MPI_REAL8
    MPI_DOUBLE->add_f();    // MPI_DOUBLE_PRECISION
    MPI_COMPLEX8->add_f();  // MPI_COMPLEX
    MPI_COMPLEX16->add_f(); // MPI_DOUBLE_COMPLEX
    if (sizeof(void*) == 8)
      MPI_2INT->add_f(); // MPI_2INTEGER
    else
      MPI_2LONG->add_f();   // MPI_2INTEGER
    MPI_UINT8_T->add_f();   // MPI_LOGICAL1
    MPI_UINT16_T->add_f();  // MPI_LOGICAL2
    MPI_UINT32_T->add_f();  // MPI_LOGICAL4
    MPI_UINT64_T->add_f();  // MPI_LOGICAL8
    MPI_2FLOAT->add_f();    // MPI_2REAL
    MPI_2DOUBLE->add_f();   // MPI_2DOUBLE_PRECISION
    MPI_PTR->add_f();       // MPI_AINT
    MPI_OFFSET->add_f();    // MPI_OFFSET
    MPI_AINT->add_f();      // MPI_COUNT
    MPI_REAL16->add_f();    // MPI_REAL16
    MPI_PACKED->add_f();    // MPI_PACKED
    MPI_COMPLEX8->add_f();  // MPI_COMPLEX8
    MPI_COMPLEX16->add_f(); // MPI_COMPLEX16
    MPI_COMPLEX32->add_f(); // MPI_COMPLEX32

    MPI_MAX->add_f();
    MPI_MIN->add_f();
    MPI_MAXLOC->add_f();
    MPI_MINLOC->add_f();
    MPI_SUM->add_f();
    MPI_PROD->add_f();
    MPI_LAND->add_f();
    MPI_LOR->add_f();
    MPI_LXOR->add_f();
    MPI_BAND->add_f();
    MPI_BOR->add_f();
    MPI_BXOR->add_f();

    MPI_ERRORS_RETURN->add_f();
    MPI_ERRORS_ARE_FATAL->add_f();

    MPI_LB->add_f();
    MPI_UB->add_f();
  }
}

extern "C" { // This should really use the C linkage to be usable from Fortran

void mpi_init_(int* ierr)
{
  smpi_init_fortran_types();
  *ierr = MPI_Init(nullptr, nullptr);
  running_processes++;
}

void mpi_finalize_(int* ierr)
{
  *ierr = MPI_Finalize();
  running_processes--;
}

void mpi_abort_(int* comm, int* errorcode, int* ierr)
{
  *ierr = MPI_Abort(simgrid::smpi::Comm::f2c(*comm), *errorcode);
}

double mpi_wtime_()
{
  return MPI_Wtime();
}

double mpi_wtick_()
{
  return MPI_Wtick();
}

void mpi_group_incl_(int* group, int* n, int* ranks, int* group_out, int* ierr)
{
  MPI_Group tmp;

  *ierr = MPI_Group_incl(simgrid::smpi::Group::f2c(*group), *n, ranks, &tmp);
  if(*ierr == MPI_SUCCESS) {
    *group_out = tmp->c2f();
  }
}

void mpi_initialized_(int* flag, int* ierr)
{
  *ierr = MPI_Initialized(flag);
}

void mpi_get_processor_name_(char* name, int* resultlen, int* ierr)
{
  //fortran does not handle string endings cleanly, so initialize everything before
  memset(name, 0, MPI_MAX_PROCESSOR_NAME);
  *ierr = MPI_Get_processor_name(name, resultlen);
}

void mpi_get_count_(MPI_Status* status, int* datatype, int* count, int* ierr)
{
  *ierr = MPI_Get_count(FORT_STATUS_IGNORE(status), simgrid::smpi::Datatype::f2c(*datatype), count);
}

void mpi_attr_get_(int* comm, int* keyval, int* attr_value, int* flag, int* ierr)
{
  int* value = nullptr;
  *ierr = MPI_Attr_get(simgrid::smpi::Comm::f2c(*comm), *keyval, &value, flag);
  if(*flag == 1)
    *attr_value=*value;
}

void mpi_error_string_(int* errorcode, char* string, int* resultlen, int* ierr)
{
  *ierr = MPI_Error_string(*errorcode, string, resultlen);
}

void mpi_win_fence_(int* assert, int* win, int* ierr)
{
  *ierr =  MPI_Win_fence(* assert, simgrid::smpi::Win::f2c(*win));
}

void mpi_win_free_(int* win, int* ierr)
{
  MPI_Win tmp = simgrid::smpi::Win::f2c(*win);
  *ierr =  MPI_Win_free(&tmp);
  if(*ierr == MPI_SUCCESS) {
    simgrid::smpi::F2C::free_f(*win);
  }
}

void mpi_win_create_(int* base, MPI_Aint* size, int* disp_unit, int* info, int* comm, int* win, int* ierr)
{
  MPI_Win tmp;
  *ierr =  MPI_Win_create( static_cast<void*>(base), *size, *disp_unit, simgrid::smpi::Info::f2c(*info), simgrid::smpi::Comm::f2c(*comm),&tmp);
  if (*ierr == MPI_SUCCESS) {
    *win = tmp->add_f();
  }
}

void mpi_win_post_(int* group, int assert, int* win, int* ierr)
{
  *ierr =  MPI_Win_post(simgrid::smpi::Group::f2c(*group), assert, simgrid::smpi::Win::f2c(*win));
}

void mpi_win_start_(int* group, int assert, int* win, int* ierr)
{
  *ierr =  MPI_Win_start(simgrid::smpi::Group::f2c(*group), assert, simgrid::smpi::Win::f2c(*win));
}

void mpi_win_complete_(int* win, int* ierr)
{
  *ierr =  MPI_Win_complete(simgrid::smpi::Win::f2c(*win));
}

void mpi_win_wait_(int* win, int* ierr)
{
  *ierr =  MPI_Win_wait(simgrid::smpi::Win::f2c(*win));
}

void mpi_win_set_name_(int* win, char* name, int* ierr, int size)
{
  //handle trailing blanks
  while(name[size-1]==' ')
    size--;
  while(*name==' '){//handle leading blanks
    size--;
    name++;
  }
  char* tname = xbt_new(char,size+1);
  strncpy(tname, name, size);
  tname[size]='\0';
  *ierr = MPI_Win_set_name(simgrid::smpi::Win::f2c(*win), tname);
  xbt_free(tname);
}

void mpi_win_get_name_(int* win, char* name, int* len, int* ierr)
{
  *ierr = MPI_Win_get_name(simgrid::smpi::Win::f2c(*win),name,len);
  if(*len>0)
    name[*len]=' ';//blank padding, not \0
}

void mpi_win_allocate_(MPI_Aint* size, int* disp_unit, int* info, int* comm, void* base, int* win, int* ierr)
{
  MPI_Win tmp;
  *ierr =
      MPI_Win_allocate(*size, *disp_unit, simgrid::smpi::Info::f2c(*info), simgrid::smpi::Comm::f2c(*comm), base, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *win = tmp->add_f();
 }
}

void mpi_win_attach_(int* win, int* base, MPI_Aint* size, int* ierr)
{
  *ierr =  MPI_Win_attach(simgrid::smpi::Win::f2c(*win), static_cast<void*>(base), *size);
}

void mpi_win_create_dynamic_(int* info, int* comm, int* win, int* ierr)
{
  MPI_Win tmp;
  *ierr =  MPI_Win_create_dynamic( simgrid::smpi::Info::f2c(*info), simgrid::smpi::Comm::f2c(*comm),&tmp);
 if(*ierr == MPI_SUCCESS) {
   *win = tmp->add_f();
 }
}

void mpi_win_detach_(int* win, int* base, int* ierr)
{
  *ierr =  MPI_Win_detach(simgrid::smpi::Win::f2c(*win), static_cast<void*>(base));
}

void mpi_win_set_info_(int* win, int* info, int* ierr)
{
  *ierr =  MPI_Win_set_info(simgrid::smpi::Win::f2c(*win), simgrid::smpi::Info::f2c(*info));
}

void mpi_win_get_info_(int* win, int* info, int* ierr)
{
  MPI_Info tmp;
  *ierr =  MPI_Win_get_info(simgrid::smpi::Win::f2c(*win), &tmp);
  if (*ierr == MPI_SUCCESS) {
    *info = tmp->add_f();
  }
}

void mpi_win_get_group_(int* win, int* group, int* ierr)
{
  MPI_Group tmp;
  *ierr =  MPI_Win_get_group(simgrid::smpi::Win::f2c(*win), &tmp);
  if (*ierr == MPI_SUCCESS) {
    *group = tmp->add_f();
  }
}

void mpi_win_get_attr_(int* win, int* type_keyval, MPI_Aint* attribute_val, int* flag, int* ierr)
{
  MPI_Aint* value = nullptr;
  *ierr = MPI_Win_get_attr(simgrid::smpi::Win::f2c(*win), *type_keyval, &value, flag);
  if (*flag == 1)
    *attribute_val = *value;
}

void mpi_win_set_attr_(int* win, int* type_keyval, MPI_Aint* att, int* ierr)
{
  MPI_Aint* val = (MPI_Aint*)xbt_malloc(sizeof(MPI_Aint));
  *val          = *att;
  *ierr = MPI_Win_set_attr(simgrid::smpi::Win::f2c(*win), *type_keyval, val);
}

void mpi_win_delete_attr_(int* win, int* comm_keyval, int* ierr)
{
  *ierr = MPI_Win_delete_attr (simgrid::smpi::Win::f2c(*win),  *comm_keyval);
}

void mpi_win_create_keyval_(void* copy_fn, void* delete_fn, int* keyval, void* extra_state, int* ierr)
{
  smpi_copy_fn _copy_fn={nullptr,nullptr,nullptr,nullptr,nullptr,(*(int*)copy_fn) == 0 ? nullptr : reinterpret_cast<MPI_Win_copy_attr_function_fort*>(copy_fn)};
  smpi_delete_fn _delete_fn={nullptr,nullptr,nullptr,nullptr,nullptr,(*(int*)delete_fn) == 0 ? nullptr : reinterpret_cast<MPI_Win_delete_attr_function_fort*>(delete_fn)};
  *ierr = simgrid::smpi::Keyval::keyval_create<simgrid::smpi::Win>(_copy_fn, _delete_fn, keyval, extra_state);
}

void mpi_win_free_keyval_(int* keyval, int* ierr)
{
  *ierr = MPI_Win_free_keyval( keyval);
}

void mpi_win_lock_(int* lock_type, int* rank, int* assert, int* win, int* ierr)
{
  *ierr = MPI_Win_lock(*lock_type, *rank, *assert, simgrid::smpi::Win::f2c(*win));
}

void mpi_win_lock_all_(int* assert, int* win, int* ierr)
{
  *ierr = MPI_Win_lock_all(*assert, simgrid::smpi::Win::f2c(*win));
}

void mpi_win_unlock_(int* rank, int* win, int* ierr)
{
  *ierr = MPI_Win_unlock(*rank, simgrid::smpi::Win::f2c(*win));
}

void mpi_win_unlock_all_(int* win, int* ierr)
{
  *ierr = MPI_Win_unlock_all(simgrid::smpi::Win::f2c(*win));
}

void mpi_win_flush_(int* rank, int* win, int* ierr)
{
  *ierr = MPI_Win_flush(*rank, simgrid::smpi::Win::f2c(*win));
}

void mpi_win_flush_local_(int* rank, int* win, int* ierr)
{
  *ierr = MPI_Win_flush_local(*rank, simgrid::smpi::Win::f2c(*win));
}

void mpi_win_flush_all_(int* win, int* ierr)
{
  *ierr = MPI_Win_flush_all(simgrid::smpi::Win::f2c(*win));
}

void mpi_win_flush_local_all_(int* win, int* ierr)
{
  *ierr = MPI_Win_flush_local_all(simgrid::smpi::Win::f2c(*win));
}

void mpi_win_null_copy_fn_(int* /*win*/, int* /*keyval*/, int* /*extrastate*/, MPI_Aint* /*valin*/,
                           MPI_Aint* /*valout*/, int* flag, int* ierr)
{
  *flag=0;
  *ierr=MPI_SUCCESS;
}

void mpi_win_dup_fn_(int* /*win*/, int* /*keyval*/, int* /*extrastate*/, MPI_Aint* valin, MPI_Aint* valout, int* flag,
                     int* ierr)
{
  *flag=1;
  *valout=*valin;
  *ierr=MPI_SUCCESS;
}

void mpi_info_create_(int* info, int* ierr)
{
  MPI_Info tmp;
  *ierr =  MPI_Info_create(&tmp);
  if(*ierr == MPI_SUCCESS) {
    *info = tmp->add_f();
  }
}

void mpi_info_set_(int* info, char* key, char* value, int* ierr, unsigned int keylen, unsigned int valuelen)
{
  //handle trailing blanks
  while(key[keylen-1]==' ')
    keylen--;
  while(*key==' '){//handle leading blanks
    keylen--;
    key++;
  }
  char* tkey = xbt_new(char,keylen+1);
  strncpy(tkey, key, keylen);
  tkey[keylen]='\0';

  while(value[valuelen-1]==' ')
    valuelen--;
  while(*value==' '){//handle leading blanks
    valuelen--;
    value++;
  }
  char* tvalue = xbt_new(char,valuelen+1);
  strncpy(tvalue, value, valuelen);
  tvalue[valuelen]='\0';

  *ierr =  MPI_Info_set( simgrid::smpi::Info::f2c(*info), tkey, tvalue);
  xbt_free(tkey);
  xbt_free(tvalue);
}

void mpi_info_get_(int* info, char* key, int* valuelen, char* value, int* flag, int* ierr, unsigned int keylen)
{
  while(key[keylen-1]==' ')
    keylen--;
  while(*key==' '){//handle leading blanks
    keylen--;
    key++;
  }
  char* tkey = xbt_new(char,keylen+1);
  strncpy(tkey, key, keylen);
  tkey[keylen]='\0';
  *ierr = MPI_Info_get(simgrid::smpi::Info::f2c(*info),tkey,*valuelen, value, flag);
  xbt_free(tkey);
  if(*flag!=0){
    int replace=0;
    for (int i = 0; i < *valuelen; i++) {
      if(value[i]=='\0')
        replace=1;
      if(replace)
        value[i]=' ';
    }
  }
}

void mpi_info_free_(int* info, int* ierr)
{
  MPI_Info tmp = simgrid::smpi::Info::f2c(*info);
  *ierr =  MPI_Info_free(&tmp);
  if(*ierr == MPI_SUCCESS) {
    simgrid::smpi::F2C::free_f(*info);
  }
}

void mpi_get_(int* origin_addr, int* origin_count, int* origin_datatype, int* target_rank, MPI_Aint* target_disp,
              int* target_count, int* tarsmpi_type_f2c, int* win, int* ierr)
{
  *ierr =  MPI_Get( static_cast<void*>(origin_addr),*origin_count, simgrid::smpi::Datatype::f2c(*origin_datatype),*target_rank,
      *target_disp, *target_count, simgrid::smpi::Datatype::f2c(*tarsmpi_type_f2c), simgrid::smpi::Win::f2c(*win));
}

void mpi_rget_(int* origin_addr, int* origin_count, int* origin_datatype, int* target_rank, MPI_Aint* target_disp,
               int* target_count, int* tarsmpi_type_f2c, int* win, int* request, int* ierr)
{
  MPI_Request req;
  *ierr =  MPI_Rget( static_cast<void*>(origin_addr),*origin_count, simgrid::smpi::Datatype::f2c(*origin_datatype),*target_rank,
      *target_disp, *target_count, simgrid::smpi::Datatype::f2c(*tarsmpi_type_f2c), simgrid::smpi::Win::f2c(*win), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_accumulate_(int* origin_addr, int* origin_count, int* origin_datatype, int* target_rank, MPI_Aint* target_disp,
                     int* target_count, int* tarsmpi_type_f2c, int* op, int* win, int* ierr)
{
  *ierr =  MPI_Accumulate( static_cast<void *>(origin_addr),*origin_count, simgrid::smpi::Datatype::f2c(*origin_datatype),*target_rank,
      *target_disp, *target_count, simgrid::smpi::Datatype::f2c(*tarsmpi_type_f2c), simgrid::smpi::Op::f2c(*op), simgrid::smpi::Win::f2c(*win));
}

void mpi_raccumulate_(int* origin_addr, int* origin_count, int* origin_datatype, int* target_rank,
                      MPI_Aint* target_disp, int* target_count, int* tarsmpi_type_f2c, int* op, int* win, int* request,
                      int* ierr)
{
  MPI_Request req;
  *ierr =  MPI_Raccumulate( static_cast<void *>(origin_addr),*origin_count, simgrid::smpi::Datatype::f2c(*origin_datatype),*target_rank,
      *target_disp, *target_count, simgrid::smpi::Datatype::f2c(*tarsmpi_type_f2c), simgrid::smpi::Op::f2c(*op), simgrid::smpi::Win::f2c(*win),&req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_put_(int* origin_addr, int* origin_count, int* origin_datatype, int* target_rank, MPI_Aint* target_disp,
              int* target_count, int* tarsmpi_type_f2c, int* win, int* ierr)
{
  *ierr =  MPI_Put( static_cast<void *>(origin_addr),*origin_count, simgrid::smpi::Datatype::f2c(*origin_datatype),*target_rank,
      *target_disp, *target_count, simgrid::smpi::Datatype::f2c(*tarsmpi_type_f2c), simgrid::smpi::Win::f2c(*win));
}

void mpi_rput_(int* origin_addr, int* origin_count, int* origin_datatype, int* target_rank, MPI_Aint* target_disp,
               int* target_count, int* tarsmpi_type_f2c, int* win, int* request, int* ierr)
{
  MPI_Request req;
  *ierr =  MPI_Rput( static_cast<void *>(origin_addr),*origin_count, simgrid::smpi::Datatype::f2c(*origin_datatype),*target_rank,
      *target_disp, *target_count, simgrid::smpi::Datatype::f2c(*tarsmpi_type_f2c), simgrid::smpi::Win::f2c(*win),&req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

void mpi_fetch_and_op_(int* origin_addr, int* result_addr, int* datatype, int* target_rank, MPI_Aint* target_disp,
                       int* op, int* win, int* ierr)
{
  *ierr =  MPI_Fetch_and_op( static_cast<void *>(origin_addr),
              static_cast<void *>(result_addr), simgrid::smpi::Datatype::f2c(*datatype),*target_rank,
              *target_disp, simgrid::smpi::Op::f2c(*op), simgrid::smpi::Win::f2c(*win));
}

void mpi_compare_and_swap_(int* origin_addr, int* compare_addr, int* result_addr, int* datatype, int* target_rank,
                           MPI_Aint* target_disp, int* win, int* ierr)
{
  *ierr =  MPI_Compare_and_swap( static_cast<void *>(origin_addr),static_cast<void *>(compare_addr),
              static_cast<void *>(result_addr), simgrid::smpi::Datatype::f2c(*datatype),*target_rank,
              *target_disp, simgrid::smpi::Win::f2c(*win));
}

void mpi_get_accumulate_(int* origin_addr, int* origin_count, int* origin_datatype, int* result_addr, int* result_count,
                         int* result_datatype, int* target_rank, MPI_Aint* target_disp, int* target_count,
                         int* target_datatype, int* op, int* win, int* ierr)
{
  *ierr =
      MPI_Get_accumulate(static_cast<void*>(origin_addr), *origin_count, simgrid::smpi::Datatype::f2c(*origin_datatype),
                         static_cast<void*>(result_addr), *result_count, simgrid::smpi::Datatype::f2c(*result_datatype),
                         *target_rank, *target_disp, *target_count, simgrid::smpi::Datatype::f2c(*target_datatype),
                         simgrid::smpi::Op::f2c(*op), simgrid::smpi::Win::f2c(*win));
}

void mpi_rget_accumulate_(int* origin_addr, int* origin_count, int* origin_datatype, int* result_addr,
                          int* result_count, int* result_datatype, int* target_rank, MPI_Aint* target_disp,
                          int* target_count, int* target_datatype, int* op, int* win, int* request, int* ierr)
{
  MPI_Request req;
  *ierr = MPI_Rget_accumulate(static_cast<void*>(origin_addr), *origin_count,
                              simgrid::smpi::Datatype::f2c(*origin_datatype), static_cast<void*>(result_addr),
                              *result_count, simgrid::smpi::Datatype::f2c(*result_datatype), *target_rank, *target_disp,
                              *target_count, simgrid::smpi::Datatype::f2c(*target_datatype),
                              simgrid::smpi::Op::f2c(*op), simgrid::smpi::Win::f2c(*win), &req);
  if(*ierr == MPI_SUCCESS) {
    *request = req->add_f();
  }
}

//following are automatically generated, and have to be checked
void mpi_finalized_(int* flag, int* ierr)
{
  *ierr = MPI_Finalized(flag);
}

void mpi_init_thread_(int* required, int* provided, int* ierr)
{
  smpi_init_fortran_types();
  *ierr = MPI_Init_thread(nullptr, nullptr,*required, provided);
  running_processes++;
}

void mpi_query_thread_(int* provided, int* ierr)
{
  *ierr = MPI_Query_thread(provided);
}

void mpi_is_thread_main_(int* flag, int* ierr)
{
  *ierr = MPI_Is_thread_main(flag);
}

void mpi_address_(void* location, MPI_Aint* address, int* ierr)
{
  *ierr = MPI_Address(location, address);
}

void mpi_get_address_(void* location, MPI_Aint* address, int* ierr)
{
  *ierr = MPI_Get_address(location, address);
}

void mpi_pcontrol_(int* level, int* ierr)
{
  *ierr = MPI_Pcontrol(*static_cast<const int*>(level));
}

void mpi_op_create_(void* function, int* commute, int* op, int* ierr)
{
  MPI_Op tmp;
  *ierr = MPI_Op_create(reinterpret_cast<MPI_User_function*>(function), *commute, &tmp);
  if (*ierr == MPI_SUCCESS) {
    tmp->set_fortran_op();
    *op = tmp->add_f();
  }
}

void mpi_op_free_(int* op, int* ierr)
{
  MPI_Op tmp= simgrid::smpi::Op::f2c(*op);
  *ierr = MPI_Op_free(& tmp);
  if(*ierr == MPI_SUCCESS) {
    simgrid::smpi::F2C::free_f(*op);
  }
}

void mpi_op_commutative_(int* op, int* commute, int* ierr)
{
  *ierr = MPI_Op_commutative(simgrid::smpi::Op::f2c(*op), commute);
}

void mpi_group_free_(int* group, int* ierr)
{
  MPI_Group tmp = simgrid::smpi::Group::f2c(*group);
  if(tmp != MPI_COMM_WORLD->group() && tmp != MPI_GROUP_EMPTY){
    simgrid::smpi::Group::unref(tmp);
    simgrid::smpi::F2C::free_f(*group);
  }
  *ierr = MPI_SUCCESS;
}

void mpi_group_size_(int* group, int* size, int* ierr)
{
  *ierr = MPI_Group_size(simgrid::smpi::Group::f2c(*group), size);
}

void mpi_group_rank_(int* group, int* rank, int* ierr)
{
  *ierr = MPI_Group_rank(simgrid::smpi::Group::f2c(*group), rank);
}

void mpi_group_translate_ranks_ (int* group1, int* n, int *ranks1, int* group2, int *ranks2, int* ierr)
{
 *ierr = MPI_Group_translate_ranks(simgrid::smpi::Group::f2c(*group1), *n, ranks1, simgrid::smpi::Group::f2c(*group2), ranks2);
}

void mpi_group_compare_(int* group1, int* group2, int* result, int* ierr)
{
  *ierr = MPI_Group_compare(simgrid::smpi::Group::f2c(*group1), simgrid::smpi::Group::f2c(*group2), result);
}

void mpi_group_union_(int* group1, int* group2, int* newgroup, int* ierr)
{
  MPI_Group tmp;
  *ierr = MPI_Group_union(simgrid::smpi::Group::f2c(*group1), simgrid::smpi::Group::f2c(*group2), &tmp);
  if (*ierr == MPI_SUCCESS) {
    *newgroup = tmp->add_f();
  }
}

void mpi_group_intersection_(int* group1, int* group2, int* newgroup, int* ierr)
{
  MPI_Group tmp;
  *ierr = MPI_Group_intersection(simgrid::smpi::Group::f2c(*group1), simgrid::smpi::Group::f2c(*group2), &tmp);
  if (*ierr == MPI_SUCCESS) {
    *newgroup = tmp->add_f();
  }
}

void mpi_group_difference_(int* group1, int* group2, int* newgroup, int* ierr)
{
  MPI_Group tmp;
  *ierr = MPI_Group_difference(simgrid::smpi::Group::f2c(*group1), simgrid::smpi::Group::f2c(*group2), &tmp);
  if (*ierr == MPI_SUCCESS) {
    *newgroup = tmp->add_f();
  }
}

void mpi_group_excl_(int* group, int* n, int* ranks, int* newgroup, int* ierr)
{
  MPI_Group tmp;
  *ierr = MPI_Group_excl(simgrid::smpi::Group::f2c(*group), *n, ranks, &tmp);
  if (*ierr == MPI_SUCCESS) {
    *newgroup = tmp->add_f();
  }
}

void mpi_group_range_incl_ (int* group, int* n, int ranges[][3], int* newgroup, int* ierr)
{
  MPI_Group tmp;
  *ierr = MPI_Group_range_incl(simgrid::smpi::Group::f2c(*group), *n, ranges, &tmp);
  if (*ierr == MPI_SUCCESS) {
    *newgroup = tmp->add_f();
  }
}

void mpi_group_range_excl_ (int* group, int* n, int ranges[][3], int* newgroup, int* ierr)
{
 MPI_Group tmp;
 *ierr = MPI_Group_range_excl(simgrid::smpi::Group::f2c(*group), *n, ranges, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *newgroup = tmp->add_f();
 }
}

void mpi_request_free_ (int* request, int* ierr){
  MPI_Request tmp=simgrid::smpi::Request::f2c(*request);
 *ierr = MPI_Request_free(&tmp);
 if(*ierr == MPI_SUCCESS) {
   simgrid::smpi::Request::free_f(*request);
 }
}

void mpi_pack_size_ (int* incount, int* datatype, int* comm, int* size, int* ierr) {
 *ierr = MPI_Pack_size(*incount, simgrid::smpi::Datatype::f2c(*datatype), simgrid::smpi::Comm::f2c(*comm), size);
}

void mpi_cart_coords_ (int* comm, int* rank, int* maxdims, int* coords, int* ierr) {
 *ierr = MPI_Cart_coords(simgrid::smpi::Comm::f2c(*comm), *rank, *maxdims, coords);
}

void mpi_cart_create_ (int* comm_old, int* ndims, int* dims, int* periods, int* reorder, int*  comm_cart, int* ierr) {
  MPI_Comm tmp;
 *ierr = MPI_Cart_create(simgrid::smpi::Comm::f2c(*comm_old), *ndims, dims, periods, *reorder, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *comm_cart = tmp->add_f();
 }
}

void mpi_cart_get_ (int* comm, int* maxdims, int* dims, int* periods, int* coords, int* ierr) {
 *ierr = MPI_Cart_get(simgrid::smpi::Comm::f2c(*comm), *maxdims, dims, periods, coords);
}

void mpi_cart_map_ (int* comm_old, int* ndims, int* dims, int* periods, int* newrank, int* ierr) {
 *ierr = MPI_Cart_map(simgrid::smpi::Comm::f2c(*comm_old), *ndims, dims, periods, newrank);
}

void mpi_cart_rank_ (int* comm, int* coords, int* rank, int* ierr) {
 *ierr = MPI_Cart_rank(simgrid::smpi::Comm::f2c(*comm), coords, rank);
}

void mpi_cart_shift_ (int* comm, int* direction, int* displ, int* source, int* dest, int* ierr) {
 *ierr = MPI_Cart_shift(simgrid::smpi::Comm::f2c(*comm), *direction, *displ, source, dest);
}

void mpi_cart_sub_ (int* comm, int* remain_dims, int*  comm_new, int* ierr) {
 MPI_Comm tmp;
 *ierr = MPI_Cart_sub(simgrid::smpi::Comm::f2c(*comm), remain_dims, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *comm_new = tmp->add_f();
 }
}

void mpi_cartdim_get_ (int* comm, int* ndims, int* ierr) {
 *ierr = MPI_Cartdim_get(simgrid::smpi::Comm::f2c(*comm), ndims);
}

void mpi_graph_create_ (int* comm_old, int* nnodes, int* index, int* edges, int* reorder, int*  comm_graph, int* ierr) {
  MPI_Comm tmp;
 *ierr = MPI_Graph_create(simgrid::smpi::Comm::f2c(*comm_old), *nnodes, index, edges, *reorder, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *comm_graph = tmp->add_f();
 }
}

void mpi_graph_get_ (int* comm, int* maxindex, int* maxedges, int* index, int* edges, int* ierr) {
 *ierr = MPI_Graph_get(simgrid::smpi::Comm::f2c(*comm), *maxindex, *maxedges, index, edges);
}

void mpi_graph_map_ (int* comm_old, int* nnodes, int* index, int* edges, int* newrank, int* ierr) {
 *ierr = MPI_Graph_map(simgrid::smpi::Comm::f2c(*comm_old), *nnodes, index, edges, newrank);
}

void mpi_graph_neighbors_ (int* comm, int* rank, int* maxneighbors, int* neighbors, int* ierr) {
 *ierr = MPI_Graph_neighbors(simgrid::smpi::Comm::f2c(*comm), *rank, *maxneighbors, neighbors);
}

void mpi_graph_neighbors_count_ (int* comm, int* rank, int* nneighbors, int* ierr) {
 *ierr = MPI_Graph_neighbors_count(simgrid::smpi::Comm::f2c(*comm), *rank, nneighbors);
}

void mpi_graphdims_get_ (int* comm, int* nnodes, int* nedges, int* ierr) {
 *ierr = MPI_Graphdims_get(simgrid::smpi::Comm::f2c(*comm), nnodes, nedges);
}

void mpi_topo_test_ (int* comm, int* top_type, int* ierr) {
 *ierr = MPI_Topo_test(simgrid::smpi::Comm::f2c(*comm), top_type);
}

void mpi_error_class_ (int* errorcode, int* errorclass, int* ierr) {
 *ierr = MPI_Error_class(*errorcode, errorclass);
}

void mpi_errhandler_create_ (void* function, int* errhandler, int* ierr) {
 MPI_Errhandler tmp;
 *ierr = MPI_Errhandler_create( reinterpret_cast<MPI_Handler_function*>(function), &tmp);
 if(*ierr==MPI_SUCCESS){
   *errhandler = tmp->c2f();
 }
}

void mpi_errhandler_free_ (int* errhandler, int* ierr) {
  MPI_Errhandler tmp = simgrid::smpi::Errhandler::f2c(*errhandler);
  *ierr =  MPI_Errhandler_free(&tmp);
  if(*ierr == MPI_SUCCESS) {
    simgrid::smpi::F2C::free_f(*errhandler);
  }
}

void mpi_errhandler_get_ (int* comm, int* errhandler, int* ierr) {
 MPI_Errhandler tmp;
 *ierr = MPI_Errhandler_get(simgrid::smpi::Comm::f2c(*comm), &tmp);
 if(*ierr == MPI_SUCCESS) {
   *errhandler = tmp->c2f();
 }
}

void mpi_errhandler_set_ (int* comm, int* errhandler, int* ierr) {
 *ierr = MPI_Errhandler_set(simgrid::smpi::Comm::f2c(*comm), simgrid::smpi::Errhandler::f2c(*errhandler));
}

void mpi_cancel_ (int* request, int* ierr) {
  MPI_Request tmp=simgrid::smpi::Request::f2c(*request);
 *ierr = MPI_Cancel(&tmp);
}

void mpi_buffer_attach_ (void* buffer, int* size, int* ierr) {
 *ierr = MPI_Buffer_attach(buffer, *size);
}

void mpi_buffer_detach_ (void* buffer, int* size, int* ierr) {
 *ierr = MPI_Buffer_detach(buffer, size);
}



void mpi_intercomm_create_ (int* local_comm, int *local_leader, int* peer_comm, int* remote_leader, int* tag,
                            int* comm_out, int* ierr) {
  MPI_Comm tmp;
  *ierr = MPI_Intercomm_create(simgrid::smpi::Comm::f2c(*local_comm), *local_leader, simgrid::smpi::Comm::f2c(*peer_comm), *remote_leader,
                               *tag, &tmp);
  if(*ierr == MPI_SUCCESS) {
    *comm_out = tmp->add_f();
  }
}

void mpi_intercomm_merge_ (int* comm, int* high, int*  comm_out, int* ierr) {
 MPI_Comm tmp;
 *ierr = MPI_Intercomm_merge(simgrid::smpi::Comm::f2c(*comm), *high, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *comm_out = tmp->add_f();
 }
}

void mpi_attr_delete_ (int* comm, int* keyval, int* ierr) {
 *ierr = MPI_Attr_delete(simgrid::smpi::Comm::f2c(*comm), *keyval);
}

void mpi_attr_put_ (int* comm, int* keyval, int* attr_value, int* ierr) {
 int* val = (int*)xbt_malloc(sizeof(int));
 *val=*attr_value;
 *ierr = MPI_Attr_put(simgrid::smpi::Comm::f2c(*comm), *keyval, val);
}

void mpi_keyval_create_ (void* copy_fn, void* delete_fn, int* keyval, void* extra_state, int* ierr) {
  smpi_copy_fn _copy_fn={nullptr,nullptr,nullptr,(*(int*)copy_fn) == 0 ? nullptr : reinterpret_cast<MPI_Copy_function_fort*>(copy_fn),nullptr,nullptr};
  smpi_delete_fn _delete_fn={nullptr,nullptr,nullptr,(*(int*)delete_fn) == 0 ? nullptr : reinterpret_cast<MPI_Delete_function_fort*>(delete_fn),nullptr,nullptr};
  *ierr = simgrid::smpi::Keyval::keyval_create<simgrid::smpi::Comm>(_copy_fn, _delete_fn, keyval, extra_state);
}

void mpi_keyval_free_ (int* keyval, int* ierr) {
 *ierr = MPI_Keyval_free(keyval);
}

void mpi_test_cancelled_ (MPI_Status*  status, int* flag, int* ierr) {
 *ierr = MPI_Test_cancelled(status, flag);
}

void mpi_get_elements_ (MPI_Status*  status, int* datatype, int* elements, int* ierr) {
 *ierr = MPI_Get_elements(status, simgrid::smpi::Datatype::f2c(*datatype), elements);
}

void mpi_dims_create_ (int* nnodes, int* ndims, int* dims, int* ierr) {
 *ierr = MPI_Dims_create(*nnodes, *ndims, dims);
}

void mpi_add_error_class_ ( int *errorclass, int* ierr){
 *ierr = MPI_Add_error_class( errorclass);
}

void mpi_add_error_code_ (  int* errorclass, int *errorcode, int* ierr){
 *ierr = MPI_Add_error_code(*errorclass, errorcode);
}

void mpi_add_error_string_ ( int* errorcode, char *string, int* ierr){
 *ierr = MPI_Add_error_string(*errorcode, string);
}

void mpi_info_dup_ (int* info, int* newinfo, int* ierr){
 MPI_Info tmp;
 *ierr = MPI_Info_dup(simgrid::smpi::Info::f2c(*info), &tmp);
 if(*ierr==MPI_SUCCESS){
   *newinfo= tmp->add_f();
 }
}

void mpi_info_get_valuelen_ ( int* info, char *key, int *valuelen, int *flag, int* ierr, unsigned int keylen){
  while(key[keylen-1]==' ')
    keylen--;
  while(*key==' '){//handle leading blanks
    keylen--;
    key++;
  }
  char* tkey = xbt_new(char, keylen+1);
  strncpy(tkey, key, keylen);
  tkey[keylen]='\0';
  *ierr = MPI_Info_get_valuelen( simgrid::smpi::Info::f2c(*info), tkey, valuelen, flag);
  xbt_free(tkey);
}

void mpi_info_delete_ (int* info, char *key, int* ierr, unsigned int keylen){
  while(key[keylen-1]==' ')
    keylen--;
  while(*key==' '){//handle leading blanks
    keylen--;
    key++;
  }
  char* tkey = xbt_new(char, keylen+1);
  strncpy(tkey, key, keylen);
  tkey[keylen]='\0';
  *ierr = MPI_Info_delete(simgrid::smpi::Info::f2c(*info), tkey);
  xbt_free(tkey);
}

void mpi_info_get_nkeys_ ( int* info, int *nkeys, int* ierr){
 *ierr = MPI_Info_get_nkeys(  simgrid::smpi::Info::f2c(*info), nkeys);
}

void mpi_info_get_nthkey_ ( int* info, int* n, char *key, int* ierr, unsigned int keylen){
  *ierr = MPI_Info_get_nthkey( simgrid::smpi::Info::f2c(*info), *n, key);
  for (unsigned int i = strlen(key); i < keylen; i++)
    key[i]=' ';
}

void mpi_get_version_ (int *version,int *subversion, int* ierr){
 *ierr = MPI_Get_version (version,subversion);
}

void mpi_get_library_version_ (char *version,int *len, int* ierr){
 *ierr = MPI_Get_library_version (version,len);
}

void mpi_request_get_status_ ( int* request, int *flag, MPI_Status* status, int* ierr){
 *ierr = MPI_Request_get_status( simgrid::smpi::Request::f2c(*request), flag, status);
}

void mpi_grequest_start_ ( void *query_fn, void *free_fn, void *cancel_fn, void *extra_state, int*request, int* ierr){
  MPI_Request tmp;
  *ierr = MPI_Grequest_start( reinterpret_cast<MPI_Grequest_query_function*>(query_fn), reinterpret_cast<MPI_Grequest_free_function*>(free_fn),
                              reinterpret_cast<MPI_Grequest_cancel_function*>(cancel_fn), extra_state, &tmp);
 if(*ierr == MPI_SUCCESS) {
   *request = tmp->add_f();
 }
}

void mpi_grequest_complete_ ( int* request, int* ierr){
 *ierr = MPI_Grequest_complete( simgrid::smpi::Request::f2c(*request));
}

void mpi_status_set_cancelled_ (MPI_Status* status,int* flag, int* ierr){
 *ierr = MPI_Status_set_cancelled(status,*flag);
}

void mpi_status_set_elements_ ( MPI_Status* status, int* datatype, int* count, int* ierr){
 *ierr = MPI_Status_set_elements( status, simgrid::smpi::Datatype::f2c(*datatype), *count);
}

void mpi_publish_name_ ( char *service_name, int* info, char *port_name, int* ierr){
 *ierr = MPI_Publish_name( service_name, simgrid::smpi::Info::f2c(*info), port_name);
}

void mpi_unpublish_name_ ( char *service_name, int* info, char *port_name, int* ierr){
 *ierr = MPI_Unpublish_name( service_name, simgrid::smpi::Info::f2c(*info), port_name);
}

void mpi_lookup_name_ ( char *service_name, int* info, char *port_name, int* ierr){
 *ierr = MPI_Lookup_name( service_name, simgrid::smpi::Info::f2c(*info), port_name);
}

void mpi_open_port_ ( int* info, char *port_name, int* ierr){
 *ierr = MPI_Open_port( simgrid::smpi::Info::f2c(*info),port_name);
}

void mpi_close_port_ ( char *port_name, int* ierr){
 *ierr = MPI_Close_port( port_name);
}

void smpi_execute_flops_(double* flops){
  smpi_execute_flops(*flops);
}

void smpi_execute_flops_benched_(double* flops){
  smpi_execute_flops_benched(*flops);
}

void smpi_execute_(double* duration){
  smpi_execute(*duration);
}

void smpi_execute_benched_(double* duration){
  smpi_execute_benched(*duration);
}

} // extern "C"

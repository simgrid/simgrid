/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.           */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license ,(GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "simgrid/modelchecker.h"
#include "smpi_comm.hpp"
#include "smpi_file.hpp"
#include "smpi_win.hpp"
#include "src/simgrid/sg_config.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_mpi, smpi, "Logging specific to SMPI ,(mpi)");

void MPI_Init()
{
  MPI_Init(nullptr, nullptr);
}

#define NOT_YET_IMPLEMENTED                                                                                            \
  {                                                                                                                    \
    xbt_die("Not yet implemented: %s. Please contact the SimGrid team if support is needed", __func__);                \
    return MPI_SUCCESS;                                                                                                \
  }

#define NOT_YET_IMPLEMENTED_NULL                                                                                       \
  {                                                                                                                    \
    xbt_die("Not yet implemented: %s. Please contact the SimGrid team if support is needed", __func__);                \
    return nullptr;                                                                                                    \
  }

#define NOT_YET_IMPLEMENTED_NOFAIL                                                                                     \
  {                                                                                                                    \
    if (static bool warned_todo = false; not warned_todo) {                                                            \
      XBT_WARN("Not yet implemented: %s. "                                                                             \
               "Please contact the SimGrid team if support is needed. "                                                \
               "Run with --log=smpi_mpi.thresh:error to hide",                                                         \
               __func__);                                                                                              \
      warned_todo = true;                                                                                              \
    }                                                                                                                  \
    return MPI_SUCCESS;                                                                                                \
  }

#define WRAPPED_PMPI_CALL_ERRHANDLER(type, name, args, args2, errhan)                                                  \
  type name args                                                                                                       \
  {                                                                                                                    \
    XBT_VERB("SMPI - Entering %s", __func__);                                                                          \
    type ret = _XBT_CONCAT(P, name) args2;                                                                             \
    if (ret != MPI_SUCCESS) {                                                                                          \
      char error_string[MPI_MAX_ERROR_STRING];                                                                         \
      int error_size;                                                                                                  \
      PMPI_Error_string(ret, error_string, &error_size);                                                               \
      MPI_Errhandler err = (errhan) ? (errhan)->errhandler() : MPI_ERRHANDLER_NULL;                                    \
      if (err == MPI_ERRHANDLER_NULL || err == MPI_ERRORS_RETURN)                                                      \
        XBT_WARN("%s - returned %.*s instead of MPI_SUCCESS", __func__, error_size, error_string);                     \
      else if (err == MPI_ERRORS_ARE_FATAL){                                                                           \
        if (xbt_log_no_loc)                                                                                            \
          XBT_INFO("The backtrace would be displayed here if --log=no_loc would not have been passed");                \
        else{                                                                                                          \
          XBT_INFO("Backtrace of the run : if incomplete, run smpirun with -keep-temps. To hide, use --log=no_loc");   \
          xbt_backtrace_display_current();                                                                             \
        }                                                                                                              \
        simgrid::smpi::utils::print_current_handle();                                                                  \
        simgrid::smpi::utils::print_buffer_info();                                                                     \
        xbt_die("%s - returned %.*s instead of MPI_SUCCESS", __func__, error_size, error_string);                      \
      } else                                                                                                           \
        err->call((errhan), ret);                                                                                      \
      if (err != MPI_ERRHANDLER_NULL)                                                                                  \
        simgrid::smpi::Errhandler::unref(err);                                                                         \
      MC_assert(not MC_is_active()); /* Only fail in MC mode */                                                        \
    }                                                                                                                  \
    XBT_VERB("SMPI - Leaving %s", __func__);                                                                           \
    return ret;                                                                                                        \
  }

#define WRAPPED_PMPI_CALL_ERRHANDLER_COMM(type, name, args, args2) WRAPPED_PMPI_CALL_ERRHANDLER(type, name, args, args2, (comm == MPI_COMM_NULL) ? MPI_COMM_WORLD : comm)
#define WRAPPED_PMPI_CALL_ERRHANDLER_WIN(type, name, args, args2) WRAPPED_PMPI_CALL_ERRHANDLER(type, name, args, args2, win)
#define WRAPPED_PMPI_CALL_ERRHANDLER_FILE(type, name, args, args2) WRAPPED_PMPI_CALL_ERRHANDLER(type, name, args, args2, fh)
#define WRAPPED_PMPI_CALL(type, name, args, args2) WRAPPED_PMPI_CALL_ERRHANDLER(type, name, args, args2, MPI_COMM_WORLD)

#define WRAPPED_PMPI_CALL_NORETURN(type, name, args, args2)                                                            \
  type name args                                                                                                       \
  {                                                                                                                    \
    XBT_VERB("SMPI - Entering %s", __func__);                                                                          \
    type ret = _XBT_CONCAT(P, name) args2;                                                                             \
    XBT_VERB("SMPI - Leaving %s", __func__);                                                                           \
    return ret;                                                                                                        \
  }

#define UNIMPLEMENTED_WRAPPED_PMPI_CALL(type, name, args, args2)                                                       \
  type _XBT_CONCAT(P, name) args { NOT_YET_IMPLEMENTED }                                                               \
  type name args { return _XBT_CONCAT(P, name) args2; }

#define UNIMPLEMENTED_WRAPPED_PMPI_CALL_NOFAIL(type, name, args, args2)                                                \
  type _XBT_CONCAT(P, name) args { NOT_YET_IMPLEMENTED_NOFAIL }                                                        \
  type name args { return _XBT_CONCAT(P, name) args2; }

#define UNIMPLEMENTED_WRAPPED_PMPI_CALL_NORETURN(type, name, args, args2)                                              \
  type _XBT_CONCAT(P, name) args { NOT_YET_IMPLEMENTED_NULL }                                                          \
  type name args { return _XBT_CONCAT(P, name) args2; }

/* MPI User level calls */

WRAPPED_PMPI_CALL_NORETURN(double, MPI_Wtick,(void),())
WRAPPED_PMPI_CALL_NORETURN(double, MPI_Wtime,(void),())
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Abort,(MPI_Comm comm, int errorcode),(comm, errorcode))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Accumulate,(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win),( origin_addr,origin_count, origin_datatype,target_rank,target_disp, target_count,target_datatype,op, win))
WRAPPED_PMPI_CALL(int, MPI_Address, (const void* location, MPI_Aint* address), (location, address))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Allgather,(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm),(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Allgatherv,(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs,MPI_Datatype recvtype, MPI_Comm comm),(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm))
WRAPPED_PMPI_CALL(int,MPI_Alloc_mem,(MPI_Aint size, MPI_Info info, void *baseptr),(size, info, baseptr))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Allreduce,(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm),(sendbuf, recvbuf, count, datatype, op, comm))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Alltoall,(const void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount,MPI_Datatype recvtype, MPI_Comm comm),(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Alltoallv,(const void *sendbuf, const int *sendcounts, const int *senddisps, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *recvdisps, MPI_Datatype recvtype, MPI_Comm comm),(sendbuf, sendcounts, senddisps, sendtype, recvbuf, recvcounts, recvdisps, recvtype, comm))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Alltoallw,(const void *sendbuf, const int *sendcnts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcnts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm),( sendbuf, sendcnts, sdispls, sendtypes, recvbuf, recvcnts, rdispls, recvtypes, comm))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Attr_delete,(MPI_Comm comm, int keyval) ,(comm, keyval))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Attr_get,(MPI_Comm comm, int keyval, void* attr_value, int* flag) ,(comm, keyval, attr_value, flag))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Attr_put,(MPI_Comm comm, int keyval, void* attr_value) ,(comm, keyval, attr_value))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Barrier,(MPI_Comm comm),(comm))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Bcast,(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm),(buf, count, datatype, root, comm))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Bsend_init,(const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request),(buf, count, datatype, dest, tag, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Bsend,(const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) ,(buf, count, datatype, dest, tag, comm))
WRAPPED_PMPI_CALL(int,MPI_Buffer_attach,(void* buffer, int size) ,(buffer, size))
WRAPPED_PMPI_CALL(int,MPI_Buffer_detach,(void* buffer, int* size) ,(buffer, size))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Cart_coords,(MPI_Comm comm, int rank, int maxdims, int* coords) ,(comm, rank, maxdims, coords))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Cart_create,(MPI_Comm comm, int ndims, const int* dims, const int* periods, int reorder, MPI_Comm* comm_cart) ,(comm, ndims, dims, periods, reorder, comm_cart))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Cartdim_get,(MPI_Comm comm, int* ndims) ,(comm, ndims))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Cart_get,(MPI_Comm comm, int maxdims, int* dims, int* periods, int* coords) ,(comm, maxdims, dims, periods, coords))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Cart_rank,(MPI_Comm comm, const int* coords, int* rank) ,(comm, coords, rank))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Cart_shift,(MPI_Comm comm, int direction, int displ, int* source, int* dest) ,(comm, direction, displ, source, dest))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Cart_sub,(MPI_Comm comm, const int* remain_dims, MPI_Comm* comm_new) ,(comm, remain_dims, comm_new))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Comm_compare,(MPI_Comm comm, MPI_Comm comm2, int *result),(comm, comm2, result))
WRAPPED_PMPI_CALL(int,MPI_Comm_create_keyval,(MPI_Comm_copy_attr_function* copy_fn, MPI_Comm_delete_attr_function* delete_fn, int* keyval, void* extra_state),(copy_fn,delete_fn,keyval,extra_state))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Comm_create,(MPI_Comm comm, MPI_Group group, MPI_Comm * newcomm),(comm, group, newcomm))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Comm_delete_attr ,(MPI_Comm comm, int comm_keyval),(comm,comm_keyval))
WRAPPED_PMPI_CALL_ERRHANDLER(int,MPI_Comm_disconnect,(MPI_Comm * comm),(comm), (*comm))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Comm_dup,(MPI_Comm comm, MPI_Comm * newcomm),(comm, newcomm))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Comm_dup_with_info,(MPI_Comm comm, MPI_Info info, MPI_Comm * newcomm),(comm,info,newcomm))
WRAPPED_PMPI_CALL(int,MPI_Comm_free_keyval,(int* keyval) ,( keyval))
WRAPPED_PMPI_CALL_ERRHANDLER(int,MPI_Comm_free,(MPI_Comm * comm),(comm), (*comm))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Comm_get_attr ,(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag),(comm, comm_keyval, attribute_val, flag))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Comm_get_info ,(MPI_Comm comm, MPI_Info* info),(comm, info))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Comm_get_name ,(MPI_Comm comm, char* name, int* len),(comm, name, len))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Comm_set_name ,(MPI_Comm comm, const char* name),(comm, name))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Comm_group,(MPI_Comm comm, MPI_Group * group),(comm, group))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Comm_rank,(MPI_Comm comm, int *rank),(comm, rank))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Comm_set_attr ,(MPI_Comm comm, int comm_keyval, void *attribute_val),( comm, comm_keyval, attribute_val))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Comm_set_info ,(MPI_Comm comm, MPI_Info info),(comm, info))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Comm_size,(MPI_Comm comm, int *size),(comm, size))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Comm_split,(MPI_Comm comm, int color, int key, MPI_Comm* comm_out),(comm, color, key, comm_out))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Comm_split_type,(MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm *newcomm),(comm, split_type, key, info, newcomm))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Comm_create_group,(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm* comm_out),(comm, group, tag, comm_out))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Comm_test_inter,(MPI_Comm comm, int* flag) ,(comm, flag))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Comm_call_errhandler,(MPI_Comm comm,int errorcode),(comm, errorcode))
WRAPPED_PMPI_CALL(int,MPI_Comm_create_errhandler,( MPI_Comm_errhandler_fn *function, MPI_Errhandler *errhandler),( function, errhandler))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Comm_get_errhandler,(MPI_Comm comm, MPI_Errhandler* errhandler) ,(comm, errhandler))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Comm_set_errhandler,(MPI_Comm comm, MPI_Errhandler errhandler) ,(comm, errhandler))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Compare_and_swap,(const void *origin_addr, void *compare_addr,
        void *result_addr, MPI_Datatype datatype, int target_rank, MPI_Aint target_disp, MPI_Win win), (origin_addr, compare_addr, result_addr, datatype, target_rank, target_disp, win))
WRAPPED_PMPI_CALL(int,MPI_Dims_create,(int nnodes, int ndims, int* dims) ,(nnodes, ndims, dims))
WRAPPED_PMPI_CALL(int,MPI_Errhandler_free,(MPI_Errhandler* errhandler) ,(errhandler))
WRAPPED_PMPI_CALL(int,MPI_Errhandler_create,(MPI_Handler_function* function, MPI_Errhandler* errhandler) ,(function, errhandler))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Errhandler_get,(MPI_Comm comm, MPI_Errhandler* errhandler) ,(comm, errhandler))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Errhandler_set,(MPI_Comm comm, MPI_Errhandler errhandler) ,(comm, errhandler))
WRAPPED_PMPI_CALL_NORETURN(MPI_Errhandler, MPI_Errhandler_f2c,(MPI_Fint errhandler),(errhandler))
WRAPPED_PMPI_CALL_NORETURN(MPI_Fint, MPI_Errhandler_c2f,(MPI_Errhandler errhandler),(errhandler))
WRAPPED_PMPI_CALL(int,MPI_Error_class,(int errorcode, int* errorclass) ,(errorcode, errorclass))
WRAPPED_PMPI_CALL_NORETURN(int,MPI_Error_string,(int errorcode, char* string, int* resultlen) ,(errorcode, string, resultlen))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Exscan,(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm),(sendbuf, recvbuf, count, datatype, op, comm))
WRAPPED_PMPI_CALL(int,MPI_Finalized,(int * flag),(flag))
WRAPPED_PMPI_CALL(int,MPI_Finalize,(void),())
WRAPPED_PMPI_CALL(int,MPI_Free_mem,(void *baseptr),(baseptr))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Gather,(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm),(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Gatherv,(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs,MPI_Datatype recvtype, int root, MPI_Comm comm),(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm))
WRAPPED_PMPI_CALL_NORETURN(MPI_Aint,MPI_Aint_add,(MPI_Aint add, MPI_Aint disp),(add, disp))
WRAPPED_PMPI_CALL_NORETURN(MPI_Aint,MPI_Aint_diff,(MPI_Aint add, MPI_Aint disp),(add, disp))
WRAPPED_PMPI_CALL(int,MPI_Get_address,(const void *location, MPI_Aint * address),(location, address))
WRAPPED_PMPI_CALL(int,MPI_Get_count,(const MPI_Status * status, MPI_Datatype datatype, int *count),(status, datatype, count))
WRAPPED_PMPI_CALL(int,MPI_Get_library_version ,(char *version,int *len),(version,len))
WRAPPED_PMPI_CALL(int,MPI_Get_processor_name,(char *name, int *resultlen),(name, resultlen))
WRAPPED_PMPI_CALL(int,MPI_Get_version ,(int *version,int *subversion),(version,subversion))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Get,( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win),(origin_addr,origin_count, origin_datatype,target_rank, target_disp, target_count,target_datatype,win))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Get_accumulate, (const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, void *result_addr, int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win),(origin_addr, origin_count, origin_datatype, result_addr, result_count, result_datatype, target_rank, target_disp, target_count, target_datatype, op, win))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Fetch_and_op, (const void *origin_addr, void *result_addr, MPI_Datatype datatype, int target_rank, MPI_Aint target_disp, MPI_Op op, MPI_Win win),(origin_addr, result_addr, datatype, target_rank, target_disp, op, win))
WRAPPED_PMPI_CALL(int,MPI_Group_compare,(MPI_Group group1, MPI_Group group2, int *result),(group1, group2, result))
WRAPPED_PMPI_CALL(int,MPI_Group_difference,(MPI_Group group1, MPI_Group group2, MPI_Group * newgroup),(group1, group2, newgroup))
WRAPPED_PMPI_CALL(int,MPI_Group_excl,(MPI_Group group, int n, const int *ranks, MPI_Group * newgroup),(group, n, ranks, newgroup))
WRAPPED_PMPI_CALL(int,MPI_Group_free,(MPI_Group * group),(group))
WRAPPED_PMPI_CALL(int,MPI_Group_incl,(MPI_Group group, int n, const int *ranks, MPI_Group * newgroup),(group, n, ranks, newgroup))
WRAPPED_PMPI_CALL(int,MPI_Group_intersection,(MPI_Group group1, MPI_Group group2, MPI_Group * newgroup),(group1, group2, newgroup))
WRAPPED_PMPI_CALL(int,MPI_Group_range_excl,(MPI_Group group, int n, int ranges[][3], MPI_Group * newgroup),(group, n, ranges, newgroup))
WRAPPED_PMPI_CALL(int,MPI_Group_range_incl,(MPI_Group group, int n, int ranges[][3], MPI_Group * newgroup),(group, n, ranges, newgroup))
WRAPPED_PMPI_CALL(int,MPI_Group_rank,(MPI_Group group, int *rank),(group, rank))
WRAPPED_PMPI_CALL(int,MPI_Group_size,(MPI_Group group, int *size),(group, size))
WRAPPED_PMPI_CALL(int,MPI_Group_translate_ranks,(MPI_Group group1, int n, const int *ranks1, MPI_Group group2, int *ranks2),(group1, n, ranks1, group2, ranks2))
WRAPPED_PMPI_CALL(int,MPI_Group_union,(MPI_Group group1, MPI_Group group2, MPI_Group * newgroup),(group1, group2, newgroup))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Ibarrier,(MPI_Comm comm, MPI_Request *request),(comm,request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Ibcast,(void* buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request),(buf, count, datatype, root, comm, request))

WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Iallgather,(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request),(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Iallgatherv,(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs,MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request),(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Iallreduce,(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request),(sendbuf, recvbuf, count, datatype, op, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Ialltoall,(const void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount,MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request),(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Ialltoallv,(const void *sendbuf, const int *sendcounts, const int *senddisps, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *recvdisps, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request),(sendbuf, sendcounts, senddisps, sendtype, recvbuf, recvcounts, recvdisps, recvtype, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Ialltoallw,(const void *sendbuf, const int *sendcnts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcnts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPI_Request *request),( sendbuf, sendcnts, sdispls, sendtypes, recvbuf, recvcnts, rdispls, recvtypes, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Igather,(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request),(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Igatherv,(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs,MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request),(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Ireduce_scatter_block,(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op,MPI_Comm comm, MPI_Request *request),(sendbuf, recvbuf, recvcount, datatype, op, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Ireduce_scatter,(const void *sendbuf, void *recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request),(sendbuf, recvbuf, recvcounts, datatype, op, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Ireduce,(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request),(sendbuf, recvbuf, count, datatype, op, root, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Iexscan,(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request),(sendbuf, recvbuf, count, datatype, op, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Iscan,(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request),(sendbuf, recvbuf, count, datatype, op, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Iscatter,(const void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount, MPI_Datatype recvtype,int root, MPI_Comm comm, MPI_Request *request),(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Iscatterv,(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount,MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request),(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, request))

WRAPPED_PMPI_CALL(int,MPI_Info_create,( MPI_Info *info),( info))
WRAPPED_PMPI_CALL(int,MPI_Info_delete,(MPI_Info info, const char *key),(info, key))
WRAPPED_PMPI_CALL(int,MPI_Info_dup,(MPI_Info info, MPI_Info *newinfo),(info, newinfo))
WRAPPED_PMPI_CALL(int,MPI_Info_free,( MPI_Info *info),( info))
WRAPPED_PMPI_CALL(int,MPI_Info_get,(MPI_Info info,const char *key,int valuelen, char *value, int *flag),(info,key,valuelen, value, flag))
WRAPPED_PMPI_CALL(int,MPI_Info_get_nkeys,( MPI_Info info, int *nkeys),(info, nkeys))
WRAPPED_PMPI_CALL(int,MPI_Info_get_nthkey,( MPI_Info info, int n, char *key),( info, n, key))
WRAPPED_PMPI_CALL(int,MPI_Info_get_valuelen,( MPI_Info info, const char *key, int *valuelen, int *flag),( info, key, valuelen, flag))
WRAPPED_PMPI_CALL(int,MPI_Info_set,( MPI_Info info, const char *key, const char *value),( info, key, value))
WRAPPED_PMPI_CALL(int,MPI_Initialized,(int* flag) ,(flag))
WRAPPED_PMPI_CALL(int,MPI_Init,(int *argc, char ***argv),(argc, argv))
WRAPPED_PMPI_CALL(int,MPI_Init_thread,(int *argc, char ***argv, int required, int *provided),(argc, argv, required, provided))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Iprobe,(int source, int tag, MPI_Comm comm, int* flag, MPI_Status* status) ,(source, tag, comm, flag, status))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Irecv,(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Request * request),(buf, count, datatype, src, tag, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Isend,(const void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request * request),(buf, count, datatype, dst, tag, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Ibsend,(const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request) ,(buf, count, datatype, dest, tag, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Issend,(const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request) ,(buf, count, datatype, dest, tag, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Irsend,(const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request),(buf, count, datatype, dest, tag, comm, request))
WRAPPED_PMPI_CALL(int,MPI_Is_thread_main,(int *flag),(flag))
WRAPPED_PMPI_CALL(int,MPI_Keyval_create,(MPI_Copy_function* copy_fn, MPI_Delete_function* delete_fn, int* keyval, void* extra_state) ,(copy_fn, delete_fn, keyval, extra_state))
WRAPPED_PMPI_CALL(int,MPI_Keyval_free,(int* keyval) ,(keyval))
WRAPPED_PMPI_CALL(int,MPI_Op_create,(MPI_User_function * function, int commute, MPI_Op * op),(function, commute, op))
WRAPPED_PMPI_CALL(int,MPI_Op_free,(MPI_Op * op),(op))
WRAPPED_PMPI_CALL(int,MPI_Op_commutative,(MPI_Op op, int *commute), (op, commute))
WRAPPED_PMPI_CALL(int,MPI_Pack_size,(int incount, MPI_Datatype datatype, MPI_Comm comm, int* size) ,(incount, datatype, comm, size))
WRAPPED_PMPI_CALL(int,MPI_Pack,(const void* inbuf, int incount, MPI_Datatype type, void* outbuf, int outcount, int* position, MPI_Comm comm) ,(inbuf, incount, type, outbuf, outcount, position, comm))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Probe,(int source, int tag, MPI_Comm comm, MPI_Status* status) ,(source, tag, comm, status))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Put,(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win),(origin_addr,origin_count, origin_datatype,target_rank,target_disp, target_count,target_datatype, win))
WRAPPED_PMPI_CALL(int,MPI_Query_thread,(int *provided),(provided))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Raccumulate,(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request* request),( origin_addr,origin_count, origin_datatype,target_rank,target_disp, target_count,target_datatype,op, win, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Recv_init,(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Request * request),(buf, count, datatype, src, tag, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Recv,(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status * status),(buf, count, datatype, src, tag, comm, status))
WRAPPED_PMPI_CALL(int,MPI_Reduce_local,(const void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype, MPI_Op op),(inbuf, inoutbuf, count, datatype, op))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Reduce_scatter_block,(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op,MPI_Comm comm),(sendbuf, recvbuf, recvcount, datatype, op, comm))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Reduce_scatter,(const void *sendbuf, void *recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm),(sendbuf, recvbuf, recvcounts, datatype, op, comm))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Reduce,(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm),(sendbuf, recvbuf, count, datatype, op, root, comm))
WRAPPED_PMPI_CALL(int,MPI_Request_free,(MPI_Request * request),(request))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Rget,(void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request* request),(origin_addr,origin_count, origin_datatype,target_rank, target_disp, target_count,target_datatype,win, request))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Rget_accumulate, (const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, void *result_addr, int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request* request),(origin_addr, origin_count, origin_datatype, result_addr, result_count, result_datatype, target_rank, target_disp, target_count, target_datatype, op, win, request))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Rput,(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request* request),(origin_addr,origin_count, origin_datatype,target_rank,target_disp, target_count,target_datatype, win, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Scan,(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm),(sendbuf, recvbuf, count, datatype, op, comm))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Scatter,(const void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount, MPI_Datatype recvtype,int root, MPI_Comm comm),(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Scatterv,(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount,MPI_Datatype recvtype, int root, MPI_Comm comm),(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Send_init,(const void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request * request),(buf, count, datatype, dst, tag, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Sendrecv_replace,(void *buf, int count, MPI_Datatype datatype, int dst, int sendtag, int src, int recvtag,MPI_Comm comm, MPI_Status * status),(buf, count, datatype, dst, sendtag, src, recvtag, comm, status))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Isendrecv_replace,(void *buf, int count, MPI_Datatype datatype, int dst, int sendtag, int src, int recvtag,MPI_Comm comm, MPI_Request* req),(buf, count, datatype, dst, sendtag, src, recvtag, comm, req))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Sendrecv,(const void *sendbuf, int sendcount, MPI_Datatype sendtype,int dst, int sendtag, void *recvbuf, int recvcount,MPI_Datatype recvtype, int src, int recvtag, MPI_Comm comm, MPI_Status * status),(sendbuf, sendcount, sendtype, dst, sendtag, recvbuf, recvcount, recvtype, src, recvtag,comm, status))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Isendrecv,(const void *sendbuf, int sendcount, MPI_Datatype sendtype,int dst, int sendtag, void *recvbuf, int recvcount,MPI_Datatype recvtype, int src, int recvtag, MPI_Comm comm, MPI_Request* req),(sendbuf, sendcount, sendtype, dst, sendtag, recvbuf, recvcount, recvtype, src, recvtag,comm, req))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Send,(const void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm),(buf, count, datatype, dst, tag, comm))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Ssend_init,(const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request),(buf, count, datatype, dest, tag, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Ssend,(const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) ,(buf, count, datatype, dest, tag, comm))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Rsend_init,(const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request),(buf, count, datatype, dest, tag, comm, request))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Rsend,(const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) ,(buf, count, datatype, dest, tag, comm))
WRAPPED_PMPI_CALL(int,MPI_Startall,(int count, MPI_Request * requests),(count, requests))
WRAPPED_PMPI_CALL(int,MPI_Start,(MPI_Request * request),(request))
WRAPPED_PMPI_CALL(int,MPI_Testall,(int count, MPI_Request* requests, int* flag, MPI_Status* statuses) ,(count, requests, flag, statuses))
WRAPPED_PMPI_CALL(int,MPI_Testany,(int count, MPI_Request requests[], int *index, int *flag, MPI_Status * status),(count, requests, index, flag, status))
WRAPPED_PMPI_CALL(int,MPI_Test,(MPI_Request * request, int *flag, MPI_Status * status),(request, flag, status))
WRAPPED_PMPI_CALL(int,MPI_Testsome,(int incount, MPI_Request* requests, int* outcount, int* indices, MPI_Status* statuses) ,(incount, requests, outcount, indices, statuses))
WRAPPED_PMPI_CALL(int,MPI_Request_get_status,( MPI_Request request, int *flag, MPI_Status *status),( request, flag, status))
WRAPPED_PMPI_CALL(int,MPI_Grequest_complete,( MPI_Request request),( request))
WRAPPED_PMPI_CALL(int,MPI_Grequest_start,(MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function *free_fn,MPI_Grequest_cancel_function *cancel_fn, void *extra_state, MPI_Request *request),( query_fn, free_fn, cancel_fn, extra_state, request))
WRAPPED_PMPI_CALL(int,MPI_Type_commit,(MPI_Datatype* datatype) ,(datatype))
WRAPPED_PMPI_CALL(int,MPI_Type_contiguous,(int count, MPI_Datatype old_type, MPI_Datatype* newtype) ,(count, old_type, newtype))
WRAPPED_PMPI_CALL(int,MPI_Type_create_hindexed_block,(int count, int blocklength, const MPI_Aint* indices, MPI_Datatype old_type, MPI_Datatype* newtype) ,(count, blocklength, indices, old_type, newtype))
WRAPPED_PMPI_CALL(int,MPI_Type_create_hindexed,(int count, const int* blocklens, const MPI_Aint* indices, MPI_Datatype old_type,MPI_Datatype* new_type) ,(count, blocklens,indices,old_type,new_type))
WRAPPED_PMPI_CALL(int,MPI_Type_create_hvector,(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* new_type) ,(count, blocklen, stride, old_type, new_type))
WRAPPED_PMPI_CALL(int,MPI_Type_create_indexed_block,(int count, int blocklength, const int* indices,MPI_Datatype old_type,MPI_Datatype *newtype),(count, blocklength, indices, old_type, newtype))
WRAPPED_PMPI_CALL(int,MPI_Type_create_indexed,(int count, const int* blocklens, const int* indices, MPI_Datatype old_type, MPI_Datatype* newtype) ,(count, blocklens, indices, old_type, newtype))
WRAPPED_PMPI_CALL(int,MPI_Type_create_keyval,(MPI_Type_copy_attr_function* copy_fn, MPI_Type_delete_attr_function* delete_fn, int* keyval,void* extra_state),(copy_fn,delete_fn,keyval,extra_state))
WRAPPED_PMPI_CALL(int,MPI_Type_create_resized,(MPI_Datatype oldtype,MPI_Aint lb, MPI_Aint extent, MPI_Datatype *newtype),(oldtype,lb, extent, newtype))
WRAPPED_PMPI_CALL(int,MPI_Type_create_struct,(int count, const int* blocklens, const MPI_Aint* indices, const MPI_Datatype* old_types,MPI_Datatype* newtype) ,(count, blocklens, indices, old_types, newtype))
WRAPPED_PMPI_CALL(int,MPI_Type_create_subarray,(int ndims,const int *array_of_sizes, const int *array_of_subsizes, const int *array_of_starts, int order,MPI_Datatype oldtype, MPI_Datatype *newtype),(ndims,array_of_sizes, array_of_subsizes, array_of_starts, order, oldtype, newtype))
WRAPPED_PMPI_CALL(int,MPI_Type_delete_attr ,(MPI_Datatype type, int type_keyval),(type,type_keyval))
WRAPPED_PMPI_CALL(int,MPI_Type_dup,(MPI_Datatype datatype, MPI_Datatype * newdatatype),(datatype, newdatatype))
WRAPPED_PMPI_CALL(int,MPI_Type_extent,(MPI_Datatype datatype, MPI_Aint * extent),(datatype, extent))
WRAPPED_PMPI_CALL(int,MPI_Type_free_keyval,(int* keyval) ,(keyval))
WRAPPED_PMPI_CALL(int,MPI_Type_free,(MPI_Datatype * datatype),(datatype))
WRAPPED_PMPI_CALL(int,MPI_Type_get_attr ,(MPI_Datatype type, int type_keyval, void *attribute_val, int* flag),( type, type_keyval, attribute_val, flag))
WRAPPED_PMPI_CALL(int,MPI_Type_get_contents,(MPI_Datatype datatype, int max_integers, int max_addresses, int max_datatypes, int* array_of_integers, MPI_Aint* array_of_addresses, MPI_Datatype *array_of_datatypes),(datatype, max_integers, max_addresses,max_datatypes, array_of_integers, array_of_addresses, array_of_datatypes))
WRAPPED_PMPI_CALL(int,MPI_Type_get_envelope,( MPI_Datatype datatype, int *num_integers, int *num_addresses, int *num_datatypes, int *combiner),(datatype, num_integers, num_addresses, num_datatypes, combiner))
WRAPPED_PMPI_CALL(int,MPI_Type_get_extent,(MPI_Datatype datatype, MPI_Aint * lb, MPI_Aint * extent),(datatype, lb, extent))
WRAPPED_PMPI_CALL(int,MPI_Type_get_extent_x,(MPI_Datatype datatype, MPI_Count * lb, MPI_Count * extent),(datatype, lb, extent))
WRAPPED_PMPI_CALL(int,MPI_Type_get_name,(MPI_Datatype datatype, char * name, int* len),(datatype,name,len))
WRAPPED_PMPI_CALL(int,MPI_Type_get_true_extent,(MPI_Datatype datatype, MPI_Aint * lb, MPI_Aint * extent),(datatype, lb, extent))
WRAPPED_PMPI_CALL(int,MPI_Type_get_true_extent_x,(MPI_Datatype datatype, MPI_Count * lb, MPI_Count * extent),(datatype, lb, extent))
WRAPPED_PMPI_CALL(int,MPI_Type_hindexed,(int count, const int* blocklens, const MPI_Aint* indices, MPI_Datatype old_type, MPI_Datatype* newtype) ,(count, blocklens, indices, old_type, newtype))
WRAPPED_PMPI_CALL(int,MPI_Type_hvector,(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* newtype) ,(count, blocklen, stride, old_type, newtype))
WRAPPED_PMPI_CALL(int,MPI_Type_indexed,(int count, const int* blocklens, const int* indices, MPI_Datatype old_type, MPI_Datatype* newtype) ,(count, blocklens, indices, old_type, newtype))
WRAPPED_PMPI_CALL(int,MPI_Type_lb,(MPI_Datatype datatype, MPI_Aint * disp),(datatype, disp))
WRAPPED_PMPI_CALL(int,MPI_Type_set_attr ,(MPI_Datatype type, int type_keyval, void *attribute_val),( type, type_keyval, attribute_val))
WRAPPED_PMPI_CALL(int,MPI_Type_set_name,(MPI_Datatype datatype, const char * name),(datatype, name))
WRAPPED_PMPI_CALL(int,MPI_Type_size,(MPI_Datatype datatype, int *size),(datatype, size))
WRAPPED_PMPI_CALL(int,MPI_Type_size_x,(MPI_Datatype datatype, MPI_Count *size),(datatype, size))
WRAPPED_PMPI_CALL(int,MPI_Type_struct,(int count, const int* blocklens, const MPI_Aint* indices, const MPI_Datatype* old_types, MPI_Datatype* newtype) ,(count, blocklens, indices, old_types, newtype))
WRAPPED_PMPI_CALL(int,MPI_Type_ub,(MPI_Datatype datatype, MPI_Aint * disp),(datatype, disp))
WRAPPED_PMPI_CALL(int,MPI_Type_vector,(int count, int blocklen, int stride, MPI_Datatype old_type, MPI_Datatype* newtype) ,(count, blocklen, stride, old_type, newtype))
WRAPPED_PMPI_CALL(int,MPI_Unpack,(const void* inbuf, int insize, int* position, void* outbuf, int outcount, MPI_Datatype type, MPI_Comm comm) ,(inbuf, insize, position, outbuf, outcount, type, comm))
WRAPPED_PMPI_CALL(int,MPI_Waitall,(int count, MPI_Request requests[], MPI_Status status[]),(count, requests, status))
WRAPPED_PMPI_CALL(int,MPI_Waitany,(int count, MPI_Request requests[], int *index, MPI_Status * status),(count, requests, index, status))
WRAPPED_PMPI_CALL(int,MPI_Wait,(MPI_Request * request, MPI_Status * status),(request, status))
WRAPPED_PMPI_CALL(int,MPI_Waitsome,(int incount, MPI_Request requests[], int *outcount, int *indices, MPI_Status status[]),(incount, requests, outcount, indices, status))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_complete,(MPI_Win win),(win))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Win_create,( void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win),( base, size, disp_unit, info, comm,win))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Win_allocate,(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *base, MPI_Win *win),(size, disp_unit, info, comm, base, win))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Win_allocate_shared,(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *base, MPI_Win *win),(size, disp_unit, info, comm, base, win))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_attach,(MPI_Win win, void *base, MPI_Aint size),(win, base, size))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_detach,(MPI_Win win, const void *base),(win, base))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int,MPI_Win_create_dynamic,( MPI_Info info, MPI_Comm comm, MPI_Win *win),(info, comm,win))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_fence,( int assert,MPI_Win win),( assert, win))
WRAPPED_PMPI_CALL_ERRHANDLER(int,MPI_Win_free,( MPI_Win* win),(win), (*win))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_get_group,(MPI_Win win, MPI_Group * group),(win, group))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_get_name,(MPI_Win win, char * name, int* len),(win,name,len))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_get_info,(MPI_Win win, MPI_Info * info),(win,info))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_post,(MPI_Group group, int assert, MPI_Win win),(group, assert, win))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_set_name,(MPI_Win win, const char * name),(win, name))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_set_info,(MPI_Win win, MPI_Info info),(win,info))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_start,(MPI_Group group, int assert, MPI_Win win),(group, assert, win))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_wait,(MPI_Win win),(win))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_lock,(int lock_type, int rank, int assert, MPI_Win win) ,(lock_type, rank, assert, win))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_unlock,(int rank, MPI_Win win),(rank, win))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_lock_all,(int assert, MPI_Win win) ,(assert, win))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_unlock_all,(MPI_Win win),(win))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_flush,(int rank, MPI_Win win),(rank, win))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_flush_local,(int rank, MPI_Win win),(rank, win))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_flush_all,(MPI_Win win),(win))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_flush_local_all,(MPI_Win win),(win))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_get_attr, (MPI_Win win, int type_keyval, void *attribute_val, int* flag), (win, type_keyval, attribute_val, flag))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_set_attr, (MPI_Win win, int type_keyval, void *att), (win, type_keyval, att))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_delete_attr, (MPI_Win win, int comm_keyval), (win, comm_keyval))
WRAPPED_PMPI_CALL(int,MPI_Win_create_keyval,(MPI_Win_copy_attr_function* copy_fn,
                              MPI_Win_delete_attr_function* delete_fn, int* keyval, void* extra_state), (copy_fn, delete_fn, keyval, extra_state))
WRAPPED_PMPI_CALL(int,MPI_Win_free_keyval,(int* keyval), (keyval))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_shared_query,(MPI_Win win, int rank, MPI_Aint* size, int* disp_unit, void* baseptr),(win, rank, size, disp_unit, baseptr))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_set_errhandler,(MPI_Win win, MPI_Errhandler errhandler) ,(win, errhandler))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_call_errhandler,(MPI_Win win,int errorcode),(win, errorcode))
WRAPPED_PMPI_CALL(int,MPI_Win_create_errhandler,( MPI_Win_errhandler_function *function, MPI_Errhandler *errhandler),( function, errhandler))
WRAPPED_PMPI_CALL_ERRHANDLER_WIN(int,MPI_Win_get_errhandler,(MPI_Win win, MPI_Errhandler* errhandler) ,(win, errhandler))
WRAPPED_PMPI_CALL_NORETURN(MPI_Comm, MPI_Comm_f2c,(MPI_Fint comm),(comm))
WRAPPED_PMPI_CALL_NORETURN(MPI_Datatype, MPI_Type_f2c,(MPI_Fint datatype),(datatype))
WRAPPED_PMPI_CALL_NORETURN(MPI_Fint, MPI_Comm_c2f,(MPI_Comm comm),(comm))
WRAPPED_PMPI_CALL_NORETURN(MPI_Fint, MPI_Group_c2f,(MPI_Group group),(group))
WRAPPED_PMPI_CALL_NORETURN(MPI_Fint, MPI_Info_c2f,(MPI_Info info),(info))
WRAPPED_PMPI_CALL_NORETURN(MPI_Fint, MPI_Op_c2f,(MPI_Op op),(op))
WRAPPED_PMPI_CALL_NORETURN(MPI_Fint, MPI_Request_c2f,(MPI_Request request) ,(request))
WRAPPED_PMPI_CALL_NORETURN(MPI_Fint, MPI_Type_c2f,(MPI_Datatype datatype),( datatype))
WRAPPED_PMPI_CALL_NORETURN(MPI_Fint, MPI_Win_c2f,(MPI_Win win),(win))
WRAPPED_PMPI_CALL_NORETURN(MPI_Group, MPI_Group_f2c,(MPI_Fint group),( group))
WRAPPED_PMPI_CALL_NORETURN(MPI_Info, MPI_Info_f2c,(MPI_Fint info),(info))
WRAPPED_PMPI_CALL_NORETURN(MPI_Op, MPI_Op_f2c,(MPI_Fint op),(op))
WRAPPED_PMPI_CALL_NORETURN(MPI_Request, MPI_Request_f2c,(MPI_Fint request),(request))
WRAPPED_PMPI_CALL_NORETURN(MPI_Win, MPI_Win_f2c,(MPI_Fint win),(win))
WRAPPED_PMPI_CALL(int, MPI_Cancel,(MPI_Request* request) ,(request))
WRAPPED_PMPI_CALL(int, MPI_Test_cancelled,(const MPI_Status* status, int* flag) ,(status, flag))
WRAPPED_PMPI_CALL(int, MPI_Status_set_cancelled,(MPI_Status *status,int flag),(status,flag))
WRAPPED_PMPI_CALL(int, MPI_Status_set_elements,( MPI_Status *status, MPI_Datatype datatype, int count),( status, datatype, count))
WRAPPED_PMPI_CALL(int, MPI_Status_set_elements_x,( MPI_Status *status, MPI_Datatype datatype, MPI_Count count),( status, datatype, count))
WRAPPED_PMPI_CALL_ERRHANDLER_COMM(int, MPI_File_open,(MPI_Comm comm, const char *filename, int amode, MPI_Info info, MPI_File *fh),(comm, filename, amode, info, fh))
WRAPPED_PMPI_CALL_ERRHANDLER(int, MPI_File_close,(MPI_File *fh), (fh), (*fh))
WRAPPED_PMPI_CALL(int, MPI_File_delete,(const char *filename, MPI_Info info), (filename, info))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_set_info,(MPI_File fh, MPI_Info info), (fh, info))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_get_info,(MPI_File fh, MPI_Info *info_used), (fh, info_used))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_read_at,(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status), (fh, offset, buf, count, datatype, status))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_read_at_all,(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status), (fh, offset, buf, count, datatype, status))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_write_at,(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status), (fh, offset, buf, count, datatype, status))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_write_at_all,(MPI_File fh, MPI_Offset offset, const void *buf,int count, MPI_Datatype datatype, MPI_Status *status), (fh, offset, buf, count, datatype, status))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_read,(MPI_File fh, void *buf, int count,MPI_Datatype datatype, MPI_Status *status), (fh, buf, count, datatype, status))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_read_all,(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status), (fh, buf, count, datatype, status))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_write,(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status), (fh, buf, count, datatype, status))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_write_all,(MPI_File fh, const void *buf, int count,MPI_Datatype datatype, MPI_Status *status), (fh, buf, count, datatype, status))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_seek,(MPI_File fh, MPI_Offset offset, int whenace), (fh, offset, whenace))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_get_position,(MPI_File fh, MPI_Offset *offset), (fh, offset))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_read_shared,(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status), (fh, buf, count, datatype, status))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_write_shared,(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status), (fh, buf, count, datatype, status))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_read_ordered,(MPI_File fh, void *buf, int count,MPI_Datatype datatype, MPI_Status *status), (fh, buf, count, datatype, status))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_write_ordered,(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status), (fh, buf, count, datatype, status))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_seek_shared,(MPI_File fh, MPI_Offset offset, int whence), (fh, offset, whence))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_get_position_shared,(MPI_File fh, MPI_Offset *offset), (fh, offset))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_get_size,(MPI_File fh, MPI_Offset *size), (fh, size))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_set_size,(MPI_File fh, MPI_Offset size), (fh, size))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_get_group,(MPI_File fh, MPI_Group *group), (fh, group))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_get_amode,(MPI_File fh, int *amode), (fh, amode))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_sync,(MPI_File fh), (fh))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_call_errhandler,(MPI_File fh, int errorcode), (fh, errorcode))
WRAPPED_PMPI_CALL(int, MPI_File_create_errhandler,(MPI_File_errhandler_function *function, MPI_Errhandler *errhandler),(function, errhandler))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_set_errhandler,( MPI_File fh, MPI_Errhandler errhandler), (fh, errhandler))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_get_errhandler,( MPI_File fh, MPI_Errhandler *errhandler), (fh, errhandler))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_set_view,(MPI_File fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype, const char *datarep, MPI_Info info), (fh, disp, etype, filetype, datarep, info))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_get_view,(MPI_File fh, MPI_Offset *disp, MPI_Datatype *etype, MPI_Datatype *filetype, char *datarep), (fh, disp, etype, filetype, datarep))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_get_type_extent,(MPI_File fh, MPI_Datatype datatype, MPI_Aint *extent), (fh, datatype, extent))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_set_atomicity,(MPI_File fh, int flag), (fh, flag))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_get_atomicity,(MPI_File fh, int *flag), (fh, flag))
WRAPPED_PMPI_CALL_ERRHANDLER_FILE(int, MPI_File_get_byte_offset,(MPI_File fh, MPI_Offset offset, MPI_Offset *disp), (fh, offset, disp))
/*
  Unimplemented Calls - both PMPI and MPI calls are generated.
  When implementing, please move ahead, swap UNIMPLEMENTED_WRAPPED_PMPI_CALL for WRAPPED_PMPI_CALL,
  and implement PMPI version of the function in smpi_pmpi.cpp file
*/


UNIMPLEMENTED_WRAPPED_PMPI_CALL_NOFAIL(int,MPI_Add_error_class,( int *errorclass),( errorclass))
UNIMPLEMENTED_WRAPPED_PMPI_CALL_NOFAIL(int,MPI_Add_error_code,(int errorclass, int *errorcode),(errorclass, errorcode))
UNIMPLEMENTED_WRAPPED_PMPI_CALL_NOFAIL(int,MPI_Add_error_string,( int errorcode, char *string),(errorcode, string))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Cart_map,(MPI_Comm comm_old, int ndims, const int* dims, const int* periods, int* newrank) ,(comm_old, ndims, dims, periods, newrank))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Close_port,(const char *port_name),( port_name))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Comm_accept,(const char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm),( port_name, info, root, comm, newcomm))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_Comm_connect,
                                (const char* port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm* newcomm),
                                (port_name, info, root, comm, newcomm))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Comm_get_parent,( MPI_Comm *parent),( parent))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Comm_idup,( MPI_Comm comm, MPI_Comm *newcomm, MPI_Request* request),( comm,  newcomm, request))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Comm_join,( int fd, MPI_Comm *intercomm),( fd, intercomm))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Comm_remote_group,(MPI_Comm comm, MPI_Group* group) ,(comm, group))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Comm_remote_size,(MPI_Comm comm, int* size) ,(comm, size))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Comm_spawn,(const char *command, char **argv, int maxprocs, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *intercomm, int* array_of_errcodes),( command, argv, maxprocs, info, root, comm, intercomm, array_of_errcodes))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Comm_spawn_multiple,(int count, char **array_of_commands, char*** array_of_argv, int* array_of_maxprocs, MPI_Info* array_of_info, int root, MPI_Comm comm, MPI_Comm *intercomm, int* array_of_errcodes), (count, array_of_commands, array_of_argv, array_of_maxprocs, array_of_info, root, comm, intercomm, array_of_errcodes))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_Dist_graph_create, (MPI_Comm comm_old, int n, const int* sources, const int* degrees, const int* destinations, const int* weights, MPI_Info info, int reorder, MPI_Comm* comm_dist_graph), (comm_old, n, sources, degrees, destinations, weights, info, reorder, comm_dist_graph))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_Dist_graph_create_adjacent, (MPI_Comm comm_old, int indegree, const int* sources, const int* sourceweights, int outdegree, const int* destinations, const int* destweights, MPI_Info info, int reorder, MPI_Comm* comm_dist_graph), (comm_old, indegree, sources, sourceweights, outdegree, destinations, destweights, info, reorder, comm_dist_graph))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_Dist_graph_neighbors, (MPI_Comm comm, int maxindegree, int* sources, int* sourceweights, int maxoutdegree, int* destinations, int* destweights), (comm, maxindegree, sources, sourceweights, maxoutdegree, destinations, destweights))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_Dist_graph_neighbors_count, (MPI_Comm comm, int *indegree, int *outdegree, int *weighted), (comm, indegree, outdegree, weighted))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(MPI_Fint, MPI_File_c2f,(MPI_File file), (file))
UNIMPLEMENTED_WRAPPED_PMPI_CALL_NORETURN(MPI_File, MPI_File_f2c,(MPI_Fint file), (file))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_preallocate,(MPI_File fh, MPI_Offset size), (fh, size))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_iread_at,(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Request *request), (fh, offset, buf, count, datatype, request))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_iwrite_at,(MPI_File fh, MPI_Offset offset, const void *buf,int count, MPI_Datatype datatype, MPI_Request *request), (fh, offset, buf, count, datatype, request))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_iread_at_all,(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Request *request), (fh, offset, buf, count, datatype, request))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_iwrite_at_all,(MPI_File fh, MPI_Offset offset, const void *buf,int count, MPI_Datatype datatype, MPI_Request *request), (fh, offset, buf, count, datatype, request))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_iread,(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Request *request), (fh, buf, count, datatype, request))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_iwrite,(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request), (fh, buf, count, datatype, request))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_iread_all,(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Request *request), (fh, buf, count, datatype, request))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_iwrite_all,(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request), (fh, buf, count, datatype, request))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_iread_shared,(MPI_File fh, void *buf, int count,MPI_Datatype datatype, MPI_Request *request), (fh, buf, count, datatype, request))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_iwrite_shared,(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request), (fh, buf, count, datatype, request))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_read_at_all_begin,(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype), (fh, offset, buf, count, datatype))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_read_at_all_end,(MPI_File fh, void *buf, MPI_Status *status), (fh, buf, status))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_write_at_all_begin,(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype), (fh, offset, buf, count, datatype))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_write_at_all_end,(MPI_File fh, const void *buf, MPI_Status *status), (fh, buf, status))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_read_all_begin,(MPI_File fh, void *buf, int count, MPI_Datatype datatype), (fh, buf, count, datatype))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_read_all_end,(MPI_File fh, void *buf, MPI_Status *status), (fh, buf, status))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_write_all_begin,(MPI_File fh, const void *buf, int count, MPI_Datatype datatype), (fh, buf, count, datatype))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_write_all_end,(MPI_File fh, const void *buf, MPI_Status *status), (fh, buf, status))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_read_ordered_begin,(MPI_File fh, void *buf, int count, MPI_Datatype datatype), (fh, buf, count, datatype))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_read_ordered_end,(MPI_File fh, void *buf, MPI_Status *status), (fh, buf, status))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_write_ordered_begin,(MPI_File fh, const void *buf, int count, MPI_Datatype datatype), (fh, buf, count, datatype))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_File_write_ordered_end,(MPI_File fh, const void *buf, MPI_Status *status), (fh, buf, status))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Get_elements,(MPI_Status* status, MPI_Datatype datatype, int* elements) ,(status, datatype, elements))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Get_elements_x,(MPI_Status* status, MPI_Datatype datatype, MPI_Count* elements) ,(status, datatype, elements))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Graph_create,(MPI_Comm comm_old, int nnodes, const int* index, const int* edges, int reorder, MPI_Comm* comm_graph) ,(comm_old, nnodes, index, edges, reorder, comm_graph))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Graphdims_get,(MPI_Comm comm, int* nnodes, int* nedges) ,(comm, nnodes, nedges))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Graph_get,(MPI_Comm comm, int maxindex, int maxedges, int* index, int* edges) ,(comm, maxindex, maxedges, index, edges))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Graph_map,(MPI_Comm comm_old, int nnodes, const int* index, const int* edges, int* newrank) ,(comm_old, nnodes, index, edges, newrank))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Graph_neighbors_count,(MPI_Comm comm, int rank, int* nneighbors) ,(comm, rank, nneighbors))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Graph_neighbors,(MPI_Comm comm, int rank, int maxneighbors, int* neighbors) ,(comm, rank, maxneighbors, neighbors))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Improbe,(int source, int tag, MPI_Comm comm, int* flag, MPI_Message *message, MPI_Status* status) ,(source, tag, comm, flag, message, status))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Imrecv,(void *buf, int count, MPI_Datatype datatype, MPI_Message *message, MPI_Request *request),(buf, count, datatype, message, request))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Intercomm_create,(MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm, int remote_leader, int tag,MPI_Comm* comm_out) ,(local_comm, local_leader, peer_comm, remote_leader, tag, comm_out))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Intercomm_merge,(MPI_Comm comm, int high, MPI_Comm* comm_out) ,(comm, high, comm_out))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Lookup_name,( char *service_name, MPI_Info info, char *port_name),( service_name, info, port_name))
UNIMPLEMENTED_WRAPPED_PMPI_CALL_NORETURN(XBT_PUBLIC MPI_Message, MPI_Message_f2c, (MPI_Fint message), (message))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(XBT_PUBLIC MPI_Fint, MPI_Message_c2f, (MPI_Message message), (message))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Mprobe,(int source, int tag, MPI_Comm comm, MPI_Message *message, MPI_Status* status) ,(source, tag, comm, message, status))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Mrecv,(void *buf, int count, MPI_Datatype datatype, MPI_Message *message, MPI_Status* status),(buf, count, datatype, message, status))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Neighbor_allgather,(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm),(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Neighbor_allgatherv,(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs,MPI_Datatype recvtype, MPI_Comm comm),(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Neighbor_alltoall,(const void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount,MPI_Datatype recvtype, MPI_Comm comm),(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Neighbor_alltoallv,(const void *sendbuf, const int *sendcounts, const int *senddisps, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *recvdisps, MPI_Datatype recvtype, MPI_Comm comm),(sendbuf, sendcounts, senddisps, sendtype, recvbuf, recvcounts, recvdisps, recvtype, comm))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Neighbor_alltoallw,(const void *sendbuf, const int *sendcnts, const MPI_Aint *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcnts, const MPI_Aint *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm),( sendbuf, sendcnts, sdispls, sendtypes, recvbuf, recvcnts, rdispls, recvtypes, comm))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Ineighbor_allgather,(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request),(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Ineighbor_allgatherv,(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs,MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request),(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, request))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Ineighbor_alltoall,(const void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount,MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request),(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Ineighbor_alltoallv,(const void *sendbuf, const int *sendcounts, const int *senddisps, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *recvdisps, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request),(sendbuf, sendcounts, senddisps, sendtype, recvbuf, recvcounts, recvdisps, recvtype, comm, request))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Ineighbor_alltoallw,(const void *sendbuf, const int *sendcnts, const MPI_Aint *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcnts, const MPI_Aint *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm, MPI_Request *request),( sendbuf, sendcnts, sdispls, sendtypes, recvbuf, recvcnts, rdispls, recvtypes, comm, request))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Open_port,( MPI_Info info, char *port_name),( info,port_name))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Pack_external,(char *datarep, void *inbuf, int incount, MPI_Datatype datatype, void *outbuf, MPI_Aint outcount, MPI_Aint *position),(datarep, inbuf, incount, datatype, outbuf, outcount, position))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Pack_external_size,(char *datarep, int incount, MPI_Datatype datatype, MPI_Aint *size),(datarep, incount, datatype, size))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Pcontrol,(const int level, ... ),(level))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Publish_name,( char *service_name, MPI_Info info, char *port_name),( service_name, info, port_name))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Register_datarep, (char *datarep, MPI_Datarep_conversion_function *read_conversion_fn, MPI_Datarep_conversion_function *write_conversion_fn, MPI_Datarep_extent_function *dtype_file_extent_fn, void *extra_state) ,(datarep, read_conversion_fn, write_conversion_fn, dtype_file_extent_fn, extra_state))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(XBT_PUBLIC int, MPI_Status_f2c, (MPI_Fint *f_status, MPI_Status *c_status), (f_status, c_status))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(XBT_PUBLIC int, MPI_Status_c2f, (MPI_Status *c_status, MPI_Fint *f_status), (c_status, f_status))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Topo_test,(MPI_Comm comm, int* top_type) ,(comm, top_type))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Type_create_f90_integer,(int count, MPI_Datatype *datatype),(count, datatype))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Type_create_f90_real,(int prec, int exp, MPI_Datatype *datatype),(prec, exp, datatype))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Type_create_f90_complex,(int prec, int exp, MPI_Datatype *datatype),(prec, exp, datatype))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Type_create_darray,(int size, int rank, int ndims, int* array_of_gsizes, int* array_of_distribs, int* array_of_dargs, int* array_of_psizes,int order, MPI_Datatype oldtype, MPI_Datatype *newtype) ,(size, rank, ndims, array_of_gsizes,array_of_distribs, array_of_dargs, array_of_psizes,order,oldtype, newtype))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Type_match_size,(int typeclass,int size,MPI_Datatype *datatype),(typeclass,size,datatype))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Unpack_external,(char *datarep, void *inbuf, MPI_Aint insize, MPI_Aint *position, void *outbuf, int outcount, MPI_Datatype datatype),( datarep, inbuf, insize, position, outbuf, outcount, datatype))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Unpublish_name,( char *service_name, MPI_Info info, char *port_name),( service_name, info, port_name))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int,MPI_Win_test,(MPI_Win win, int *flag),(win, flag))
UNIMPLEMENTED_WRAPPED_PMPI_CALL_NOFAIL(int,MPI_Win_sync,(MPI_Win win),(win))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_Parrived, (MPI_Request request, int partition, int *flag), (request, partition, flag))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_Pready, (int partitions, MPI_Request request), (partitions, request))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_Pready_range, (int partition_low, int partition_high, MPI_Request request),(partition_low, partition_high, request))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_Pready_list, (int length, int partition_list[], MPI_Request request), (length, partition_list, request))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_Precv_init, (void* buf, int partitions, MPI_Count count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Info info, MPI_Request *request), (buf, partitions, count, datatype, source, tag, comm, info, request))
UNIMPLEMENTED_WRAPPED_PMPI_CALL(int, MPI_Psend_init, (const void* buf, int partitions, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Info info, MPI_Request *request), (buf, partitions, count, datatype, dest, tag, comm, info, request))

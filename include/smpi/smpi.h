/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_H
#define SMPI_H

#include <stddef.h>
#include <sys/time.h>
#include <xbt/misc.h>
#include <xbt/function_types.h>

#define MPI_CALL(type,name,args) \
  type name args; \
  type P##name args

SG_BEGIN_DECL()
#define MPI_THREAD_SINGLE     0
#define MPI_THREAD_FUNNELED   1
#define MPI_THREAD_SERIALIZED 2
#define MPI_THREAD_MULTIPLE   3
//FIXME: check values
#define MPI_MAX_PROCESSOR_NAME 100
#define MPI_MAX_ERROR_STRING   100
#define MPI_MAX_DATAREP_STRIN  100
#define MPI_MAX_INFO_KEY       100
#define MPI_MAX_INFO_VAL       100
#define MPI_MAX_OBJECT_NAME    100
#define MPI_MAX_PORT_NAME      100
#define SMPI_RAND_SEED 5
#define MPI_ANY_SOURCE -1
#define MPI_PROC_NULL -2
#define MPI_ANY_TAG -1
#define MPI_UNDEFINED -1
// errorcodes
#define MPI_SUCCESS       0
#define MPI_ERR_COMM      1
#define MPI_ERR_ARG       2
#define MPI_ERR_TYPE      3
#define MPI_ERR_REQUEST   4
#define MPI_ERR_INTERN    5
#define MPI_ERR_COUNT     6
#define MPI_ERR_RANK      7
#define MPI_ERR_TAG       8
#define MPI_ERR_TRUNCATE  9
#define MPI_ERR_GROUP    10
#define MPI_ERR_OP       11
#define MPI_ERR_OTHER    12
#define MPI_IDENT     0
#define MPI_SIMILAR   1
#define MPI_UNEQUAL   2
#define MPI_CONGRUENT 3
#define MPI_WTIME_IS_GLOBAL 1
typedef ptrdiff_t MPI_Aint;
typedef long long MPI_Offset;

struct s_smpi_mpi_datatype;
typedef struct s_smpi_mpi_datatype *MPI_Datatype;

typedef struct {
  int MPI_SOURCE;
  int MPI_TAG;
  int MPI_ERROR;
  int count;
} MPI_Status;

#define MPI_STATUS_IGNORE NULL

#define MPI_DATATYPE_NULL NULL
extern MPI_Datatype MPI_CHAR;
extern MPI_Datatype MPI_SHORT;
extern MPI_Datatype MPI_INT;
extern MPI_Datatype MPI_LONG;
extern MPI_Datatype MPI_LONG_LONG;
#define MPI_LONG_LONG_INT MPI_LONG_LONG
extern MPI_Datatype MPI_SIGNED_CHAR;
extern MPI_Datatype MPI_UNSIGNED_CHAR;
extern MPI_Datatype MPI_UNSIGNED_SHORT;
extern MPI_Datatype MPI_UNSIGNED;
extern MPI_Datatype MPI_UNSIGNED_LONG;
extern MPI_Datatype MPI_UNSIGNED_LONG_LONG;
extern MPI_Datatype MPI_FLOAT;
extern MPI_Datatype MPI_DOUBLE;
extern MPI_Datatype MPI_LONG_DOUBLE;
extern MPI_Datatype MPI_WCHAR;
extern MPI_Datatype MPI_C_BOOL;
extern MPI_Datatype MPI_INT8_T;
extern MPI_Datatype MPI_INT16_T;
extern MPI_Datatype MPI_INT32_T;
extern MPI_Datatype MPI_INT64_T;
extern MPI_Datatype MPI_UINT8_T;
#define MPI_BYTE MPI_UINT8_T
extern MPI_Datatype MPI_UINT16_T;
extern MPI_Datatype MPI_UINT32_T;
extern MPI_Datatype MPI_UINT64_T;
extern MPI_Datatype MPI_C_FLOAT_COMPLEX;
#define MPI_C_COMPLEX MPI_C_FLOAT_COMPLEX
extern MPI_Datatype MPI_C_DOUBLE_COMPLEX;
extern MPI_Datatype MPI_C_LONG_DOUBLE_COMPLEX;
extern MPI_Datatype MPI_AINT;
extern MPI_Datatype MPI_OFFSET;

//The following are datatypes for the MPI functions MPI_MAXLOC  and MPI_MINLOC.
extern MPI_Datatype MPI_FLOAT_INT;
extern MPI_Datatype MPI_LONG_INT;
extern MPI_Datatype MPI_DOUBLE_INT;
extern MPI_Datatype MPI_SHORT_INT;
extern MPI_Datatype MPI_2INT;
extern MPI_Datatype MPI_LONG_DOUBLE_INT;

typedef void MPI_User_function(void *invec, void *inoutvec, int *len,
                               MPI_Datatype * datatype);
struct s_smpi_mpi_op;
typedef struct s_smpi_mpi_op *MPI_Op;

#define MPI_OP_NULL NULL
extern MPI_Op MPI_MAX;
extern MPI_Op MPI_MIN;
extern MPI_Op MPI_MAXLOC;
extern MPI_Op MPI_MINLOC;
extern MPI_Op MPI_SUM;
extern MPI_Op MPI_PROD;
extern MPI_Op MPI_LAND;
extern MPI_Op MPI_LOR;
extern MPI_Op MPI_LXOR;
extern MPI_Op MPI_BAND;
extern MPI_Op MPI_BOR;
extern MPI_Op MPI_BXOR;

struct s_smpi_mpi_group;
typedef struct s_smpi_mpi_group *MPI_Group;

#define MPI_GROUP_NULL NULL

extern MPI_Group MPI_GROUP_EMPTY;

struct s_smpi_mpi_communicator;
typedef struct s_smpi_mpi_communicator *MPI_Comm;

#define MPI_COMM_NULL NULL
extern MPI_Comm MPI_COMM_WORLD;
#define MPI_COMM_SELF smpi_process_comm_self()

struct s_smpi_mpi_request;
typedef struct s_smpi_mpi_request *MPI_Request;

#define MPI_REQUEST_NULL NULL

MPI_CALL(XBT_PUBLIC(int), MPI_Init, (int *argc, char ***argv));
MPI_CALL(XBT_PUBLIC(int), MPI_Finalize, (void));
MPI_CALL(XBT_PUBLIC(int), MPI_Init_thread,
                            (int *argc, char ***argv, int required,
                             int *provided));
MPI_CALL(XBT_PUBLIC(int), MPI_Query_thread, (int *provided));
MPI_CALL(XBT_PUBLIC(int), MPI_Is_thread_main, (int *flag));
MPI_CALL(XBT_PUBLIC(int), MPI_Abort, (MPI_Comm comm, int errorcode));
MPI_CALL(XBT_PUBLIC(double), MPI_Wtime, (void));

MPI_CALL(XBT_PUBLIC(int), MPI_Address, (void *location, MPI_Aint * address));

MPI_CALL(XBT_PUBLIC(int), MPI_Type_free, (MPI_Datatype * datatype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_size, (MPI_Datatype datatype, int *size));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_get_extent,
                            (MPI_Datatype datatype, MPI_Aint * lb,
                             MPI_Aint * extent));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_extent, (MPI_Datatype datatype, MPI_Aint * extent));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_lb, (MPI_Datatype datatype, MPI_Aint * disp));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_ub, (MPI_Datatype datatype, MPI_Aint * disp));

MPI_CALL(XBT_PUBLIC(int), MPI_Op_create,
                            (MPI_User_function * function, int commute,
                             MPI_Op * op));
MPI_CALL(XBT_PUBLIC(int), MPI_Op_free, (MPI_Op * op));

MPI_CALL(XBT_PUBLIC(int), MPI_Group_free, (MPI_Group * group));
MPI_CALL(XBT_PUBLIC(int), MPI_Group_size, (MPI_Group group, int *size));
MPI_CALL(XBT_PUBLIC(int), MPI_Group_rank, (MPI_Group group, int *rank));
MPI_CALL(XBT_PUBLIC(int), MPI_Group_translate_ranks,
                            (MPI_Group group1, int n,
                             int *ranks1, MPI_Group group2,
                             int *ranks2));
MPI_CALL(XBT_PUBLIC(int), MPI_Group_compare,
                            (MPI_Group group1, MPI_Group group2,
                             int *result));
MPI_CALL(XBT_PUBLIC(int), MPI_Group_union,
                            (MPI_Group group1, MPI_Group group2,
                             MPI_Group * newgroup));
MPI_CALL(XBT_PUBLIC(int), MPI_Group_intersection,
                            (MPI_Group group1, MPI_Group group2,
                             MPI_Group * newgroup));
MPI_CALL(XBT_PUBLIC(int), MPI_Group_difference,
                            (MPI_Group group1, MPI_Group group2,
                             MPI_Group * newgroup));
MPI_CALL(XBT_PUBLIC(int), MPI_Group_incl,
                            (MPI_Group group, int n, int *ranks,
                             MPI_Group * newgroup));
MPI_CALL(XBT_PUBLIC(int), MPI_Group_excl,
                            (MPI_Group group, int n, int *ranks,
                             MPI_Group * newgroup));
MPI_CALL(XBT_PUBLIC(int), MPI_Group_range_incl,
                            (MPI_Group group, int n, int ranges[][3],
                             MPI_Group * newgroup));
MPI_CALL(XBT_PUBLIC(int), MPI_Group_range_excl,
                            (MPI_Group group, int n, int ranges[][3],
                             MPI_Group * newgroup));

MPI_CALL(XBT_PUBLIC(int), MPI_Comm_rank, (MPI_Comm comm, int *rank));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_size, (MPI_Comm comm, int *size));
MPI_CALL(XBT_PUBLIC(int), MPI_Get_processor_name, (char *name, int *resultlen));
MPI_CALL(XBT_PUBLIC(int), MPI_Get_count,
                            (MPI_Status * status, MPI_Datatype datatype,
                             int *count));

MPI_CALL(XBT_PUBLIC(int), MPI_Comm_group, (MPI_Comm comm, MPI_Group * group));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_compare,
                            (MPI_Comm comm1, MPI_Comm comm2, int *result));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_dup, (MPI_Comm comm, MPI_Comm * newcomm));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_create,
                            (MPI_Comm comm, MPI_Group group, MPI_Comm * newcomm));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_free, (MPI_Comm * comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_split, (MPI_Comm comm, int color, int key, MPI_Comm* comm_out));

MPI_CALL(XBT_PUBLIC(int), MPI_Send_init,
                            (void *buf, int count, MPI_Datatype datatype,
                             int dst, int tag, MPI_Comm comm,
                             MPI_Request * request));
MPI_CALL(XBT_PUBLIC(int), MPI_Recv_init,
                            (void *buf, int count, MPI_Datatype datatype,
                             int src, int tag, MPI_Comm comm,
                             MPI_Request * request));
MPI_CALL(XBT_PUBLIC(int), MPI_Start, (MPI_Request * request));
MPI_CALL(XBT_PUBLIC(int), MPI_Startall, (int count, MPI_Request * requests));
MPI_CALL(XBT_PUBLIC(int), MPI_Request_free, (MPI_Request * request));
MPI_CALL(XBT_PUBLIC(int), MPI_Irecv,
                            (void *buf, int count, MPI_Datatype datatype,
                             int src, int tag, MPI_Comm comm,
                             MPI_Request * request));
MPI_CALL(XBT_PUBLIC(int), MPI_Isend,
                            (void *buf, int count, MPI_Datatype datatype,
                             int dst, int tag, MPI_Comm comm,
                             MPI_Request * request));
MPI_CALL(XBT_PUBLIC(int), MPI_Recv,
                            (void *buf, int count, MPI_Datatype datatype,
                             int src, int tag, MPI_Comm comm,
                             MPI_Status * status));
MPI_CALL(XBT_PUBLIC(int), MPI_Send,
                            (void *buf, int count, MPI_Datatype datatype,
                             int dst, int tag, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Sendrecv,
                            (void *sendbuf, int sendcount,
                             MPI_Datatype sendtype, int dst, int sendtag,
                             void *recvbuf, int recvcount,
                             MPI_Datatype recvtype, int src, int recvtag,
                             MPI_Comm comm, MPI_Status * status));
MPI_CALL(XBT_PUBLIC(int), MPI_Sendrecv_replace,
                            (void *buf, int count,
                             MPI_Datatype datatype, int dst,
                             int sendtag, int src, int recvtag,
                             MPI_Comm comm, MPI_Status * status));

MPI_CALL(XBT_PUBLIC(int), MPI_Test,
                            (MPI_Request * request, int *flag, MPI_Status* status));
MPI_CALL(XBT_PUBLIC(int), MPI_Testany,
                            (int count, MPI_Request requests[], int *index,
                             int *flag, MPI_Status * status));
MPI_CALL(XBT_PUBLIC(int), MPI_Wait, (MPI_Request * request, MPI_Status * status));
MPI_CALL(XBT_PUBLIC(int), MPI_Waitany,
                            (int count, MPI_Request requests[], int *index,
                             MPI_Status * status));
MPI_CALL(XBT_PUBLIC(int), MPI_Waitall,
                            (int count, MPI_Request requests[],
                             MPI_Status status[]));
MPI_CALL(XBT_PUBLIC(int), MPI_Waitsome,
                            (int incount, MPI_Request requests[],
                             int *outcount, int *indices,
                             MPI_Status status[]));

MPI_CALL(XBT_PUBLIC(int), MPI_Bcast,
                            (void *buf, int count, MPI_Datatype datatype,
                             int root, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Barrier, (MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Gather,
                            (void *sendbuf, int sendcount,
                             MPI_Datatype sendtype, void *recvbuf,
                             int recvcount, MPI_Datatype recvtype, int root,
                             MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Gatherv,
                            (void *sendbuf, int sendcount,
                             MPI_Datatype sendtype, void *recvbuf,
                             int *recvcounts, int *displs,
                             MPI_Datatype recvtype, int root,
                             MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Allgather,
                            (void *sendbuf, int sendcount,
                             MPI_Datatype sendtype, void *recvbuf,
                             int recvcount, MPI_Datatype recvtype,
                             MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Allgatherv,
                            (void *sendbuf, int sendcount,
                             MPI_Datatype sendtype, void *recvbuf,
                             int *recvcounts, int *displs,
                             MPI_Datatype recvtype, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Scatter,
                            (void *sendbuf, int sendcount,
                             MPI_Datatype sendtype, void *recvbuf,
                             int recvcount, MPI_Datatype recvtype, int root,
                             MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Scatterv,
                            (void *sendbuf, int *sendcounts, int *displs,
                             MPI_Datatype sendtype, void *recvbuf,
                             int recvcount, MPI_Datatype recvtype,
                             int root, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Reduce,
                            (void *sendbuf, void *recvbuf, int count,
                             MPI_Datatype datatype, MPI_Op op, int root,
                             MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Allreduce,
                            (void *sendbuf, void *recvbuf, int count,
                             MPI_Datatype datatype, MPI_Op op,
                             MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Scan,
                            (void *sendbuf, void *recvbuf, int count,
                             MPI_Datatype datatype, MPI_Op op, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Reduce_scatter,
                            (void *sendbuf, void *recvbuf,
                             int *recvcounts, MPI_Datatype datatype,
                             MPI_Op op, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Alltoall,
                            (void *sendbuf, int sendcount,
                             MPI_Datatype sendtype, void *recvbuf,
                             int recvcount, MPI_Datatype recvtype,
                             MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Alltoallv,
                            (void *sendbuf, int *sendcounts,
                             int *senddisps, MPI_Datatype sendtype,
                             void *recvbuf, int *recvcounts,
                             int *recvdisps, MPI_Datatype recvtype,
                             MPI_Comm comm));

//FIXME: these are not yet implemented
typedef void MPI_Handler_function(MPI_Comm*, int*, ...);
typedef void* MPI_Errhandler;
typedef int MPI_Copy_function(MPI_Comm oldcomm, int keyval, void* extra_state, void* attribute_val_in,
                              void* attribute_val_out, int* flag);
typedef int MPI_Delete_function(MPI_Comm comm, int keyval, void* attribute_val, void* extra_state);

MPI_CALL(XBT_PUBLIC(int), MPI_Pack_size, (int incount, MPI_Datatype datatype, MPI_Comm comm, int* size));
MPI_CALL(XBT_PUBLIC(int), MPI_Cart_coords, (MPI_Comm comm, int rank, int maxdims, int* coords));
MPI_CALL(XBT_PUBLIC(int), MPI_Cart_create, (MPI_Comm comm_old, int ndims, int* dims, int* periods, int reorder, MPI_Comm* comm_cart));
MPI_CALL(XBT_PUBLIC(int), MPI_Cart_get, (MPI_Comm comm, int maxdims, int* dims, int* periods, int* coords));
MPI_CALL(XBT_PUBLIC(int), MPI_Cart_map, (MPI_Comm comm_old, int ndims, int* dims, int* periods, int* newrank));
MPI_CALL(XBT_PUBLIC(int), MPI_Cart_rank, (MPI_Comm comm, int* coords, int* rank));
MPI_CALL(XBT_PUBLIC(int), MPI_Cart_shift, (MPI_Comm comm, int direction, int displ, int* source, int* dest));
MPI_CALL(XBT_PUBLIC(int), MPI_Cart_sub, (MPI_Comm comm, int* remain_dims, MPI_Comm* comm_new));
MPI_CALL(XBT_PUBLIC(int), MPI_Cartdim_get, (MPI_Comm comm, int* ndims));
MPI_CALL(XBT_PUBLIC(int), MPI_Graph_create, (MPI_Comm comm_old, int nnodes, int* index, int* edges, int reorder, MPI_Comm* comm_graph));
MPI_CALL(XBT_PUBLIC(int), MPI_Graph_get, (MPI_Comm comm, int maxindex, int maxedges, int* index, int* edges));
MPI_CALL(XBT_PUBLIC(int), MPI_Graph_map, (MPI_Comm comm_old, int nnodes, int* index, int* edges, int* newrank));
MPI_CALL(XBT_PUBLIC(int), MPI_Graph_neighbors, (MPI_Comm comm, int rank, int maxneighbors, int* neighbors));
MPI_CALL(XBT_PUBLIC(int), MPI_Graph_neighbors_count, (MPI_Comm comm, int rank, int* nneighbors));
MPI_CALL(XBT_PUBLIC(int), MPI_Graphdims_get, (MPI_Comm comm, int* nnodes, int* nedges));
MPI_CALL(XBT_PUBLIC(int), MPI_Topo_test, (MPI_Comm comm, int* top_type));
MPI_CALL(XBT_PUBLIC(int), MPI_Error_class, (int errorcode, int* errorclass));
MPI_CALL(XBT_PUBLIC(int), MPI_Errhandler_create, (MPI_Handler_function* function, MPI_Errhandler* errhandler));
MPI_CALL(XBT_PUBLIC(int), MPI_Errhandler_free, (MPI_Errhandler* errhandler));
MPI_CALL(XBT_PUBLIC(int), MPI_Errhandler_get, (MPI_Comm comm, MPI_Errhandler* errhandler));
MPI_CALL(XBT_PUBLIC(int), MPI_Error_string, (int errorcode, char* string, int* resultlen));
MPI_CALL(XBT_PUBLIC(int), MPI_Errhandler_set, (MPI_Comm comm, MPI_Errhandler errhandler));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_contiguous, (int count, MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC(int), MPI_Cancel, (MPI_Request* request));
MPI_CALL(XBT_PUBLIC(int), MPI_Buffer_attach, (void* buffer, int size));
MPI_CALL(XBT_PUBLIC(int), MPI_Buffer_detach, (void* buffer, int* size));
MPI_CALL(XBT_PUBLIC(int), MPI_Testsome, (int incount, MPI_Request* requests, int* outcount, int* indices, MPI_Status* statuses));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_test_inter, (MPI_Comm comm, int* flag));
MPI_CALL(XBT_PUBLIC(int), MPI_Unpack, (void* inbuf, int insize, int* position, void* outbuf, int outcount, MPI_Datatype type, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_commit, (MPI_Datatype* datatype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_hindexed, (int count, int* blocklens, MPI_Aint* indices, MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_hvector, (int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_indexed, (int count, int* blocklens, int* indices, MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_struct, (int count, int* blocklens, MPI_Aint* indices, MPI_Datatype* old_types, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_vector, (int count, int blocklen, int stride, MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC(int), MPI_Ssend, (void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Ssend_init, (void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request));
MPI_CALL(XBT_PUBLIC(int), MPI_Intercomm_create, (MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm, int remote_leader, int tag, MPI_Comm* comm_out));
MPI_CALL(XBT_PUBLIC(int), MPI_Intercomm_merge, (MPI_Comm comm, int high, MPI_Comm* comm_out));
MPI_CALL(XBT_PUBLIC(int), MPI_Bsend, (void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Bsend_init, (void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request));
MPI_CALL(XBT_PUBLIC(int), MPI_Ibsend, (void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_remote_group, (MPI_Comm comm, MPI_Group* group));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_remote_size, (MPI_Comm comm, int* size));
MPI_CALL(XBT_PUBLIC(int), MPI_Issend, (void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request));
MPI_CALL(XBT_PUBLIC(int), MPI_Probe, (int source, int tag, MPI_Comm comm, MPI_Status* status));
MPI_CALL(XBT_PUBLIC(int), MPI_Attr_delete, (MPI_Comm comm, int keyval));
MPI_CALL(XBT_PUBLIC(int), MPI_Attr_get, (MPI_Comm comm, int keyval, void* attr_value, int* flag));
MPI_CALL(XBT_PUBLIC(int), MPI_Attr_put, (MPI_Comm comm, int keyval, void* attr_value));
MPI_CALL(XBT_PUBLIC(int), MPI_Rsend, (void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Rsend_init, (void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request));
MPI_CALL(XBT_PUBLIC(int), MPI_Irsend, (void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request));
MPI_CALL(XBT_PUBLIC(int), MPI_Keyval_create, (MPI_Copy_function* copy_fn, MPI_Delete_function* delete_fn, int* keyval, void* extra_state));
MPI_CALL(XBT_PUBLIC(int), MPI_Keyval_free, (int* keyval));
MPI_CALL(XBT_PUBLIC(int), MPI_Test_cancelled, (MPI_Status* status, int* flag));
MPI_CALL(XBT_PUBLIC(int), MPI_Pack, (void* inbuf, int incount, MPI_Datatype type, void* outbuf, int outcount, int* position, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Testall, (int count, MPI_Request* requests, int* flag, MPI_Status* statuses));
MPI_CALL(XBT_PUBLIC(int), MPI_Get_elements, (MPI_Status* status, MPI_Datatype datatype, int* elements));
MPI_CALL(XBT_PUBLIC(int), MPI_Dims_create, (int nnodes, int ndims, int* dims));
MPI_CALL(XBT_PUBLIC(int), MPI_Iprobe, (int source, int tag, MPI_Comm comm, int* flag, MPI_Status* status));
MPI_CALL(XBT_PUBLIC(int), MPI_Initialized, (int* flag));
//FIXME: End of all the not yet implemented stuff

// smpi functions
XBT_IMPORT_NO_EXPORT(int) smpi_simulated_main(int argc, char **argv);
XBT_PUBLIC(MPI_Comm) smpi_process_comm_self(void);
/*
XBT_PUBLIC(void) smpi_exit(int);
*/

XBT_PUBLIC(unsigned int) smpi_sleep(unsigned int secs);
XBT_PUBLIC(int) smpi_gettimeofday(struct timeval *tv, struct timezone *tz);
XBT_PUBLIC(void) smpi_sample_1(int global, const char *file, int line,
                               int max);
XBT_PUBLIC(int) smpi_sample_2(int global, const char *file, int line);
XBT_PUBLIC(void) smpi_sample_3(int global, const char *file, int line);
XBT_PUBLIC(void) smpi_sample_flops(double flops);

#define SMPI_SAMPLE_LOCAL(num) for(smpi_sample_1(0, __FILE__, __LINE__, num); \
                                   smpi_sample_2(0, __FILE__, __LINE__);      \
                                   smpi_sample_3(0, __FILE__, __LINE__))

#define SMPI_SAMPLE_GLOBAL(num) for(smpi_sample_1(1, __FILE__, __LINE__, num); \
                                    smpi_sample_2(1, __FILE__, __LINE__);      \
                                    smpi_sample_3(1, __FILE__, __LINE__))

#define SMPI_SAMPLE_DELAY(flops) for(smpi_sample_flops(flops); 0; )

XBT_PUBLIC(void *) smpi_shared_malloc(size_t size, const char *file,
                                      int line);
#define SMPI_SHARED_MALLOC(size) smpi_shared_malloc(size, __FILE__, __LINE__)

XBT_PUBLIC(void) smpi_shared_free(void *data);
#define SMPI_SHARED_FREE(data) smpi_shared_free(data)

/* Fortran specific stuff */
XBT_PUBLIC(int) MAIN__(void);

SG_END_DECL()
#endif

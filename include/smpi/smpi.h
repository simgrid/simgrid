/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_H
#define SMPI_H

#include <unistd.h>
#include <sys/time.h>
#if _POSIX_TIMERS
#include <time.h>
#endif

#include <stddef.h>
#include <xbt/misc.h>
#include <xbt/function_types.h>

#ifdef _WIN32
#define MPI_CALL(type,name,args) \
  type name args; \
  type P##name args
#else
#define MPI_CALL(type,name,args) \
  type name args __attribute__((weak)); \
  type P##name args
#endif

SG_BEGIN_DECL()
#define MPI_THREAD_SINGLE     0
#define MPI_THREAD_FUNNELED   1
#define MPI_THREAD_SERIALIZED 2
#define MPI_THREAD_MULTIPLE   3
//FIXME: check values
#define MPI_MAX_PROCESSOR_NAME 100
#define MPI_MAX_NAME_STRING    100
#define MPI_MAX_ERROR_STRING   100
#define MPI_MAX_DATAREP_STRIN  100
#define MPI_MAX_INFO_KEY       100
#define MPI_MAX_INFO_VAL       100
#define MPI_MAX_OBJECT_NAME    100
#define MPI_MAX_PORT_NAME      100
#define MPI_MAX_LIBRARY_VERSION_STRING 100
#define SMPI_RAND_SEED 5
#define MPI_ANY_SOURCE -555
#define MPI_BOTTOM (void *)-111
#define MPI_PROC_NULL -666
#define MPI_ANY_TAG -444
#define MPI_UNDEFINED -333
#define MPI_IN_PLACE (void *)-222

// errorcodes
#define MPI_SUCCESS                    0
#define MPI_ERR_COMM                   1
#define MPI_ERR_ARG                    2
#define MPI_ERR_TYPE                   3
#define MPI_ERR_REQUEST                4
#define MPI_ERR_INTERN                 5
#define MPI_ERR_COUNT                  6
#define MPI_ERR_RANK                   7
#define MPI_ERR_TAG                    8
#define MPI_ERR_TRUNCATE               9
#define MPI_ERR_GROUP                 10
#define MPI_ERR_OP                    11
#define MPI_ERR_OTHER                 12
#define MPI_ERR_IN_STATUS             13
#define MPI_ERR_PENDING               14
#define MPI_ERR_BUFFER                15
#define MPI_ERR_NAME                  16
#define MPI_ERR_DIMS                  17
#define MPI_ERR_TOPOLOGY              18
#define MPI_ERR_NO_MEM                19
#define MPI_ERR_WIN                   20
#define MPI_ERR_INFO_VALUE            21
#define MPI_ERR_INFO_KEY              22
#define MPI_ERR_INFO_NOKEY            23
#define MPI_ERR_ROOT                  24
#define MPI_ERR_UNKNOWN               25
#define MPI_ERR_KEYVAL                26
#define MPI_ERR_BASE                  27
#define MPI_ERR_SPAWN                 28
#define MPI_ERR_PORT                  29
#define MPI_ERR_SERVICE               30
#define MPI_ERR_SIZE                  31
#define MPI_ERR_DISP                  32
#define MPI_ERR_INFO                  33
#define MPI_ERR_LOCKTYPE              34
#define MPI_ERR_ASSERT                35
#define MPI_RMA_CONFLICT              36
#define MPI_RMA_SYNC                  37
#define MPI_ERR_FILE                  38
#define MPI_ERR_NOT_SAME              39
#define MPI_ERR_AMODE                 40
#define MPI_ERR_UNSUPPORTED_DATAREP   41
#define MPI_ERR_UNSUPPORTED_OPERATION 42
#define MPI_ERR_NO_SUCH_FILE          43
#define MPI_ERR_FILE_EXISTS           44
#define MPI_ERR_BAD_FILE              45
#define MPI_ERR_ACCESS                46
#define MPI_ERR_NO_SPACE              47
#define MPI_ERR_QUOTA                 48
#define MPI_ERR_READ_ONLY             49
#define MPI_ERR_FILE_IN_USE           50
#define MPI_ERR_DUP_DATAREP           51
#define MPI_ERR_CONVERSION            52
#define MPI_ERR_IO                    53
#define MPI_ERR_RMA_ATTACH            54
#define MPI_ERR_RMA_CONFLICT          55
#define MPI_ERR_RMA_RANGE             56
#define MPI_ERR_RMA_SHARED            57
#define MPI_ERR_RMA_SYNC              58
#define MPI_ERR_RMA_FLAVOR            59
#define MPI_T_ERR_CANNOT_INIT         60
#define MPI_T_ERR_NOT_INITIALIZED     61
#define MPI_T_ERR_MEMORY              62
#define MPI_T_ERR_INVALID_INDEX       63
#define MPI_T_ERR_INVALID_ITEM        64
#define MPI_T_ERR_INVALID_SESSION     65
#define MPI_T_ERR_INVALID_HANDLE      66
#define MPI_T_ERR_OUT_OF_HANDLES      67
#define MPI_T_ERR_OUT_OF_SESSIONS     68
#define MPI_T_ERR_CVAR_SET_NOT_NOW    69
#define MPI_T_ERR_CVAR_SET_NEVER      70
#define MPI_T_ERR_PVAR_NO_WRITE       71
#define MPI_T_ERR_PVAR_NO_STARTSTOP   72
#define MPI_T_ERR_PVAR_NO_ATOMIC      73

#define MPI_ERRCODES_IGNORE (int *)0
#define MPI_IDENT     0
#define MPI_SIMILAR   1
#define MPI_UNEQUAL   2
#define MPI_CONGRUENT 3

#define MPI_BSEND_OVERHEAD   0

/* Attribute keys */
#define MPI_IO               -1
#define MPI_HOST             -2
#define MPI_WTIME_IS_GLOBAL  -3
#define MPI_APPNUM           -4
#define MPI_TAG_UB           -5
#define MPI_TAG_LB           -6
#define MPI_UNIVERSE_SIZE    -7
#define MPI_LASTUSEDCODE     -8

#define MPI_MODE_NOSTORE 0x1
#define MPI_MODE_NOPUT 0x2
#define MPI_MODE_NOPRECEDE 0x4
#define MPI_MODE_NOSUCCEED 0x8
#define MPI_MODE_NOCHECK 0x10

#define MPI_KEYVAL_INVALID 0
#define MPI_NULL_COPY_FN NULL
#define MPI_NULL_DELETE_FN NULL
#define MPI_ERR_LASTCODE 74

#define MPI_CXX_BOOL MPI_DATATYPE_NULL
#define MPI_CXX_FLOAT_COMPLEX MPI_DATATYPE_NULL
#define MPI_CXX_DOUBLE_COMPLEX MPI_DATATYPE_NULL
#define MPI_CXX_LONG_DOUBLE_COMPLEX MPI_DATATYPE_NULL

#define MPI_DISTRIBUTE_BLOCK 0
#define MPI_DISTRIBUTE_NONE 1
#define MPI_DISTRIBUTE_CYCLIC 2
#define MPI_DISTRIBUTE_DFLT_DARG 0
#define MPI_ORDER_C 1
#define MPI_ORDER_FORTRAN 0

#define MPI_TYPECLASS_REAL 0
#define MPI_TYPECLASS_INTEGER 1
#define MPI_TYPECLASS_COMPLEX 2
#define MPI_ROOT 0
#define MPI_INFO_NULL NULL
#define MPI_COMM_TYPE_SHARED    1
#define MPI_WIN_NULL ((MPI_Win)NULL)

#define MPI_VERSION 1
#define MPI_SUBVERSION 1
#define MPI_UNWEIGHTED      (int *)0
#define MPI_ARGV_NULL (char **)0
#define MPI_ARGVS_NULL (char ***)0
#define MPI_LOCK_EXCLUSIVE           1
#define MPI_LOCK_SHARED              2

#define MPI_MODE_RDONLY              2
#define MPI_MODE_RDWR                8
#define MPI_MODE_WRONLY              4
#define MPI_MODE_CREATE              1
#define MPI_MODE_EXCL               64
#define MPI_MODE_DELETE_ON_CLOSE    16
#define MPI_MODE_UNIQUE_OPEN        32
#define MPI_MODE_APPEND            128
#define MPI_MODE_SEQUENTIAL        256
#define MPI_FILE_NULL ((MPI_File) 0)
#define MPI_DISPLACEMENT_CURRENT   -54278278
#define MPI_SEEK_SET            600
#define MPI_SEEK_CUR            602
#define MPI_SEEK_END            604
#define MPI_MAX_DATAREP_STRING  128

// FIXME : used nowhere...
typedef enum MPIR_Combiner_enum{
  MPI_COMBINER_NAMED,
  MPI_COMBINER_DUP,
  MPI_COMBINER_CONTIGUOUS,
  MPI_COMBINER_VECTOR,
  MPI_COMBINER_HVECTOR_INTEGER,
  MPI_COMBINER_HVECTOR,
  MPI_COMBINER_INDEXED,
  MPI_COMBINER_HINDEXED_INTEGER,
  MPI_COMBINER_HINDEXED,
  MPI_COMBINER_INDEXED_BLOCK,
  MPI_COMBINER_STRUCT_INTEGER,
  MPI_COMBINER_STRUCT,
  MPI_COMBINER_SUBARRAY,
  MPI_COMBINER_DARRAY,
  MPI_COMBINER_F90_REAL,
  MPI_COMBINER_F90_COMPLEX,
  MPI_COMBINER_F90_INTEGER,
  MPI_COMBINER_RESIZED,
  MPI_COMBINER_HINDEXED_BLOCK
}MPIR_Combiner_enum;

typedef enum MPIR_Topo_type {
  MPI_GRAPH=1,
  MPI_CART=2,
  MPI_DIST_GRAPH=3,
  MPI_INVALID_TOPO=-1
} MPIR_Topo_type;

typedef ptrdiff_t MPI_Aint;
typedef long long MPI_Offset;

struct s_MPI_File;
typedef struct s_MPI_File *MPI_File;

struct s_smpi_mpi_datatype;
typedef struct s_smpi_mpi_datatype *MPI_Datatype;

typedef struct {
  int MPI_SOURCE;
  int MPI_TAG;
  int MPI_ERROR;
  int count;
} MPI_Status;

struct s_smpi_mpi_win;
typedef struct s_smpi_mpi_win* MPI_Win;
struct s_smpi_mpi_info;
typedef struct s_smpi_mpi_info *MPI_Info;

#define MPI_STATUS_IGNORE ((MPI_Status*)NULL)
#define MPI_STATUSES_IGNORE ((MPI_Status*)NULL)

#define MPI_DATATYPE_NULL ((const MPI_Datatype)NULL)
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_CHAR;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_SHORT;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_INT;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_LONG;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_LONG_LONG;
#define MPI_LONG_LONG_INT MPI_LONG_LONG
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_SIGNED_CHAR;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_UNSIGNED_CHAR;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_UNSIGNED_SHORT;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_UNSIGNED;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_UNSIGNED_LONG;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_UNSIGNED_LONG_LONG;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_FLOAT;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_DOUBLE;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_LONG_DOUBLE;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_WCHAR;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_C_BOOL;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_INT8_T;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_INT16_T;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_INT32_T;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_INT64_T;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_UINT8_T;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_BYTE;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_UINT16_T;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_UINT32_T;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_UINT64_T;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_C_FLOAT_COMPLEX;
#define MPI_C_COMPLEX MPI_C_FLOAT_COMPLEX
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_C_DOUBLE_COMPLEX;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_C_LONG_DOUBLE_COMPLEX;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_AINT;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_OFFSET;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_LB;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_UB;
//The following are datatypes for the MPI functions MPI_MAXLOC  and MPI_MINLOC.
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_FLOAT_INT;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_LONG_INT;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_DOUBLE_INT;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_SHORT_INT;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_2INT;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_LONG_DOUBLE_INT;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_2FLOAT;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_2DOUBLE;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_2LONG;//only for compatibility with Fortran

XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_REAL;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_REAL4;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_REAL8;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_REAL16;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_COMPLEX8;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_COMPLEX16;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_COMPLEX32;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_INTEGER1;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_INTEGER2;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_INTEGER4;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_INTEGER8;
XBT_PUBLIC_DATA( const MPI_Datatype ) MPI_INTEGER16;

//for now we only send int values at max
#define MPI_Count int
#define MPI_COUNT MPI_INT

//defines for fortran compatibility
#if defined(__alpha__) || defined(__sparc64__) || defined(__x86_64__) || defined(__ia64__)
  #define MPI_INTEGER MPI_INT
  #define MPI_2INTEGER MPI_2INT
  #define MPI_LOGICAL MPI_INT
#else
  #define MPI_INTEGER MPI_LONG
  #define MPI_2INTEGER MPI_2LONG
  #define MPI_LOGICAL MPI_LONG
#endif

#define MPI_Fint int

#define MPI_COMPLEX MPI_C_FLOAT_COMPLEX
#define MPI_DOUBLE_COMPLEX MPI_C_DOUBLE_COMPLEX
#define MPI_LOGICAL1 MPI_UINT8_T
#define MPI_LOGICAL2 MPI_UINT16_T
#define MPI_LOGICAL4 MPI_UINT32_T
#define MPI_LOGICAL8 MPI_UINT64_T
#define MPI_2REAL MPI_2FLOAT
#define MPI_CHARACTER MPI_CHAR
#define MPI_DOUBLE_PRECISION MPI_DOUBLE
#define MPI_2DOUBLE_PRECISION MPI_2DOUBLE

typedef void MPI_User_function(void *invec, void *inoutvec, int *len, MPI_Datatype * datatype);
struct s_smpi_mpi_op;
typedef struct s_smpi_mpi_op *MPI_Op;

#define MPI_OP_NULL ((MPI_Op)NULL)
XBT_PUBLIC_DATA( MPI_Op ) MPI_MAX;
XBT_PUBLIC_DATA( MPI_Op ) MPI_MIN;
XBT_PUBLIC_DATA( MPI_Op ) MPI_MAXLOC;
XBT_PUBLIC_DATA( MPI_Op ) MPI_MINLOC;
XBT_PUBLIC_DATA( MPI_Op ) MPI_SUM;
XBT_PUBLIC_DATA( MPI_Op ) MPI_PROD;
XBT_PUBLIC_DATA( MPI_Op ) MPI_LAND;
XBT_PUBLIC_DATA( MPI_Op ) MPI_LOR;
XBT_PUBLIC_DATA( MPI_Op ) MPI_LXOR;
XBT_PUBLIC_DATA( MPI_Op ) MPI_BAND;
XBT_PUBLIC_DATA( MPI_Op ) MPI_BOR;
XBT_PUBLIC_DATA( MPI_Op ) MPI_BXOR;
//For accumulate
XBT_PUBLIC_DATA( MPI_Op ) MPI_REPLACE;

struct s_smpi_mpi_topology;
typedef struct s_smpi_mpi_topology *MPI_Topology;
                                   
struct s_smpi_mpi_group;
typedef struct s_smpi_mpi_group *MPI_Group;

#define MPI_GROUP_NULL ((MPI_Group)NULL)

XBT_PUBLIC_DATA( MPI_Group ) MPI_GROUP_EMPTY;

struct s_smpi_mpi_communicator;
typedef struct s_smpi_mpi_communicator *MPI_Comm;

#define MPI_COMM_NULL ((MPI_Comm)NULL)
XBT_PUBLIC_DATA( MPI_Comm ) MPI_COMM_WORLD;
#define MPI_COMM_SELF smpi_process_comm_self()

struct s_smpi_mpi_request;
typedef struct s_smpi_mpi_request *MPI_Request;

#define MPI_REQUEST_NULL ((MPI_Request)NULL)
#define MPI_FORTRAN_REQUEST_NULL -1

MPI_CALL(XBT_PUBLIC(int), MPI_Init, (int *argc, char ***argv));
MPI_CALL(XBT_PUBLIC(int), MPI_Finalize, (void));
MPI_CALL(XBT_PUBLIC(int), MPI_Finalized, (int* flag));
MPI_CALL(XBT_PUBLIC(int), MPI_Init_thread, (int *argc, char ***argv, int required, int *provided));
MPI_CALL(XBT_PUBLIC(int), MPI_Query_thread, (int *provided));
MPI_CALL(XBT_PUBLIC(int), MPI_Is_thread_main, (int *flag));
MPI_CALL(XBT_PUBLIC(int), MPI_Abort, (MPI_Comm comm, int errorcode));
MPI_CALL(XBT_PUBLIC(double), MPI_Wtime, (void));
MPI_CALL(XBT_PUBLIC(double), MPI_Wtick,(void));
MPI_CALL(XBT_PUBLIC(int), MPI_Address, (void *location, MPI_Aint * address));
MPI_CALL(XBT_PUBLIC(int), MPI_Get_address, (void *location, MPI_Aint * address));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_free, (MPI_Datatype * datatype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_size, (MPI_Datatype datatype, int *size));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_get_extent, (MPI_Datatype datatype, MPI_Aint * lb, MPI_Aint * extent));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_get_true_extent, (MPI_Datatype datatype, MPI_Aint * lb, MPI_Aint * extent));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_extent, (MPI_Datatype datatype, MPI_Aint * extent));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_lb, (MPI_Datatype datatype, MPI_Aint * disp));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_ub, (MPI_Datatype datatype, MPI_Aint * disp));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_commit, (MPI_Datatype* datatype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_hindexed,(int count, int* blocklens, MPI_Aint* indices,
                            MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_create_hindexed, (int count, int* blocklens, MPI_Aint* indices,
                            MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_create_hindexed_block, (int count, int blocklength, MPI_Aint* indices,
                            MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_hvector, (int count, int blocklen, MPI_Aint stride,
                             MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_create_hvector, (int count, int blocklen, MPI_Aint stride,
                             MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_indexed, (int count, int* blocklens, int* indices,
                             MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_create_indexed, (int count, int* blocklens, int* indices,
                             MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_create_indexed_block, (int count, int blocklength, int* indices,
                             MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_struct, (int count, int* blocklens, MPI_Aint* indices,
                             MPI_Datatype* old_types, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_create_struct, (int count, int* blocklens, MPI_Aint* indices,
                             MPI_Datatype* old_types, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_vector, (int count, int blocklen, int stride,
                             MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_contiguous, (int count, MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC(int), MPI_Testall, (int count, MPI_Request* requests, int* flag, MPI_Status* statuses));
MPI_CALL(XBT_PUBLIC(int), MPI_Op_create, (MPI_User_function * function, int commute, MPI_Op * op));
MPI_CALL(XBT_PUBLIC(int), MPI_Op_free, (MPI_Op * op));

MPI_CALL(XBT_PUBLIC(int), MPI_Group_free, (MPI_Group * group));
MPI_CALL(XBT_PUBLIC(int), MPI_Group_size, (MPI_Group group, int *size));
MPI_CALL(XBT_PUBLIC(int), MPI_Group_rank, (MPI_Group group, int *rank));
MPI_CALL(XBT_PUBLIC(int), MPI_Group_translate_ranks, (MPI_Group group1, int n, int *ranks1, MPI_Group group2,
                             int *ranks2));
MPI_CALL(XBT_PUBLIC(int), MPI_Group_compare, (MPI_Group group1, MPI_Group group2, int *result));
MPI_CALL(XBT_PUBLIC(int), MPI_Group_union, (MPI_Group group1, MPI_Group group2, MPI_Group * newgroup));
MPI_CALL(XBT_PUBLIC(int), MPI_Group_intersection, (MPI_Group group1, MPI_Group group2, MPI_Group * newgroup));
MPI_CALL(XBT_PUBLIC(int), MPI_Group_difference, (MPI_Group group1, MPI_Group group2, MPI_Group * newgroup));
MPI_CALL(XBT_PUBLIC(int), MPI_Group_incl, (MPI_Group group, int n, int *ranks, MPI_Group * newgroup));
MPI_CALL(XBT_PUBLIC(int), MPI_Group_excl, (MPI_Group group, int n, int *ranks, MPI_Group * newgroup));
MPI_CALL(XBT_PUBLIC(int), MPI_Group_range_incl, (MPI_Group group, int n, int ranges[][3], MPI_Group * newgroup));
MPI_CALL(XBT_PUBLIC(int), MPI_Group_range_excl, (MPI_Group group, int n, int ranges[][3], MPI_Group * newgroup));

MPI_CALL(XBT_PUBLIC(int), MPI_Comm_rank, (MPI_Comm comm, int *rank));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_size, (MPI_Comm comm, int *size));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_get_name, (MPI_Comm comm, char* name, int* len));
MPI_CALL(XBT_PUBLIC(int), MPI_Get_processor_name, (char *name, int *resultlen));
MPI_CALL(XBT_PUBLIC(int), MPI_Get_count, (MPI_Status * status, MPI_Datatype datatype, int *count));

MPI_CALL(XBT_PUBLIC(int), MPI_Comm_group, (MPI_Comm comm, MPI_Group * group));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_compare, (MPI_Comm comm1, MPI_Comm comm2, int *result));

MPI_CALL(XBT_PUBLIC(int), MPI_Comm_create, (MPI_Comm comm, MPI_Group group, MPI_Comm * newcomm));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_free, (MPI_Comm * comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_disconnect, (MPI_Comm * comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_split, (MPI_Comm comm, int color, int key, MPI_Comm* comm_out));

MPI_CALL(XBT_PUBLIC(int), MPI_Send_init, (void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm,
                             MPI_Request * request));
MPI_CALL(XBT_PUBLIC(int), MPI_Recv_init, (void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm,
                             MPI_Request * request));
MPI_CALL(XBT_PUBLIC(int), MPI_Start, (MPI_Request * request));
MPI_CALL(XBT_PUBLIC(int), MPI_Startall, (int count, MPI_Request * requests));
MPI_CALL(XBT_PUBLIC(int), MPI_Request_free, (MPI_Request * request));
MPI_CALL(XBT_PUBLIC(int), MPI_Irecv, (void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm,
                             MPI_Request * request));
MPI_CALL(XBT_PUBLIC(int), MPI_Isend, (void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm,
                             MPI_Request * request));
MPI_CALL(XBT_PUBLIC(int), MPI_Recv, (void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm,
                             MPI_Status * status));
MPI_CALL(XBT_PUBLIC(int), MPI_Send, (void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Sendrecv, (void *sendbuf, int sendcount, MPI_Datatype sendtype, int dst, int sendtag,
                             void *recvbuf, int recvcount, MPI_Datatype recvtype, int src, int recvtag,
                             MPI_Comm comm, MPI_Status * status));
MPI_CALL(XBT_PUBLIC(int), MPI_Sendrecv_replace, (void *buf, int count, MPI_Datatype datatype, int dst,
                             int sendtag, int src, int recvtag, MPI_Comm comm, MPI_Status * status));

MPI_CALL(XBT_PUBLIC(int), MPI_Test, (MPI_Request * request, int *flag, MPI_Status* status));
MPI_CALL(XBT_PUBLIC(int), MPI_Testany, (int count, MPI_Request requests[], int *index, int *flag, MPI_Status * status));
MPI_CALL(XBT_PUBLIC(int), MPI_Wait, (MPI_Request * request, MPI_Status * status));
MPI_CALL(XBT_PUBLIC(int), MPI_Waitany, (int count, MPI_Request requests[], int *index, MPI_Status * status));
MPI_CALL(XBT_PUBLIC(int), MPI_Waitall, (int count, MPI_Request requests[], MPI_Status status[]));
MPI_CALL(XBT_PUBLIC(int), MPI_Waitsome, (int incount, MPI_Request requests[], int *outcount, int *indices,
                             MPI_Status status[]));
MPI_CALL(XBT_PUBLIC(int), MPI_Testsome, (int incount, MPI_Request requests[], int *outcount, int *indices,
                             MPI_Status status[]));
MPI_CALL(XBT_PUBLIC(int), MPI_Bcast, (void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Barrier, (MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Gather, (void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                             int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Gatherv, (void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                             int *recvcounts, int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Allgather, (void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                             int recvcount, MPI_Datatype recvtype, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Allgatherv, (void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                             int *recvcounts, int *displs, MPI_Datatype recvtype, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Scatter, (void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                             int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Scatterv, (void *sendbuf, int *sendcounts, int *displs, MPI_Datatype sendtype,
                             void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Reduce, (void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                             int root, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Allreduce, (void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                             MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Scan, (void *sendbuf, void *recvbuf, int count,MPI_Datatype datatype, MPI_Op op,
                             MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Reduce_scatter, (void *sendbuf, void *recvbuf, int *recvcounts, MPI_Datatype datatype,
                             MPI_Op op, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Reduce_scatter_block, (void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype,
                             MPI_Op op, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Alltoall, (void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                             int recvcount, MPI_Datatype recvtype, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Alltoallv, (void *sendbuf, int *sendcounts, int *senddisps, MPI_Datatype sendtype,
                             void *recvbuf, int *recvcounts, int *recvdisps, MPI_Datatype recvtype, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Iprobe, (int source, int tag, MPI_Comm comm, int* flag, MPI_Status* status));
MPI_CALL(XBT_PUBLIC(int), MPI_Probe, (int source, int tag, MPI_Comm comm, MPI_Status* status));
MPI_CALL(XBT_PUBLIC(int), MPI_Get_version, (int *version,int *subversion));
MPI_CALL(XBT_PUBLIC(int), MPI_Get_library_version, (char *version,int *len));
MPI_CALL(XBT_PUBLIC(int), MPI_Reduce_local,(void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype, MPI_Op op));

MPI_CALL(XBT_PUBLIC(int), MPI_Win_free,( MPI_Win* win));
MPI_CALL(XBT_PUBLIC(int), MPI_Win_create,( void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm,
                              MPI_Win *win));
MPI_CALL(XBT_PUBLIC(int), MPI_Win_set_name,(MPI_Win  win, char * name));
MPI_CALL(XBT_PUBLIC(int), MPI_Win_get_name,(MPI_Win  win, char * name, int* len));
MPI_CALL(XBT_PUBLIC(int), MPI_Win_get_group,(MPI_Win  win, MPI_Group * group));
MPI_CALL(XBT_PUBLIC(int), MPI_Win_fence,( int assert,  MPI_Win win));

MPI_CALL(XBT_PUBLIC(int), MPI_Get,( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
    MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win));
MPI_CALL(XBT_PUBLIC(int), MPI_Put,( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
    MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win));
MPI_CALL(XBT_PUBLIC(int), MPI_Accumulate,( void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
    int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win));
MPI_CALL(XBT_PUBLIC(int), MPI_Alloc_mem, (MPI_Aint size, MPI_Info info, void *baseptr));
MPI_CALL(XBT_PUBLIC(int), MPI_Free_mem, (void *base));

MPI_CALL(XBT_PUBLIC(MPI_Datatype), MPI_Type_f2c,(MPI_Fint datatype));
MPI_CALL(XBT_PUBLIC(MPI_Fint), MPI_Type_c2f,(MPI_Datatype datatype));
MPI_CALL(XBT_PUBLIC(MPI_Group), MPI_Group_f2c,(MPI_Fint group));
MPI_CALL(XBT_PUBLIC(MPI_Fint), MPI_Group_c2f,(MPI_Group group));
MPI_CALL(XBT_PUBLIC(MPI_Request), MPI_Request_f2c,(MPI_Fint request));
MPI_CALL(XBT_PUBLIC(MPI_Fint), MPI_Request_c2f,(MPI_Request request));
MPI_CALL(XBT_PUBLIC(MPI_Win), MPI_Win_f2c,(MPI_Fint win));
MPI_CALL(XBT_PUBLIC(MPI_Fint), MPI_Win_c2f,(MPI_Win win));
MPI_CALL(XBT_PUBLIC(MPI_Op), MPI_Op_f2c,(MPI_Fint op));
MPI_CALL(XBT_PUBLIC(MPI_Fint), MPI_Op_c2f,(MPI_Op op));
MPI_CALL(XBT_PUBLIC(MPI_Comm), MPI_Comm_f2c,(MPI_Fint comm));
MPI_CALL(XBT_PUBLIC(MPI_Fint), MPI_Comm_c2f,(MPI_Comm comm));

//FIXME: these are not yet implemented

typedef void MPI_Handler_function(MPI_Comm*, int*, ...);

typedef void* MPI_Errhandler;

typedef int MPI_Copy_function(MPI_Comm oldcomm, int keyval, void* extra_state, void* attribute_val_in,
                              void* attribute_val_out, int* flag);
typedef int MPI_Delete_function(MPI_Comm comm, int keyval, void* attribute_val, void* extra_state);
#define MPI_Comm_copy_attr_function MPI_Copy_function
#define MPI_Comm_delete_attr_function MPI_Delete_function
typedef int MPI_Type_copy_attr_function(MPI_Datatype type, int keyval, void* extra_state, void* attribute_val_in,
                              void* attribute_val_out, int* flag);
typedef int MPI_Type_delete_attr_function(MPI_Datatype type, int keyval, void* attribute_val, void* extra_state);
typedef void MPI_Comm_errhandler_function(MPI_Comm *, int *, ...);
typedef int MPI_Grequest_query_function(void *extra_state, MPI_Status *status); 
typedef int MPI_Grequest_free_function(void *extra_state); 
typedef int MPI_Grequest_cancel_function(void *extra_state, int complete); 
#define MPI_DUP_FN MPI_Comm_dup
#define MPI_COMM_NULL_COPY_FN ((MPI_Comm_copy_attr_function*)0)
#define MPI_COMM_NULL_DELETE_FN ((MPI_Comm_delete_attr_function*)0)
#define MPI_COMM_DUP_FN  ((MPI_Comm_copy_attr_function *)MPI_DUP_FN)
#define MPI_TYPE_NULL_COPY_FN ((MPI_Type_copy_attr_function*)0)
#define MPI_TYPE_NULL_DELETE_FN ((MPI_Type_delete_attr_function*)0)
#define MPI_TYPE_DUP_FN ((MPI_Type_copy_attr_function*)MPI_DUP_FN)

typedef MPI_Comm_errhandler_function MPI_Comm_errhandler_fn;
#define MPI_INFO_ENV 1
XBT_PUBLIC_DATA( const MPI_Datatype )  MPI_PACKED;
XBT_PUBLIC_DATA(MPI_Errhandler*)  MPI_ERRORS_RETURN;
XBT_PUBLIC_DATA(MPI_Errhandler*)  MPI_ERRORS_ARE_FATAL;
XBT_PUBLIC_DATA(MPI_Errhandler*)  MPI_ERRHANDLER_NULL;

MPI_CALL(XBT_PUBLIC(MPI_Info), MPI_Info_f2c,(MPI_Fint info));
MPI_CALL(XBT_PUBLIC(MPI_Fint), MPI_Info_c2f,(MPI_Info info));
MPI_CALL(XBT_PUBLIC(MPI_Errhandler), MPI_Errhandler_f2c,(MPI_Fint errhandler));
MPI_CALL(XBT_PUBLIC(MPI_Fint), MPI_Errhandler_c2f,(MPI_Errhandler errhandler));
MPI_CALL(XBT_PUBLIC(int), MPI_Pack_size, (int incount, MPI_Datatype datatype, MPI_Comm comm, int* size));
MPI_CALL(XBT_PUBLIC(int), MPI_Cart_coords, (MPI_Comm comm, int rank, int maxdims, int* coords));
MPI_CALL(XBT_PUBLIC(int), MPI_Cart_create, (MPI_Comm comm_old, int ndims, int* dims, int* periods, int reorder,
                                            MPI_Comm* comm_cart));
MPI_CALL(XBT_PUBLIC(int), MPI_Cart_get, (MPI_Comm comm, int maxdims, int* dims, int* periods, int* coords));
MPI_CALL(XBT_PUBLIC(int), MPI_Cart_map, (MPI_Comm comm_old, int ndims, int* dims, int* periods, int* newrank));
MPI_CALL(XBT_PUBLIC(int), MPI_Cart_rank, (MPI_Comm comm, int* coords, int* rank));
MPI_CALL(XBT_PUBLIC(int), MPI_Cart_shift, (MPI_Comm comm, int direction, int displ, int* source, int* dest));
MPI_CALL(XBT_PUBLIC(int), MPI_Cart_sub, (MPI_Comm comm, int* remain_dims, MPI_Comm* comm_new));
MPI_CALL(XBT_PUBLIC(int), MPI_Cartdim_get, (MPI_Comm comm, int* ndims));
MPI_CALL(XBT_PUBLIC(int), MPI_Graph_create, (MPI_Comm comm_old, int nnodes, int* index, int* edges, int reorder,
                                             MPI_Comm* comm_graph));
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
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_set_errhandler, (MPI_Comm comm, MPI_Errhandler errhandler));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_get_errhandler, (MPI_Comm comm, MPI_Errhandler *errhandler));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_create_errhandler,( MPI_Comm_errhandler_fn *function, MPI_Errhandler *errhandler));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_call_errhandler,(MPI_Comm comm,int errorcode));
MPI_CALL(XBT_PUBLIC(int), MPI_Add_error_class,( int *errorclass));
MPI_CALL(XBT_PUBLIC(int), MPI_Add_error_code,(  int errorclass, int *errorcode));
MPI_CALL(XBT_PUBLIC(int), MPI_Add_error_string,( int errorcode, char *string));
MPI_CALL(XBT_PUBLIC(int), MPI_Cancel, (MPI_Request* request));
MPI_CALL(XBT_PUBLIC(int), MPI_Buffer_attach, (void* buffer, int size));
MPI_CALL(XBT_PUBLIC(int), MPI_Buffer_detach, (void* buffer, int* size));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_test_inter, (MPI_Comm comm, int* flag));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_get_attr, (MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_set_attr, (MPI_Comm comm, int comm_keyval, void *attribute_val));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_delete_attr, (MPI_Comm comm, int comm_keyval));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_create_keyval,(MPI_Comm_copy_attr_function* copy_fn,
                              MPI_Comm_delete_attr_function* delete_fn, int* keyval, void* extra_state));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_free_keyval,(int* keyval));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_get_attr, (MPI_Datatype type, int type_keyval, void *attribute_val, int* flag));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_set_attr, (MPI_Datatype type, int type_keyval, void *att));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_delete_attr, (MPI_Datatype type, int comm_keyval));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_create_keyval,(MPI_Type_copy_attr_function* copy_fn,
                              MPI_Type_delete_attr_function* delete_fn, int* keyval, void* extra_state));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_free_keyval,(int* keyval));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_dup,(MPI_Datatype datatype,MPI_Datatype *newtype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_set_name,(MPI_Datatype  datatype, char * name));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_get_name,(MPI_Datatype  datatype, char * name, int* len));
MPI_CALL(XBT_PUBLIC(int), MPI_Unpack, (void* inbuf, int insize, int* position, void* outbuf, int outcount,
                              MPI_Datatype type, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Ssend, (void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Ssend_init, (void* buf, int count, MPI_Datatype datatype, int dest, int tag,
                              MPI_Comm comm, MPI_Request* request));
MPI_CALL(XBT_PUBLIC(int), MPI_Intercomm_create, (MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm,
                              int remote_leader, int tag, MPI_Comm* comm_out));
MPI_CALL(XBT_PUBLIC(int), MPI_Intercomm_merge, (MPI_Comm comm, int high, MPI_Comm* comm_out));
MPI_CALL(XBT_PUBLIC(int), MPI_Bsend, (void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Bsend_init, (void* buf, int count, MPI_Datatype datatype, int dest, int tag,
                              MPI_Comm comm, MPI_Request* request));
MPI_CALL(XBT_PUBLIC(int), MPI_Ibsend, (void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                              MPI_Request* request));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_remote_group, (MPI_Comm comm, MPI_Group* group));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_remote_size, (MPI_Comm comm, int* size));
MPI_CALL(XBT_PUBLIC(int), MPI_Issend, (void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                              MPI_Request* request));
MPI_CALL(XBT_PUBLIC(int), MPI_Attr_delete, (MPI_Comm comm, int keyval));
MPI_CALL(XBT_PUBLIC(int), MPI_Attr_get, (MPI_Comm comm, int keyval, void* attr_value, int* flag));
MPI_CALL(XBT_PUBLIC(int), MPI_Attr_put, (MPI_Comm comm, int keyval, void* attr_value));
MPI_CALL(XBT_PUBLIC(int), MPI_Rsend, (void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Rsend_init, (void* buf, int count, MPI_Datatype datatype, int dest, int tag,
                              MPI_Comm comm, MPI_Request* request));
MPI_CALL(XBT_PUBLIC(int), MPI_Irsend, (void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                              MPI_Request* request));
MPI_CALL(XBT_PUBLIC(int), MPI_Keyval_create, (MPI_Copy_function* copy_fn, MPI_Delete_function* delete_fn, int* keyval,
                              void* extra_state));
MPI_CALL(XBT_PUBLIC(int), MPI_Keyval_free, (int* keyval));
MPI_CALL(XBT_PUBLIC(int), MPI_Test_cancelled, (MPI_Status* status, int* flag));
MPI_CALL(XBT_PUBLIC(int), MPI_Pack, (void* inbuf, int incount, MPI_Datatype type, void* outbuf, int outcount,
                              int* position, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Get_elements, (MPI_Status* status, MPI_Datatype datatype, int* elements));
MPI_CALL(XBT_PUBLIC(int), MPI_Dims_create, (int nnodes, int ndims, int* dims));
MPI_CALL(XBT_PUBLIC(int), MPI_Initialized, (int* flag));
MPI_CALL(XBT_PUBLIC(int), MPI_Pcontrol, (const int level ));

MPI_CALL(XBT_PUBLIC(int), MPI_Info_create,( MPI_Info *info));
MPI_CALL(XBT_PUBLIC(int), MPI_Info_set,( MPI_Info info, char *key, char *value));
MPI_CALL(XBT_PUBLIC(int), MPI_Info_get,(MPI_Info info,char *key,int valuelen, char *value, int *flag));
MPI_CALL(XBT_PUBLIC(int), MPI_Info_free,( MPI_Info *info));
MPI_CALL(XBT_PUBLIC(int), MPI_Info_delete,( MPI_Info info,  char *key));
MPI_CALL(XBT_PUBLIC(int), MPI_Info_dup,(MPI_Info info, MPI_Info *newinfo));
MPI_CALL(XBT_PUBLIC(int), MPI_Info_get_nkeys,( MPI_Info info, int *nkeys));
MPI_CALL(XBT_PUBLIC(int), MPI_Info_get_nthkey,( MPI_Info info, int n, char *key));
MPI_CALL(XBT_PUBLIC(int), MPI_Info_get_valuelen,( MPI_Info info, char *key, int *valuelen, int *flag));


MPI_CALL(XBT_PUBLIC(int), MPI_Win_set_errhandler, (MPI_Win win, MPI_Errhandler errhandler));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_get_envelope,(MPI_Datatype datatype,int *num_integers,int *num_addresses,
                            int *num_datatypes, int *combiner));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_get_contents,(MPI_Datatype datatype, int max_integers, int max_addresses,
                            int max_datatypes, int* array_of_integers, MPI_Aint* array_of_addresses, 
                            MPI_Datatype *array_of_datatypes));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_create_darray,(int size, int rank, int ndims, int* array_of_gsizes,
                            int* array_of_distribs, int* array_of_dargs, int* array_of_psizes,
                            int order, MPI_Datatype oldtype, MPI_Datatype *newtype));
MPI_CALL(XBT_PUBLIC(int), MPI_Pack_external_size, (char *datarep, int incount, MPI_Datatype datatype, MPI_Aint *size));
MPI_CALL(XBT_PUBLIC(int), MPI_Pack_external, (char *datarep, void *inbuf, int incount, MPI_Datatype datatype,
                                              void *outbuf, MPI_Aint outcount, MPI_Aint *position));
MPI_CALL(XBT_PUBLIC(int), MPI_Unpack_external, (char *datarep, void *inbuf, MPI_Aint insize, MPI_Aint *position,
                                                void *outbuf, int outcount, MPI_Datatype datatype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_create_resized ,(MPI_Datatype oldtype,MPI_Aint lb, MPI_Aint extent,
                                                    MPI_Datatype *newtype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_create_subarray,(int ndims,int *array_of_sizes, int *array_of_subsizes,
                              int *array_of_starts, int order, MPI_Datatype oldtype, MPI_Datatype *newtype));
MPI_CALL(XBT_PUBLIC(int), MPI_Type_match_size,(int typeclass,int size,MPI_Datatype *datatype));
MPI_CALL(XBT_PUBLIC(int), MPI_Alltoallw, ( void *sendbuf, int *sendcnts, int *sdispls, MPI_Datatype *sendtypes,
                              void *recvbuf, int *recvcnts, int *rdispls, MPI_Datatype *recvtypes, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Exscan,(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                                      MPI_Comm comm));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_set_name, (MPI_Comm comm, char* name));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_set_info, (MPI_Comm comm, MPI_Info info));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_get_info, (MPI_Comm comm, MPI_Info* info));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_dup, (MPI_Comm comm, MPI_Comm * newcomm));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_dup_with_info,(MPI_Comm comm, MPI_Info info, MPI_Comm * newcomm));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_split_type,(MPI_Comm comm, int split_type, int key, MPI_Info info,
                                               MPI_Comm *newcomm));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_connect,(char *port_name, MPI_Info info, int root, MPI_Comm comm,
                                            MPI_Comm *newcomm));
MPI_CALL(XBT_PUBLIC(int), MPI_Request_get_status,( MPI_Request request, int *flag, MPI_Status *status));
MPI_CALL(XBT_PUBLIC(int), MPI_Grequest_start,(MPI_Grequest_query_function *query_fn,
                                              MPI_Grequest_free_function *free_fn,
                                              MPI_Grequest_cancel_function *cancel_fn,
                                              void *extra_state, MPI_Request *request));
MPI_CALL(XBT_PUBLIC(int), MPI_Grequest_complete,( MPI_Request request));
MPI_CALL(XBT_PUBLIC(int), MPI_Status_set_cancelled,(MPI_Status *status,int flag));
MPI_CALL(XBT_PUBLIC(int), MPI_Status_set_elements,( MPI_Status *status, MPI_Datatype datatype, int count));
MPI_CALL(XBT_PUBLIC(int), MPI_Unpublish_name,( char *service_name, MPI_Info info, char *port_name));
MPI_CALL(XBT_PUBLIC(int), MPI_Publish_name,( char *service_name, MPI_Info info, char *port_name));
MPI_CALL(XBT_PUBLIC(int), MPI_Lookup_name,( char *service_name, MPI_Info info, char *port_name));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_join,( int fd, MPI_Comm *intercomm));
MPI_CALL(XBT_PUBLIC(int), MPI_Open_port,( MPI_Info info, char *port_name));
MPI_CALL(XBT_PUBLIC(int), MPI_Close_port,( char *port_name));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_accept,(char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_spawn,(char *command, char **argv, int maxprocs, MPI_Info info, int root,
                                          MPI_Comm comm, MPI_Comm *intercomm, int* array_of_errcodes));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_spawn_multiple,(int count, char **array_of_commands, char*** array_of_argv,
                                                   int* array_of_maxprocs, MPI_Info* array_of_info, int root,
                                                   MPI_Comm comm, MPI_Comm *intercomm, int* array_of_errcodes));
MPI_CALL(XBT_PUBLIC(int), MPI_Comm_get_parent,( MPI_Comm *parent));
MPI_CALL(XBT_PUBLIC(int),  MPI_Win_complete,(MPI_Win win));
MPI_CALL(XBT_PUBLIC(int),  MPI_Win_lock,(int lock_type, int rank, int assert, MPI_Win win));
MPI_CALL(XBT_PUBLIC(int),  MPI_Win_post,(MPI_Group group, int assert, MPI_Win win));
MPI_CALL(XBT_PUBLIC(int),  MPI_Win_start,(MPI_Group group, int assert, MPI_Win win));
MPI_CALL(XBT_PUBLIC(int),  MPI_Win_test,(MPI_Win win, int *flag));
MPI_CALL(XBT_PUBLIC(int),  MPI_Win_unlock,(int rank, MPI_Win win));
MPI_CALL(XBT_PUBLIC(int),  MPI_Win_wait,(MPI_Win win));


MPI_CALL(XBT_PUBLIC(int),  MPI_File_get_errhandler , (MPI_File file, MPI_Errhandler *errhandler));
MPI_CALL(XBT_PUBLIC(int),  MPI_File_set_errhandler, (MPI_File file, MPI_Errhandler errhandler));

MPI_CALL(XBT_PUBLIC(int),  MPI_File_open,(MPI_Comm comm, const char *filename, int amode, MPI_Info info, MPI_File *fh));
MPI_CALL(XBT_PUBLIC(int),  MPI_File_close,(MPI_File *fh));
MPI_CALL(XBT_PUBLIC(int),  MPI_File_delete,(const char *filename, MPI_Info info));
MPI_CALL(XBT_PUBLIC(int),  MPI_File_set_size,(MPI_File fh, MPI_Offset size));
MPI_CALL(XBT_PUBLIC(int),  MPI_File_get_size,(MPI_File fh, MPI_Offset *size));

MPI_CALL(XBT_PUBLIC(int),  MPI_File_set_view,(MPI_File fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype,
                      const char *datarep, MPI_Info info));
MPI_CALL(XBT_PUBLIC(int),  MPI_File_get_view,(MPI_File fh, MPI_Offset *disp, MPI_Datatype *etype, MPI_Datatype *filetype,
                      char *datarep));

MPI_CALL(XBT_PUBLIC(int),  MPI_File_read_at,(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype,
                     MPI_Status *status));
MPI_CALL(XBT_PUBLIC(int),  MPI_File_read_at_all,(MPI_File fh, MPI_Offset offset, void * buf, int count,
                         MPI_Datatype datatype, MPI_Status *status));
MPI_CALL(XBT_PUBLIC(int),  MPI_File_write_at,(MPI_File fh, MPI_Offset offset, const void * buf, int count,
                      MPI_Datatype datatype, MPI_Status *status));
MPI_CALL(XBT_PUBLIC(int),  MPI_File_write_at_all,(MPI_File fh, MPI_Offset offset, const void *buf, int count,
                          MPI_Datatype datatype, MPI_Status *status));
MPI_CALL(XBT_PUBLIC(int),  MPI_File_set_atomicity,(MPI_File fh, int flag));
MPI_CALL(XBT_PUBLIC(int),  MPI_File_get_atomicity,(MPI_File fh, int *flag));
MPI_CALL(XBT_PUBLIC(int),  MPI_File_sync,(MPI_File fh));

MPI_CALL(XBT_PUBLIC(int), MPI_File_read_at_all_begin,(MPI_File fh, MPI_Offset offset, void *buf, int count,
                               MPI_Datatype datatype));
MPI_CALL(XBT_PUBLIC(int), MPI_File_read_at_all_end,(MPI_File fh, void *buf, MPI_Status *status));
MPI_CALL(XBT_PUBLIC(int), MPI_File_write_at_all_begin,(MPI_File fh, MPI_Offset offset, const void *buf, int count,MPI_Datatype datatype));
MPI_CALL(XBT_PUBLIC(int),  MPI_File_write_at_all_end,(MPI_File fh, const void *buf, MPI_Status *status));
MPI_CALL(XBT_PUBLIC(int),  MPI_File_read_all_begin,(MPI_File fh, void *buf, int count, MPI_Datatype datatype));
MPI_CALL(XBT_PUBLIC(int),  MPI_File_read_all_end,(MPI_File fh, void *buf, MPI_Status *status));
MPI_CALL(XBT_PUBLIC(int),  MPI_File_write_all_begin,(MPI_File fh, const void *buf, int count, MPI_Datatype datatype));
MPI_CALL(XBT_PUBLIC(int),  MPI_File_write_all_end,(MPI_File fh, const void *buf, MPI_Status *status));
MPI_CALL(XBT_PUBLIC(int),  MPI_File_read_ordered_begin,(MPI_File fh, void *buf, int count, MPI_Datatype datatype));
MPI_CALL(XBT_PUBLIC(int),  MPI_File_read_ordered_end,(MPI_File fh, void *buf, MPI_Status *status));
MPI_CALL(XBT_PUBLIC(int),  MPI_File_write_ordered_begin,(MPI_File fh, const void *buf, int count, MPI_Datatype datatype));
MPI_CALL(XBT_PUBLIC(int),  MPI_File_write_ordered_end,(MPI_File fh, const void *buf, MPI_Status *status));

//FIXME: End of all the not yet implemented stuff

// smpi functions
XBT_PUBLIC(int) smpi_global_size(void);
XBT_PUBLIC(MPI_Comm) smpi_process_comm_self(void);
XBT_PUBLIC(void*) smpi_process_get_user_data(void);
XBT_PUBLIC(void) smpi_process_set_user_data(void *);

XBT_PUBLIC(void) smpi_execute_flops(double flops);
XBT_PUBLIC(void) smpi_execute(double duration);

XBT_PUBLIC(double) smpi_get_host_power_peak_at(int pstate_index);
XBT_PUBLIC(double) smpi_get_host_current_power_peak(void);
XBT_PUBLIC(int) smpi_get_host_nb_pstates(void);
XBT_PUBLIC(void) smpi_set_host_pstate(int pstate_index);
XBT_PUBLIC(int)  smpi_get_host_pstate(void);

XBT_PUBLIC(double) smpi_get_host_consumed_energy(void);

XBT_PUBLIC(int) smpi_usleep(useconds_t usecs);
#if _POSIX_TIMERS > 0
XBT_PUBLIC(int) smpi_nanosleep(const struct timespec *tp, struct timespec * t);
XBT_PUBLIC(int) smpi_clock_gettime(clockid_t clk_id, struct timespec *tp);
#endif
XBT_PUBLIC(unsigned int) smpi_sleep(unsigned int secs);
XBT_PUBLIC(int) smpi_gettimeofday(struct timeval *tv, void* tz);
XBT_PUBLIC(unsigned long long) smpi_rastro_resolution (void);
XBT_PUBLIC(unsigned long long) smpi_rastro_timestamp (void);
XBT_PUBLIC(void) smpi_sample_1(int global, const char *file, int line, int iters, double threshold);
XBT_PUBLIC(int) smpi_sample_2(int global, const char *file, int line);
XBT_PUBLIC(void) smpi_sample_3(int global, const char *file, int line);

/** 
 * Functions for call location tracing. These functions will be
 * called from the user's application! (With the __FILE__ and __LINE__ values
 * passed as parameters.)
 */
XBT_PUBLIC(void) smpi_trace_set_call_location(const char *file, int line);
/** Fortran binding **/
XBT_PUBLIC(void) smpi_trace_set_call_location_(const char *file, int* line);
/** Fortran binding + -fsecond-underscore **/
XBT_PUBLIC(void) smpi_trace_set_call_location__(const char *file, int* line);

#define SMPI_SAMPLE_LOCAL(iters,thres) for(smpi_sample_1(0, __FILE__, __LINE__, iters, thres); \
                                           smpi_sample_2(0, __FILE__, __LINE__);      \
                                           smpi_sample_3(0, __FILE__, __LINE__))

#define SMPI_SAMPLE_GLOBAL(iters,thres) for(smpi_sample_1(1, __FILE__, __LINE__, iters, thres); \
                                            smpi_sample_2(1, __FILE__, __LINE__);      \
                                            smpi_sample_3(1, __FILE__, __LINE__))

#define SMPI_SAMPLE_DELAY(duration) for(smpi_execute(duration); 0; )
#define SMPI_SAMPLE_FLOPS(flops) for(smpi_execute_flops(flops); 0; )

XBT_PUBLIC(void *) smpi_shared_malloc(size_t size, const char *file, int line);
#define SMPI_SHARED_MALLOC(size) smpi_shared_malloc(size, __FILE__, __LINE__)

XBT_PUBLIC(void) smpi_shared_free(void *data);
#define SMPI_SHARED_FREE(data) smpi_shared_free(data)

XBT_PUBLIC(int) smpi_shared_known_call(const char* func, const char* input);
XBT_PUBLIC(void*) smpi_shared_get_call(const char* func, const char* input);
XBT_PUBLIC(void*) smpi_shared_set_call(const char* func, const char* input, void* data);
#define SMPI_SHARED_CALL(func, input, ...) \
   (smpi_shared_known_call(#func, input) ? smpi_shared_get_call(#func, input) \
                                         : smpi_shared_set_call(#func, input, (func(__VA_ARGS__))))

/* Fortran specific stuff */

XBT_PUBLIC(int) __attribute__((weak)) smpi_simulated_main_(int argc, char** argv);
XBT_PUBLIC(int) __attribute__((weak)) MAIN__(void);
XBT_PUBLIC(int) smpi_main(int (*realmain) (int argc, char *argv[]),int argc, char *argv[]);
XBT_PUBLIC(void) __attribute__((weak)) user_main_(void);
XBT_PUBLIC(int) smpi_process_index(void);
XBT_PUBLIC(void) smpi_process_init(int *argc, char ***argv);

/* Trace replay specific stuff */
XBT_PUBLIC(void) smpi_replay_run(int *argc, char***argv);

XBT_PUBLIC(void) SMPI_app_instance_register(const char *name, xbt_main_func_t code, int num_processes);
XBT_PUBLIC(void) SMPI_init(void);
XBT_PUBLIC(void) SMPI_finalize(void);

/* Manual global privatization fallback */
XBT_PUBLIC(void) smpi_register_static(void* arg, void_f_pvoid_t free_fn);
XBT_PUBLIC(void) smpi_free_static(void);

#define SMPI_VARINIT_GLOBAL(name,type)                          \
type *name = NULL;                                              \
static void __attribute__((constructor)) __preinit_##name(void) { \
   if(!name)                                                    \
      name = (type*)calloc(smpi_global_size(), sizeof(type));   \
}                                                               \
static void __attribute__((destructor)) __postfini_##name(void) { \
   free(name);                                                  \
   name = NULL;                                                 \
}

#define SMPI_VARINIT_GLOBAL_AND_SET(name,type,expr)             \
type *name = NULL;                                              \
static void __attribute__((constructor)) __preinit_##name(void) { \
   size_t size = smpi_global_size();                            \
   size_t i;                                                    \
   type value = expr;                                           \
   if(!name) {                                                  \
      name = (type*)malloc(size * sizeof(type));                \
      for(i = 0; i < size; i++) {                               \
         name[i] = value;                                       \
      }                                                         \
   }                                                            \
}                                                               \
static void __attribute__((destructor)) __postfini_##name(void) { \
   free(name);                                                  \
   name = NULL;                                                 \
}

#define SMPI_VARGET_GLOBAL(name) name[smpi_process_index()]

/** 
 * This is used for the old privatization method, i.e., on old
 * machines that do not yet support privatization via mmap
 */
#define SMPI_VARINIT_STATIC(name,type)                      \
static type *name = NULL;                                   \
if(!name) {                                                 \
   name = (type*)calloc(smpi_global_size(), sizeof(type));  \
   smpi_register_static(name, xbt_free_f);                  \
}

#define SMPI_VARINIT_STATIC_AND_SET(name,type,expr) \
static type *name = NULL;                           \
if(!name) {                                         \
   size_t size = smpi_global_size();                \
   size_t i;                                        \
   type value = expr;                               \
   name = (type*)malloc(size * sizeof(type));       \
   for(i = 0; i < size; i++) {                      \
      name[i] = value;                              \
   }                                                \
   smpi_register_static(name, xbt_free_f);          \
}

#define SMPI_VARGET_STATIC(name) name[smpi_process_index()]


SG_END_DECL()
#endif

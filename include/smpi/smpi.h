/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_H
#define SMPI_H

#include <simgrid/forward.h>
#include <smpi/forward.hpp>
#include <smpi/smpi_helpers_internal.h>
#include <xbt/function_types.h>

#include <stddef.h>
#include <sys/time.h>
#include <unistd.h>
#include <xbt/misc.h>

#ifdef __cplusplus
#include <functional>
#include <vector>
#endif

#ifdef _WIN32
#define MPI_CALL(type, name, args)                                                                                     \
  type name args;                                                                                                      \
  type _XBT_CONCAT(P, name) args
#else
#define MPI_CALL(type, name, args)                                                                                     \
  type name args __attribute__((weak));                                                                                \
  type _XBT_CONCAT(P, name) args
#endif

SG_BEGIN_DECL
#define MPI_THREAD_SINGLE     0
#define MPI_THREAD_FUNNELED   1
#define MPI_THREAD_SERIALIZED 2
#define MPI_THREAD_MULTIPLE   3
//FIXME: check values
#define MPI_MAX_PROCESSOR_NAME 100
#define MPI_MAX_NAME_STRING    100
#define MPI_MAX_ERROR_STRING   100
#define MPI_MAX_DATAREP_STRING 128
#define MPI_MAX_INFO_KEY       100
#define MPI_MAX_INFO_VAL       100
#define MPI_MAX_OBJECT_NAME    100
#define MPI_MAX_PORT_NAME      100
#define MPI_MAX_LIBRARY_VERSION_STRING 100
#define MPI_ANY_SOURCE -555
#define MPI_BOTTOM (void *)-111
#define MPI_PROC_NULL -666
#define MPI_ANY_TAG -444
#define MPI_UNDEFINED -333
#define MPI_IN_PLACE (void *)-222

// errorcodes
#define FOREACH_ERROR(ERROR)                    \
          ERROR(MPI_SUCCESS)                    \
          ERROR(MPI_ERR_COMM)                   \
          ERROR(MPI_ERR_ARG)                    \
          ERROR(MPI_ERR_TYPE)                   \
          ERROR(MPI_ERR_REQUEST)                \
          ERROR(MPI_ERR_INTERN)                 \
          ERROR(MPI_ERR_COUNT)                  \
          ERROR(MPI_ERR_RANK)                   \
          ERROR(MPI_ERR_TAG)                    \
          ERROR(MPI_ERR_TRUNCATE)               \
          ERROR(MPI_ERR_GROUP)                  \
          ERROR(MPI_ERR_OP)                     \
          ERROR(MPI_ERR_OTHER)                  \
          ERROR(MPI_ERR_IN_STATUS)              \
          ERROR(MPI_ERR_PENDING)                \
          ERROR(MPI_ERR_BUFFER)                 \
          ERROR(MPI_ERR_NAME)                   \
          ERROR(MPI_ERR_DIMS)                   \
          ERROR(MPI_ERR_TOPOLOGY)               \
          ERROR(MPI_ERR_NO_MEM)                 \
          ERROR(MPI_ERR_WIN)                    \
          ERROR(MPI_ERR_INFO_VALUE)             \
          ERROR(MPI_ERR_INFO_KEY)               \
          ERROR(MPI_ERR_INFO_NOKEY)             \
          ERROR(MPI_ERR_ROOT)                   \
          ERROR(MPI_ERR_UNKNOWN)                \
          ERROR(MPI_ERR_KEYVAL)                 \
          ERROR(MPI_ERR_BASE)                   \
          ERROR(MPI_ERR_SPAWN)                  \
          ERROR(MPI_ERR_PORT)                   \
          ERROR(MPI_ERR_SERVICE)                \
          ERROR(MPI_ERR_SIZE)                   \
          ERROR(MPI_ERR_DISP)                   \
          ERROR(MPI_ERR_INFO)                   \
          ERROR(MPI_ERR_LOCKTYPE)               \
          ERROR(MPI_ERR_ASSERT)                 \
          ERROR(MPI_RMA_CONFLICT)               \
          ERROR(MPI_RMA_SYNC)                   \
          ERROR(MPI_ERR_FILE)                   \
          ERROR(MPI_ERR_NOT_SAME)               \
          ERROR(MPI_ERR_AMODE)                  \
          ERROR(MPI_ERR_UNSUPPORTED_DATAREP)    \
          ERROR(MPI_ERR_UNSUPPORTED_OPERATION)  \
          ERROR(MPI_ERR_NO_SUCH_FILE)           \
          ERROR(MPI_ERR_FILE_EXISTS)            \
          ERROR(MPI_ERR_BAD_FILE)               \
          ERROR(MPI_ERR_ACCESS)                 \
          ERROR(MPI_ERR_NO_SPACE)               \
          ERROR(MPI_ERR_QUOTA)                  \
          ERROR(MPI_ERR_READ_ONLY)              \
          ERROR(MPI_ERR_FILE_IN_USE)            \
          ERROR(MPI_ERR_DUP_DATAREP)            \
          ERROR(MPI_ERR_CONVERSION)             \
          ERROR(MPI_ERR_IO)                     \
          ERROR(MPI_ERR_RMA_ATTACH)             \
          ERROR(MPI_ERR_RMA_CONFLICT)           \
          ERROR(MPI_ERR_RMA_RANGE)              \
          ERROR(MPI_ERR_RMA_SHARED)             \
          ERROR(MPI_ERR_RMA_SYNC)               \
          ERROR(MPI_ERR_RMA_FLAVOR)             \
          ERROR(MPI_T_ERR_CANNOT_INIT)          \
          ERROR(MPI_T_ERR_NOT_INITIALIZED)      \
          ERROR(MPI_T_ERR_MEMORY)               \
          ERROR(MPI_T_ERR_INVALID_INDEX)        \
          ERROR(MPI_T_ERR_INVALID_ITEM)         \
          ERROR(MPI_T_ERR_INVALID_SESSION)      \
          ERROR(MPI_T_ERR_INVALID_HANDLE)       \
          ERROR(MPI_T_ERR_OUT_OF_HANDLES)       \
          ERROR(MPI_T_ERR_OUT_OF_SESSIONS)      \
          ERROR(MPI_T_ERR_CVAR_SET_NOT_NOW)     \
          ERROR(MPI_T_ERR_CVAR_SET_NEVER)       \
          ERROR(MPI_T_ERR_PVAR_NO_WRITE)        \
          ERROR(MPI_T_ERR_PVAR_NO_STARTSTOP)    \
          ERROR(MPI_T_ERR_PVAR_NO_ATOMIC)

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) _XBT_STRINGIFY(STRING),

enum ERROR_ENUM {
    FOREACH_ERROR(GENERATE_ENUM)
};

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
#define MPI_KEYVAL_INVALID   -9

#define MPI_MODE_NOSTORE 0x1
#define MPI_MODE_NOPUT 0x2
#define MPI_MODE_NOPRECEDE 0x4
#define MPI_MODE_NOSUCCEED 0x8
#define MPI_MODE_NOCHECK 0x10

#define MPI_NULL_COPY_FN NULL
#define MPI_NULL_DELETE_FN NULL
#define MPI_ERR_LASTCODE 74

#define MPI_REAL2 MPI_DATATYPE_NULL
#define MPI_COMPLEX4 MPI_DATATYPE_NULL

#define MPI_DISTRIBUTE_BLOCK 0
#define MPI_DISTRIBUTE_NONE 1
#define MPI_DISTRIBUTE_CYCLIC 2
#define MPI_DISTRIBUTE_DFLT_DARG 3
#define MPI_ORDER_C 1
#define MPI_ORDER_FORTRAN 0

#define MPI_TYPECLASS_REAL 0
#define MPI_TYPECLASS_INTEGER 1
#define MPI_TYPECLASS_COMPLEX 2
#define MPI_ROOT 0
#define MPI_INFO_NULL ((MPI_Info)NULL)
#define MPI_COMM_TYPE_SHARED    1
#define MPI_WIN_NULL ((MPI_Win)NULL)

#define MPI_VERSION 3
#define MPI_SUBVERSION 1
#define MPI_UNWEIGHTED      (int *)0
#define MPI_WEIGHTS_EMPTY   (int *)1
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

#define MPI_WIN_BASE -1
#define MPI_WIN_SIZE -2
#define MPI_WIN_DISP_UNIT -3
#define MPI_WIN_CREATE_FLAVOR -4
#define MPI_WIN_MODEL -5
#define MPI_WIN_UNIFIED 0
#define MPI_WIN_SEPARATE 1


typedef ptrdiff_t MPI_Aint;
typedef long long MPI_Offset;
typedef long long MPI_Count;

typedef SMPI_File *MPI_File;
typedef SMPI_Datatype *MPI_Datatype;

typedef struct {
  int MPI_SOURCE;
  int MPI_TAG;
  int MPI_ERROR;
  int count;
  int cancelled;
} MPI_Status;

typedef SMPI_Win* MPI_Win;
typedef SMPI_Info* MPI_Info;

#define MPI_STATUS_IGNORE ((MPI_Status*)NULL)
#define MPI_STATUSES_IGNORE ((MPI_Status*)NULL)
#define MPI_STATUS_SIZE 5

#if !defined(DLL_EXPORT)
#if defined(c_plusplus) || defined(__cplusplus)
#define SMPI_PREDEFINED_POINTER(type, internal) (static_cast<type> (static_cast<void*> (&(internal))))
#else
#define SMPI_PREDEFINED_POINTER(type, internal) ((type) ((void *) &(internal)))
#endif
#else
#define SMPI_PREDEFINED_POINTER(type, internal) ((type) &(internal))
#endif

extern SMPI_Datatype smpi_MPI_DATATYPE_NULL;
extern SMPI_Datatype smpi_MPI_CHAR;
extern SMPI_Datatype smpi_MPI_SHORT;
extern SMPI_Datatype smpi_MPI_INT;
extern SMPI_Datatype smpi_MPI_LONG;
extern SMPI_Datatype smpi_MPI_LONG_LONG;
extern SMPI_Datatype smpi_MPI_SIGNED_CHAR;
extern SMPI_Datatype smpi_MPI_UNSIGNED_CHAR;
extern SMPI_Datatype smpi_MPI_UNSIGNED_SHORT;
extern SMPI_Datatype smpi_MPI_UNSIGNED;
extern SMPI_Datatype smpi_MPI_UNSIGNED_LONG;
extern SMPI_Datatype smpi_MPI_UNSIGNED_LONG_LONG;
extern SMPI_Datatype smpi_MPI_FLOAT;
extern SMPI_Datatype smpi_MPI_DOUBLE;
extern SMPI_Datatype smpi_MPI_LONG_DOUBLE;
extern SMPI_Datatype smpi_MPI_WCHAR;
extern SMPI_Datatype smpi_MPI_C_BOOL;
extern SMPI_Datatype smpi_MPI_INT8_T;
extern SMPI_Datatype smpi_MPI_INT16_T;
extern SMPI_Datatype smpi_MPI_INT32_T;
extern SMPI_Datatype smpi_MPI_INT64_T;
extern SMPI_Datatype smpi_MPI_UINT8_T;
extern SMPI_Datatype smpi_MPI_BYTE;
extern SMPI_Datatype smpi_MPI_UINT16_T;
extern SMPI_Datatype smpi_MPI_UINT32_T;
extern SMPI_Datatype smpi_MPI_UINT64_T;
extern SMPI_Datatype smpi_MPI_C_FLOAT_COMPLEX;
extern SMPI_Datatype smpi_MPI_C_DOUBLE_COMPLEX;
extern SMPI_Datatype smpi_MPI_C_LONG_DOUBLE_COMPLEX;
extern SMPI_Datatype smpi_MPI_AINT;
extern SMPI_Datatype smpi_MPI_OFFSET;
extern SMPI_Datatype smpi_MPI_LB;
extern SMPI_Datatype smpi_MPI_UB;
extern SMPI_Datatype smpi_MPI_FLOAT_INT;
extern SMPI_Datatype smpi_MPI_LONG_INT;
extern SMPI_Datatype smpi_MPI_DOUBLE_INT;
extern SMPI_Datatype smpi_MPI_SHORT_INT;
extern SMPI_Datatype smpi_MPI_2INT;
extern SMPI_Datatype smpi_MPI_LONG_DOUBLE_INT;
extern SMPI_Datatype smpi_MPI_2FLOAT;
extern SMPI_Datatype smpi_MPI_2DOUBLE;
extern SMPI_Datatype smpi_MPI_2LONG;
extern SMPI_Datatype smpi_MPI_REAL;
extern SMPI_Datatype smpi_MPI_REAL4;
extern SMPI_Datatype smpi_MPI_REAL8;
extern SMPI_Datatype smpi_MPI_REAL16;
extern SMPI_Datatype smpi_MPI_COMPLEX8;
extern SMPI_Datatype smpi_MPI_COMPLEX16;
extern SMPI_Datatype smpi_MPI_COMPLEX32;
extern SMPI_Datatype smpi_MPI_INTEGER1;
extern SMPI_Datatype smpi_MPI_INTEGER2;
extern SMPI_Datatype smpi_MPI_INTEGER4;
extern SMPI_Datatype smpi_MPI_INTEGER8;
extern SMPI_Datatype smpi_MPI_INTEGER16;
extern SMPI_Datatype smpi_MPI_COUNT;
extern SMPI_Datatype smpi_MPI_CXX_BOOL;
extern SMPI_Datatype smpi_MPI_MPI_CXX_FLOAT_COMPLEX;
extern SMPI_Datatype smpi_MPI_MPI_CXX_DOULE_COMPLEX;
extern SMPI_Datatype smpi_MPI_MPI_CXX_LONG_DOUBLE_COMPLEX;

#define MPI_DATATYPE_NULL SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_DATATYPE_NULL)
#define MPI_CHAR SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_CHAR)
#define MPI_SHORT SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_SHORT)
#define MPI_INT SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_INT)
#define MPI_LONG SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_LONG)
#define MPI_LONG_LONG SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_LONG_LONG)
#define MPI_LONG_LONG_INT MPI_LONG_LONG
#define MPI_SIGNED_CHAR SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_SIGNED_CHAR)
#define MPI_UNSIGNED_CHAR SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_UNSIGNED_CHAR)
#define MPI_UNSIGNED_SHORT SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_UNSIGNED_SHORT)
#define MPI_UNSIGNED SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_UNSIGNED)
#define MPI_UNSIGNED_LONG SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_UNSIGNED_LONG)
#define MPI_UNSIGNED_LONG_LONG SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_UNSIGNED_LONG_LONG)
#define MPI_FLOAT SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_FLOAT)
#define MPI_DOUBLE SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_DOUBLE)
#define MPI_LONG_DOUBLE SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_LONG_DOUBLE)
#define MPI_WCHAR SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_WCHAR)
#define MPI_C_BOOL SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_C_BOOL)
#define MPI_INT8_T SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_INT8_T)
#define MPI_INT16_T SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_INT16_T)
#define MPI_INT32_T SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_INT32_T)
#define MPI_INT64_T SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_INT64_T)
#define MPI_UINT8_T SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_UINT8_T)
#define MPI_BYTE SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_BYTE)
#define MPI_UINT16_T SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_UINT16_T)
#define MPI_UINT32_T SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_UINT32_T)
#define MPI_UINT64_T SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_UINT64_T)
#define MPI_C_FLOAT_COMPLEX SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_C_FLOAT_COMPLEX)
#define MPI_C_COMPLEX MPI_C_FLOAT_COMPLEX
#define MPI_C_DOUBLE_COMPLEX SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_C_DOUBLE_COMPLEX)
#define MPI_C_LONG_DOUBLE_COMPLEX SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_C_LONG_DOUBLE_COMPLEX)
#define MPI_AINT SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_AINT)
#define MPI_OFFSET SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_OFFSET)
#define MPI_LB SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_LB)
#define MPI_UB SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_UB)
//The following are datatypes for the MPI functions MPI_MAXLOC  and MPI_MINLOC.
#define MPI_FLOAT_INT SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_FLOAT_INT)
#define MPI_LONG_INT SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_LONG_INT)
#define MPI_DOUBLE_INT SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_DOUBLE_INT)
#define MPI_SHORT_INT SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_SHORT_INT)
#define MPI_2INT SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_2INT)
#define MPI_LONG_DOUBLE_INT SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_LONG_DOUBLE_INT)
#define MPI_2FLOAT SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_2FLOAT)
#define MPI_2DOUBLE SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_2DOUBLE)
#define MPI_2LONG SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_2LONG)
 // only for compatibility with Fortran
#define MPI_REAL SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_REAL)
#define MPI_REAL4 SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_REAL4)
#define MPI_REAL8 SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_REAL8)
#define MPI_REAL16 SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_REAL16)
#define MPI_COMPLEX8 SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_COMPLEX8)
#define MPI_COMPLEX16 SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_COMPLEX16)
#define MPI_COMPLEX32 SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_COMPLEX32)
#define MPI_INTEGER1 SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_INTEGER1)
#define MPI_INTEGER2 SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_INTEGER2)
#define MPI_INTEGER4 SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_INTEGER4)
#define MPI_INTEGER8 SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_INTEGER8)
#define MPI_INTEGER16 SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_INTEGER16)
#define MPI_COUNT SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_COUNT)

#if defined(c_plusplus) || defined(__cplusplus)
#define MPI_CXX_BOOL SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_CXX_BOOL)
#define MPI_CXX_FLOAT_COMPLEX SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_CXX_FLOAT_COMPLEX)
#define MPI_CXX_DOUBLE_COMPLEX SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_CXX_DOUBLE_COMPLEX)
#define MPI_CXX_LONG_DOUBLE_COMPLEX SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_CXX_LONG_DOUBLE_COMPLEX)
#else
#define MPI_CXX_BOOL MPI_DATATYPE_NULL
#define MPI_CXX_FLOAT_COMPLEX MPI_DATATYPE_NULL
#define MPI_CXX_DOUBLE_COMPLEX MPI_DATATYPE_NULL
#define MPI_CXX_LONG_DOUBLE_COMPLEX MPI_DATATYPE_NULL
#endif

//defines for fortran compatibility
#if defined(__alpha__) || defined(__sparc64__) || defined(__x86_64__) || defined(__ia64__) || defined(__aarch64__)
#define MPI_INTEGER MPI_INT
#define MPI_2INTEGER MPI_2INT
#define MPI_LOGICAL MPI_INT
#else
#define MPI_INTEGER MPI_LONG
#define MPI_2INTEGER MPI_2LONG
#define MPI_LOGICAL MPI_LONG
#endif

typedef int MPI_Fint;

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

#define MPI_WIN_FLAVOR_CREATE 0
#define MPI_WIN_FLAVOR_ALLOCATE 1
#define MPI_WIN_FLAVOR_DYNAMIC 2
#define MPI_WIN_FLAVOR_SHARED 3

typedef void MPI_User_function(void *invec, void *inoutvec, int *len, MPI_Datatype * datatype);
typedef SMPI_Op *MPI_Op;

#define MPI_OP_NULL ((MPI_Op)NULL)
extern SMPI_Op smpi_MPI_MAX;
extern SMPI_Op smpi_MPI_MIN;
extern SMPI_Op smpi_MPI_MAXLOC;
extern SMPI_Op smpi_MPI_MINLOC;
extern SMPI_Op smpi_MPI_SUM;
extern SMPI_Op smpi_MPI_PROD;
extern SMPI_Op smpi_MPI_LAND;
extern SMPI_Op smpi_MPI_LOR;
extern SMPI_Op smpi_MPI_LXOR;
extern SMPI_Op smpi_MPI_BAND;
extern SMPI_Op smpi_MPI_BOR;
extern SMPI_Op smpi_MPI_BXOR;
extern SMPI_Op smpi_MPI_REPLACE;
extern SMPI_Op smpi_MPI_NO_OP;

#define MPI_MAX SMPI_PREDEFINED_POINTER(MPI_Op, smpi_MPI_MAX)
#define MPI_MIN SMPI_PREDEFINED_POINTER(MPI_Op, smpi_MPI_MIN)
#define MPI_MAXLOC SMPI_PREDEFINED_POINTER(MPI_Op, smpi_MPI_MAXLOC)
#define MPI_MINLOC SMPI_PREDEFINED_POINTER(MPI_Op, smpi_MPI_MINLOC)
#define MPI_SUM SMPI_PREDEFINED_POINTER(MPI_Op, smpi_MPI_SUM)
#define MPI_PROD SMPI_PREDEFINED_POINTER(MPI_Op, smpi_MPI_PROD)
#define MPI_LAND SMPI_PREDEFINED_POINTER(MPI_Op, smpi_MPI_LAND)
#define MPI_LOR SMPI_PREDEFINED_POINTER(MPI_Op, smpi_MPI_LOR)
#define MPI_LXOR SMPI_PREDEFINED_POINTER(MPI_Op, smpi_MPI_LXOR)
#define MPI_BAND SMPI_PREDEFINED_POINTER(MPI_Op, smpi_MPI_BAND)
#define MPI_BOR SMPI_PREDEFINED_POINTER(MPI_Op, smpi_MPI_BOR)
#define MPI_BXOR SMPI_PREDEFINED_POINTER(MPI_Op, smpi_MPI_BXOR)

//For accumulate
#define MPI_REPLACE SMPI_PREDEFINED_POINTER(MPI_Op, smpi_MPI_REPLACE)
#define MPI_NO_OP SMPI_PREDEFINED_POINTER(MPI_Op, smpi_MPI_NO_OP)


typedef SMPI_Group* MPI_Group;

extern SMPI_Group smpi_MPI_GROUP_EMPTY;
#define MPI_GROUP_EMPTY SMPI_PREDEFINED_POINTER(MPI_Group, smpi_MPI_GROUP_EMPTY)

#define MPI_GROUP_NULL ((MPI_Group)NULL)

typedef SMPI_Comm* MPI_Comm;

#define MPI_COMM_NULL ((MPI_Comm)NULL)
XBT_PUBLIC_DATA MPI_Comm MPI_COMM_WORLD;
#define MPI_COMM_SELF smpi_process_comm_self()

typedef SMPI_Request* MPI_Request;

#define MPIO_Request MPI_Request
#define MPI_REQUEST_NULL ((MPI_Request)NULL)
#define MPI_FORTRAN_REQUEST_NULL -1

typedef SMPI_Errhandler* MPI_Errhandler;
#define MPI_ERRHANDLER_NULL ((MPI_Errhandler)NULL)
extern SMPI_Errhandler smpi_MPI_ERRORS_RETURN;
#define MPI_ERRORS_RETURN SMPI_PREDEFINED_POINTER(MPI_Errhandler, smpi_MPI_ERRORS_RETURN)
extern SMPI_Errhandler smpi_MPI_ERRORS_ARE_FATAL;
#define MPI_ERRORS_ARE_FATAL SMPI_PREDEFINED_POINTER(MPI_Errhandler, smpi_MPI_ERRORS_ARE_FATAL)

typedef enum SMPI_Combiner_enum{
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
}SMPI_Combiner_enum;

typedef enum SMPI_Topo_type {
  MPI_GRAPH=1,
  MPI_CART=2,
  MPI_DIST_GRAPH=3,
  MPI_INVALID_TOPO=-1
} SMPI_Topo_type;

typedef int MPI_Copy_function(MPI_Comm oldcomm, int keyval, void* extra_state, void* attribute_val_in,
                              void* attribute_val_out, int* flag);
typedef int MPI_Delete_function(MPI_Comm comm, int keyval, void* attribute_val, void* extra_state);
typedef void MPI_Copy_function_fort(MPI_Comm oldcomm, int keyval, void* extra_state, void* attribute_val_in,
                              void* attribute_val_out, int* flag, int* ierr);
typedef void MPI_Delete_function_fort(MPI_Comm comm, int keyval, void* attribute_val, void* extra_state, int* ierr);
#define MPI_Comm_copy_attr_function MPI_Copy_function
#define MPI_Comm_delete_attr_function MPI_Delete_function
#define MPI_Comm_copy_attr_function_fort MPI_Copy_function_fort
#define MPI_Comm_delete_attr_function_fort MPI_Delete_function_fort
typedef int MPI_Type_copy_attr_function(MPI_Datatype type, int keyval, void* extra_state, void* attribute_val_in,
                              void* attribute_val_out, int* flag);
typedef int MPI_Type_delete_attr_function(MPI_Datatype type, int keyval, void* attribute_val, void* extra_state);
typedef int MPI_Win_copy_attr_function(MPI_Win win, int keyval, void* extra_state, void* attribute_val_in,
                              void* attribute_val_out, int* flag);
typedef int MPI_Win_delete_attr_function(MPI_Win win, int keyval, void* attribute_val, void* extra_state);
typedef void MPI_Type_copy_attr_function_fort(MPI_Datatype type, int keyval, void* extra_state, void* attribute_val_in,
                              void* attribute_val_out, int* flag, int* ierr);
typedef void MPI_Type_delete_attr_function_fort(MPI_Datatype type, int keyval, void* attribute_val, void* extra_state, int* ierr);
typedef void MPI_Win_copy_attr_function_fort(MPI_Win win, int keyval, void* extra_state, void* attribute_val_in,
                              void* attribute_val_out, int* flag, int* ierr);
typedef void MPI_Win_delete_attr_function_fort(MPI_Win win, int keyval, void* attribute_val, void* extra_state, int* ierr);
typedef int MPI_Grequest_query_function(void *extra_state, MPI_Status *status);
typedef int MPI_Grequest_free_function(void *extra_state);
typedef int MPI_Grequest_cancel_function(void *extra_state, int complete);
#define MPI_COMM_NULL_COPY_FN ((MPI_Comm_copy_attr_function*)0)
#define MPI_COMM_NULL_DELETE_FN ((MPI_Comm_delete_attr_function*)0)
#define MPI_TYPE_NULL_COPY_FN ((MPI_Type_copy_attr_function*)0)
#define MPI_TYPE_NULL_DELETE_FN ((MPI_Type_delete_attr_function*)0)
#define MPI_WIN_NULL_COPY_FN ((MPI_Win_copy_attr_function*)0)
#define MPI_WIN_NULL_DELETE_FN ((MPI_Win_delete_attr_function*)0)

typedef int (MPI_Datarep_extent_function)(MPI_Datatype, MPI_Aint *, void *);
typedef int (MPI_Datarep_conversion_function)(void *, MPI_Datatype, int, void *, MPI_Offset, void *);

typedef void MPI_Handler_function(MPI_Comm*, int*, ...);
typedef void MPI_Comm_errhandler_function(MPI_Comm *, int *, ...);
typedef void MPI_File_errhandler_function(MPI_File *, int *, ...);
typedef void MPI_Win_errhandler_function(MPI_Win *, int *, ...);
typedef MPI_Comm_errhandler_function MPI_Comm_errhandler_fn;
typedef MPI_File_errhandler_function MPI_File_errhandler_fn;
typedef MPI_Win_errhandler_function MPI_Win_errhandler_fn;

MPI_CALL(XBT_PUBLIC int, MPI_Init, (int* argc, char*** argv));
MPI_CALL(XBT_PUBLIC int, MPI_Finalize, (void));
MPI_CALL(XBT_PUBLIC int, MPI_Finalized, (int* flag));
MPI_CALL(XBT_PUBLIC int, MPI_Init_thread, (int* argc, char*** argv, int required, int* provided));
MPI_CALL(XBT_PUBLIC int, MPI_Initialized, (int* flag));
MPI_CALL(XBT_PUBLIC int, MPI_Query_thread, (int* provided));
MPI_CALL(XBT_PUBLIC int, MPI_Is_thread_main, (int* flag));
MPI_CALL(XBT_PUBLIC int, MPI_Get_version, (int* version, int* subversion));
MPI_CALL(XBT_PUBLIC int, MPI_Get_library_version, (char* version, int* len));
MPI_CALL(XBT_PUBLIC int, MPI_Get_processor_name, (char* name, int* resultlen));
MPI_CALL(XBT_PUBLIC int, MPI_Abort, (MPI_Comm comm, int errorcode));
MPI_CALL(XBT_PUBLIC int, MPI_Alloc_mem, (MPI_Aint size, MPI_Info info, void* baseptr));
MPI_CALL(XBT_PUBLIC int, MPI_Free_mem, (void* base));
MPI_CALL(XBT_PUBLIC double, MPI_Wtime, (void));
MPI_CALL(XBT_PUBLIC double, MPI_Wtick, (void));
MPI_CALL(XBT_PUBLIC int, MPI_Buffer_attach, (void* buffer, int size));
MPI_CALL(XBT_PUBLIC int, MPI_Buffer_detach, (void* buffer, int* size));
MPI_CALL(XBT_PUBLIC int, MPI_Address, (const void* location, MPI_Aint* address));
MPI_CALL(XBT_PUBLIC int, MPI_Get_address, (const void* location, MPI_Aint* address));
MPI_CALL(XBT_PUBLIC MPI_Aint, MPI_Aint_diff, (MPI_Aint base, MPI_Aint disp));
MPI_CALL(XBT_PUBLIC MPI_Aint, MPI_Aint_add, (MPI_Aint base, MPI_Aint disp));
MPI_CALL(XBT_PUBLIC int, MPI_Error_class, (int errorcode, int* errorclass));
MPI_CALL(XBT_PUBLIC int, MPI_Error_string, (int errorcode, char* string, int* resultlen));

MPI_CALL(XBT_PUBLIC int, MPI_Attr_delete, (MPI_Comm comm, int keyval));
MPI_CALL(XBT_PUBLIC int, MPI_Attr_get, (MPI_Comm comm, int keyval, void* attr_value, int* flag));
MPI_CALL(XBT_PUBLIC int, MPI_Attr_put, (MPI_Comm comm, int keyval, void* attr_value));
MPI_CALL(XBT_PUBLIC int, MPI_Keyval_create,
         (MPI_Copy_function * copy_fn, MPI_Delete_function* delete_fn, int* keyval, void* extra_state));
MPI_CALL(XBT_PUBLIC int, MPI_Keyval_free, (int* keyval));

MPI_CALL(XBT_PUBLIC int, MPI_Type_free, (MPI_Datatype * datatype));
MPI_CALL(XBT_PUBLIC int, MPI_Type_size, (MPI_Datatype datatype, int* size));
MPI_CALL(XBT_PUBLIC int, MPI_Type_size_x, (MPI_Datatype datatype, MPI_Count* size));
MPI_CALL(XBT_PUBLIC int, MPI_Type_get_extent, (MPI_Datatype datatype, MPI_Aint* lb, MPI_Aint* extent));
MPI_CALL(XBT_PUBLIC int, MPI_Type_get_extent_x, (MPI_Datatype datatype, MPI_Count* lb, MPI_Count* extent));
MPI_CALL(XBT_PUBLIC int, MPI_Type_get_true_extent, (MPI_Datatype datatype, MPI_Aint* lb, MPI_Aint* extent));
MPI_CALL(XBT_PUBLIC int, MPI_Type_get_true_extent_x, (MPI_Datatype datatype, MPI_Count* lb, MPI_Count* extent));
MPI_CALL(XBT_PUBLIC int, MPI_Type_extent, (MPI_Datatype datatype, MPI_Aint* extent));
MPI_CALL(XBT_PUBLIC int, MPI_Type_lb, (MPI_Datatype datatype, MPI_Aint* disp));
MPI_CALL(XBT_PUBLIC int, MPI_Type_ub, (MPI_Datatype datatype, MPI_Aint* disp));
MPI_CALL(XBT_PUBLIC int, MPI_Type_commit, (MPI_Datatype * datatype));
MPI_CALL(XBT_PUBLIC int, MPI_Type_hindexed,
         (int count, const int* blocklens, const MPI_Aint* indices, MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC int, MPI_Type_create_hindexed,
         (int count, const int* blocklens, const MPI_Aint* indices, MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC int, MPI_Type_create_hindexed_block,
         (int count, int blocklength, const MPI_Aint* indices, MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC int, MPI_Type_hvector,
         (int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC int, MPI_Type_create_hvector,
         (int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC int, MPI_Type_indexed,
         (int count, const int* blocklens, const int* indices, MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC int, MPI_Type_create_indexed,
         (int count, const int* blocklens, const int* indices, MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC int, MPI_Type_create_indexed_block,
         (int count, int blocklength, const int* indices, MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC int, MPI_Type_struct,
         (int count, const int* blocklens, const MPI_Aint* indices, const MPI_Datatype* old_types, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC int, MPI_Type_create_struct,
         (int count, const int* blocklens, const MPI_Aint* indices, const MPI_Datatype* old_types, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC int, MPI_Type_vector,
         (int count, int blocklen, int stride, MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC int, MPI_Type_contiguous, (int count, MPI_Datatype old_type, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC int, MPI_Type_create_resized,
         (MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC MPI_Datatype, MPI_Type_f2c, (MPI_Fint datatype));
MPI_CALL(XBT_PUBLIC MPI_Fint, MPI_Type_c2f, (MPI_Datatype datatype));
MPI_CALL(XBT_PUBLIC int, MPI_Get_count, (const MPI_Status * status, MPI_Datatype datatype, int* count));
MPI_CALL(XBT_PUBLIC int, MPI_Type_get_attr, (MPI_Datatype type, int type_keyval, void* attribute_val, int* flag));
MPI_CALL(XBT_PUBLIC int, MPI_Type_set_attr, (MPI_Datatype type, int type_keyval, void* att));
MPI_CALL(XBT_PUBLIC int, MPI_Type_delete_attr, (MPI_Datatype type, int comm_keyval));
MPI_CALL(XBT_PUBLIC int, MPI_Type_create_keyval,
         (MPI_Type_copy_attr_function * copy_fn, MPI_Type_delete_attr_function* delete_fn, int* keyval,
          void* extra_state));
MPI_CALL(XBT_PUBLIC int, MPI_Type_free_keyval, (int* keyval));
MPI_CALL(XBT_PUBLIC int, MPI_Type_dup, (MPI_Datatype datatype, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC int, MPI_Type_set_name, (MPI_Datatype datatype, const char* name));
MPI_CALL(XBT_PUBLIC int, MPI_Type_get_name, (MPI_Datatype datatype, char* name, int* len));

MPI_CALL(XBT_PUBLIC int, MPI_Pack,
         (const void* inbuf, int incount, MPI_Datatype type, void* outbuf, int outcount, int* position, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Pack_size, (int incount, MPI_Datatype datatype, MPI_Comm comm, int* size));
MPI_CALL(XBT_PUBLIC int, MPI_Unpack,
         (const void* inbuf, int insize, int* position, void* outbuf, int outcount, MPI_Datatype type, MPI_Comm comm));

MPI_CALL(XBT_PUBLIC int, MPI_Op_create, (MPI_User_function * function, int commute, MPI_Op* op));
MPI_CALL(XBT_PUBLIC int, MPI_Op_free, (MPI_Op * op));
MPI_CALL(XBT_PUBLIC int, MPI_Op_commutative, (MPI_Op op, int* commute));
MPI_CALL(XBT_PUBLIC MPI_Op, MPI_Op_f2c, (MPI_Fint op));
MPI_CALL(XBT_PUBLIC MPI_Fint, MPI_Op_c2f, (MPI_Op op));

MPI_CALL(XBT_PUBLIC int, MPI_Group_free, (MPI_Group * group));
MPI_CALL(XBT_PUBLIC int, MPI_Group_size, (MPI_Group group, int* size));
MPI_CALL(XBT_PUBLIC int, MPI_Group_rank, (MPI_Group group, int* rank));
MPI_CALL(XBT_PUBLIC int, MPI_Group_translate_ranks,
         (MPI_Group group1, int n, const int* ranks1, MPI_Group group2, int* ranks2));
MPI_CALL(XBT_PUBLIC int, MPI_Group_compare, (MPI_Group group1, MPI_Group group2, int* result));
MPI_CALL(XBT_PUBLIC int, MPI_Group_union, (MPI_Group group1, MPI_Group group2, MPI_Group* newgroup));
MPI_CALL(XBT_PUBLIC int, MPI_Group_intersection, (MPI_Group group1, MPI_Group group2, MPI_Group* newgroup));
MPI_CALL(XBT_PUBLIC int, MPI_Group_difference, (MPI_Group group1, MPI_Group group2, MPI_Group* newgroup));
MPI_CALL(XBT_PUBLIC int, MPI_Group_incl, (MPI_Group group, int n, const int* ranks, MPI_Group* newgroup));
MPI_CALL(XBT_PUBLIC int, MPI_Group_excl, (MPI_Group group, int n, const int* ranks, MPI_Group* newgroup));
MPI_CALL(XBT_PUBLIC int, MPI_Group_range_incl, (MPI_Group group, int n, int ranges[][3], MPI_Group* newgroup));
MPI_CALL(XBT_PUBLIC int, MPI_Group_range_excl, (MPI_Group group, int n, int ranges[][3], MPI_Group* newgroup));
MPI_CALL(XBT_PUBLIC MPI_Group, MPI_Group_f2c, (MPI_Fint group));
MPI_CALL(XBT_PUBLIC MPI_Fint, MPI_Group_c2f, (MPI_Group group));

MPI_CALL(XBT_PUBLIC int, MPI_Comm_rank, (MPI_Comm comm, int* rank));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_size, (MPI_Comm comm, int* size));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_get_name, (MPI_Comm comm, char* name, int* len));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_set_name, (MPI_Comm comm, const char* name));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_dup, (MPI_Comm comm, MPI_Comm* newcomm));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_dup_with_info, (MPI_Comm comm, MPI_Info info, MPI_Comm* newcomm));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_get_attr, (MPI_Comm comm, int comm_keyval, void* attribute_val, int* flag));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_set_attr, (MPI_Comm comm, int comm_keyval, void* attribute_val));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_delete_attr, (MPI_Comm comm, int comm_keyval));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_create_keyval,
         (MPI_Comm_copy_attr_function * copy_fn, MPI_Comm_delete_attr_function* delete_fn, int* keyval,
          void* extra_state));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_free_keyval, (int* keyval));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_group, (MPI_Comm comm, MPI_Group* group));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_compare, (MPI_Comm comm1, MPI_Comm comm2, int* result));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_create, (MPI_Comm comm, MPI_Group group, MPI_Comm* newcomm));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_create_group, (MPI_Comm comm, MPI_Group group, int tag, MPI_Comm* newcomm));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_free, (MPI_Comm * comm));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_disconnect, (MPI_Comm * comm));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_split, (MPI_Comm comm, int color, int key, MPI_Comm* comm_out));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_set_info, (MPI_Comm comm, MPI_Info info));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_get_info, (MPI_Comm comm, MPI_Info* info));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_split_type,
         (MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm* newcomm));
MPI_CALL(XBT_PUBLIC MPI_Comm, MPI_Comm_f2c, (MPI_Fint comm));
MPI_CALL(XBT_PUBLIC MPI_Fint, MPI_Comm_c2f, (MPI_Comm comm));


MPI_CALL(XBT_PUBLIC int, MPI_Start, (MPI_Request * request));
MPI_CALL(XBT_PUBLIC int, MPI_Startall, (int count, MPI_Request* requests));
MPI_CALL(XBT_PUBLIC int, MPI_Request_free, (MPI_Request * request));
MPI_CALL(XBT_PUBLIC int, MPI_Recv,
         (void* buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_Recv_init,
         (void* buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_Irecv,
         (void* buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_Send, (const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Send_init,
         (const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_Isend,
         (const void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_Ssend, (const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Ssend_init,
         (const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_Issend,
         (const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_Bsend, (const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Bsend_init,
         (const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_Ibsend,
         (const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_Rsend, (const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Rsend_init,
         (const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_Irsend,
         (const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_Sendrecv,
         (const void* sendbuf, int sendcount, MPI_Datatype sendtype, int dst, int sendtag, void* recvbuf, int recvcount,
          MPI_Datatype recvtype, int src, int recvtag, MPI_Comm comm, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_Sendrecv_replace, (void* buf, int count, MPI_Datatype datatype, int dst, int sendtag,
                                                int src, int recvtag, MPI_Comm comm, MPI_Status* status));

MPI_CALL(XBT_PUBLIC int, MPI_Test, (MPI_Request * request, int* flag, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_Testany, (int count, MPI_Request requests[], int* index, int* flag, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_Testall, (int count, MPI_Request* requests, int* flag, MPI_Status* statuses));
MPI_CALL(XBT_PUBLIC int, MPI_Testsome,
         (int incount, MPI_Request requests[], int* outcount, int* indices, MPI_Status status[]));
MPI_CALL(XBT_PUBLIC int, MPI_Test_cancelled, (const MPI_Status * status, int* flag));
MPI_CALL(XBT_PUBLIC int, MPI_Wait, (MPI_Request * request, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_Waitany, (int count, MPI_Request requests[], int* index, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_Waitall, (int count, MPI_Request requests[], MPI_Status status[]));
MPI_CALL(XBT_PUBLIC int, MPI_Waitsome,
         (int incount, MPI_Request requests[], int* outcount, int* indices, MPI_Status status[]));
MPI_CALL(XBT_PUBLIC int, MPI_Iprobe, (int source, int tag, MPI_Comm comm, int* flag, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_Probe, (int source, int tag, MPI_Comm comm, MPI_Status* status));
MPI_CALL(XBT_PUBLIC MPI_Request, MPI_Request_f2c, (MPI_Fint request));
MPI_CALL(XBT_PUBLIC MPI_Fint, MPI_Request_c2f, (MPI_Request request));
MPI_CALL(XBT_PUBLIC int, MPI_Cancel, (MPI_Request * request));

MPI_CALL(XBT_PUBLIC int, MPI_Bcast, (void* buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Barrier, (MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Ibarrier, (MPI_Comm comm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Ibcast, (void* buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Igather, (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                                      MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Igatherv, (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                       const int* recvcounts, const int* displs, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Iallgather, (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                         int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Iallgatherv, (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                          const int* recvcounts, const int* displs, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Iscatter, (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                       int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Iscatterv, (const void* sendbuf, const int* sendcounts, const int* displs, MPI_Datatype sendtype,
                                        void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Ireduce,
         (const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Iallreduce,
         (const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Iscan,
         (const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Iexscan,
         (const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Ireduce_scatter,
         (const void* sendbuf, void* recvbuf, const int* recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Ireduce_scatter_block,
         (const void* sendbuf, void* recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Ialltoall, (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                        int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Ialltoallv,
         (const void* sendbuf, const int* sendcounts, const int* senddisps, MPI_Datatype sendtype, void* recvbuf, const int* recvcounts,
          const int* recvdisps, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Ialltoallw,
         (const void* sendbuf, const int* sendcounts, const int* senddisps, const MPI_Datatype* sendtypes, void* recvbuf, const int* recvcounts,
          const int* recvdisps, const MPI_Datatype* recvtypes, MPI_Comm comm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Gather, (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                                      MPI_Datatype recvtype, int root, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Gatherv, (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                       const int* recvcounts, const int* displs, MPI_Datatype recvtype, int root, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Allgather, (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                         int recvcount, MPI_Datatype recvtype, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Allgatherv, (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                          const int* recvcounts, const int* displs, MPI_Datatype recvtype, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Scatter, (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                       int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Scatterv, (const void* sendbuf, const int* sendcounts, const int* displs, MPI_Datatype sendtype,
                                        void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Reduce,
         (const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Allreduce,
         (const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Scan,
         (const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Exscan,
         (const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Reduce_scatter,
         (const void* sendbuf, void* recvbuf, const int* recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Reduce_scatter_block,
         (const void* sendbuf, void* recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Alltoall, (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                        int recvcount, MPI_Datatype recvtype, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Alltoallv,
         (const void* sendbuf, const int* sendcounts, const int* senddisps, MPI_Datatype sendtype, void* recvbuf, const int* recvcounts,
          const int* recvdisps, MPI_Datatype recvtype, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Alltoallw,
         (const void* sendbuf, const int* sendcnts, const int* sdispls, const MPI_Datatype* sendtypes, void* recvbuf, const int* recvcnts,
          const int* rdispls, const MPI_Datatype* recvtypes, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Reduce_local, (const void* inbuf, void* inoutbuf, int count, MPI_Datatype datatype, MPI_Op op));

MPI_CALL(XBT_PUBLIC int, MPI_Info_create, (MPI_Info * info));
MPI_CALL(XBT_PUBLIC int, MPI_Info_set, (MPI_Info info, const char* key, const char* value));
MPI_CALL(XBT_PUBLIC int, MPI_Info_get, (MPI_Info info, const char* key, int valuelen, char* value, int* flag));
MPI_CALL(XBT_PUBLIC int, MPI_Info_free, (MPI_Info * info));
MPI_CALL(XBT_PUBLIC int, MPI_Info_delete, (MPI_Info info, const char* key));
MPI_CALL(XBT_PUBLIC int, MPI_Info_dup, (MPI_Info info, MPI_Info* newinfo));
MPI_CALL(XBT_PUBLIC int, MPI_Info_get_nkeys, (MPI_Info info, int* nkeys));
MPI_CALL(XBT_PUBLIC int, MPI_Info_get_nthkey, (MPI_Info info, int n, char* key));
MPI_CALL(XBT_PUBLIC int, MPI_Info_get_valuelen, (MPI_Info info, const char* key, int* valuelen, int* flag));
MPI_CALL(XBT_PUBLIC MPI_Info, MPI_Info_f2c, (MPI_Fint info));
MPI_CALL(XBT_PUBLIC MPI_Fint, MPI_Info_c2f, (MPI_Info info));

MPI_CALL(XBT_PUBLIC int, MPI_Win_free, (MPI_Win * win));
MPI_CALL(XBT_PUBLIC int, MPI_Win_create,
         (void* base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win* win));
MPI_CALL(XBT_PUBLIC int, MPI_Win_allocate,
         (MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void* base, MPI_Win* win));
MPI_CALL(XBT_PUBLIC int, MPI_Win_allocate_shared,
         (MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void* base, MPI_Win* win));
MPI_CALL(XBT_PUBLIC int, MPI_Win_create_dynamic, (MPI_Info info, MPI_Comm comm, MPI_Win* win));
MPI_CALL(XBT_PUBLIC int, MPI_Win_attach, (MPI_Win win, void* base, MPI_Aint size));
MPI_CALL(XBT_PUBLIC int, MPI_Win_detach, (MPI_Win win, const void* base));
MPI_CALL(XBT_PUBLIC int, MPI_Win_set_name, (MPI_Win win, const char* name));
MPI_CALL(XBT_PUBLIC int, MPI_Win_get_name, (MPI_Win win, char* name, int* len));
MPI_CALL(XBT_PUBLIC int, MPI_Win_set_info, (MPI_Win win, MPI_Info info));
MPI_CALL(XBT_PUBLIC int, MPI_Win_get_info, (MPI_Win win, MPI_Info* info));
MPI_CALL(XBT_PUBLIC int, MPI_Win_get_group, (MPI_Win win, MPI_Group* group));
MPI_CALL(XBT_PUBLIC int, MPI_Win_fence, (int assert, MPI_Win win));
MPI_CALL(XBT_PUBLIC int, MPI_Win_get_attr, (MPI_Win type, int type_keyval, void* attribute_val, int* flag));
MPI_CALL(XBT_PUBLIC int, MPI_Win_set_attr, (MPI_Win type, int type_keyval, void* att));
MPI_CALL(XBT_PUBLIC int, MPI_Win_delete_attr, (MPI_Win type, int comm_keyval));
MPI_CALL(XBT_PUBLIC int, MPI_Win_create_keyval,
         (MPI_Win_copy_attr_function * copy_fn, MPI_Win_delete_attr_function* delete_fn, int* keyval,
          void* extra_state));
MPI_CALL(XBT_PUBLIC int, MPI_Win_free_keyval, (int* keyval));
MPI_CALL(XBT_PUBLIC int, MPI_Win_complete, (MPI_Win win));
MPI_CALL(XBT_PUBLIC int, MPI_Win_post, (MPI_Group group, int assert, MPI_Win win));
MPI_CALL(XBT_PUBLIC int, MPI_Win_start, (MPI_Group group, int assert, MPI_Win win));
MPI_CALL(XBT_PUBLIC int, MPI_Win_wait, (MPI_Win win));
MPI_CALL(XBT_PUBLIC int, MPI_Win_lock, (int lock_type, int rank, int assert, MPI_Win win));
MPI_CALL(XBT_PUBLIC int, MPI_Win_lock_all, (int assert, MPI_Win win));
MPI_CALL(XBT_PUBLIC int, MPI_Win_unlock, (int rank, MPI_Win win));
MPI_CALL(XBT_PUBLIC int, MPI_Win_unlock_all, (MPI_Win win));
MPI_CALL(XBT_PUBLIC int, MPI_Win_flush, (int rank, MPI_Win win));
MPI_CALL(XBT_PUBLIC int, MPI_Win_flush_local, (int rank, MPI_Win win));
MPI_CALL(XBT_PUBLIC int, MPI_Win_flush_all, (MPI_Win win));
MPI_CALL(XBT_PUBLIC int, MPI_Win_flush_local_all, (MPI_Win win));
MPI_CALL(XBT_PUBLIC int, MPI_Win_shared_query, (MPI_Win win, int rank, MPI_Aint* size, int* disp_unit, void* baseptr));
MPI_CALL(XBT_PUBLIC int, MPI_Win_sync, (MPI_Win win));
MPI_CALL(XBT_PUBLIC MPI_Win, MPI_Win_f2c, (MPI_Fint win));
MPI_CALL(XBT_PUBLIC MPI_Fint, MPI_Win_c2f, (MPI_Win win));

MPI_CALL(XBT_PUBLIC int, MPI_Get, (void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
                                   MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win));
MPI_CALL(XBT_PUBLIC int, MPI_Put, (const void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
                                   MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win));
MPI_CALL(XBT_PUBLIC int, MPI_Accumulate,
         (const void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
          int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win));
MPI_CALL(XBT_PUBLIC int, MPI_Get_accumulate,
         (const void* origin_addr, int origin_count, MPI_Datatype origin_datatype, void* result_addr, int result_count,
          MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, int target_count,
          MPI_Datatype target_datatype, MPI_Op op, MPI_Win win));
MPI_CALL(XBT_PUBLIC int, MPI_Rget,
         (void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
          int target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_Rput,
         (const void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
          int target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_Raccumulate,
         (const void* origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
          int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_Rget_accumulate,
         (const void* origin_addr, int origin_count, MPI_Datatype origin_datatype, void* result_addr, int result_count,
          MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, int target_count,
          MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_Fetch_and_op, (const void* origin_addr, void* result_addr, MPI_Datatype datatype,
                                            int target_rank, MPI_Aint target_disp, MPI_Op op, MPI_Win win));
MPI_CALL(XBT_PUBLIC int, MPI_Compare_and_swap,
         (const void* origin_addr, void* compare_addr, void* result_addr, MPI_Datatype datatype, int target_rank,
          MPI_Aint target_disp, MPI_Win win));

MPI_CALL(XBT_PUBLIC int, MPI_Cart_coords, (MPI_Comm comm, int rank, int maxdims, int* coords));
MPI_CALL(XBT_PUBLIC int, MPI_Cart_create,
         (MPI_Comm comm_old, int ndims, const int* dims, const int* periods, int reorder, MPI_Comm* comm_cart));
MPI_CALL(XBT_PUBLIC int, MPI_Cart_get, (MPI_Comm comm, int maxdims, int* dims, int* periods, int* coords));
MPI_CALL(XBT_PUBLIC int, MPI_Cart_rank, (MPI_Comm comm, const int* coords, int* rank));
MPI_CALL(XBT_PUBLIC int, MPI_Cart_shift, (MPI_Comm comm, int direction, int displ, int* source, int* dest));
MPI_CALL(XBT_PUBLIC int, MPI_Cart_sub, (MPI_Comm comm, const int* remain_dims, MPI_Comm* comm_new));
MPI_CALL(XBT_PUBLIC int, MPI_Cartdim_get, (MPI_Comm comm, int* ndims));
MPI_CALL(XBT_PUBLIC int, MPI_Dims_create, (int nnodes, int ndims, int* dims));
MPI_CALL(XBT_PUBLIC int, MPI_Request_get_status, (MPI_Request request, int* flag, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_Grequest_start,
         (MPI_Grequest_query_function * query_fn, MPI_Grequest_free_function* free_fn,
          MPI_Grequest_cancel_function* cancel_fn, void* extra_state, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_Grequest_complete, (MPI_Request request));
MPI_CALL(XBT_PUBLIC int, MPI_Status_set_cancelled, (MPI_Status * status, int flag));
MPI_CALL(XBT_PUBLIC int, MPI_Status_set_elements, (MPI_Status * status, MPI_Datatype datatype, int count));
MPI_CALL(XBT_PUBLIC int, MPI_Status_set_elements_x, (MPI_Status * status, MPI_Datatype datatype, MPI_Count count));
MPI_CALL(XBT_PUBLIC int, MPI_Type_create_subarray,
         (int ndims, const int* array_of_sizes, const int* array_of_subsizes, const int* array_of_starts, int order, MPI_Datatype oldtype,
          MPI_Datatype* newtype));

MPI_CALL(XBT_PUBLIC int, MPI_File_open, (MPI_Comm comm, const char* filename, int amode, MPI_Info info, MPI_File* fh));
MPI_CALL(XBT_PUBLIC int, MPI_File_close, (MPI_File * fh));
MPI_CALL(XBT_PUBLIC int, MPI_File_delete, (const char* filename, MPI_Info info));
MPI_CALL(XBT_PUBLIC int, MPI_File_get_size, (MPI_File fh, MPI_Offset* size));
MPI_CALL(XBT_PUBLIC int, MPI_File_get_group, (MPI_File fh, MPI_Group* group));
MPI_CALL(XBT_PUBLIC int, MPI_File_get_amode, (MPI_File fh, int* amode));
MPI_CALL(XBT_PUBLIC int, MPI_File_set_info, (MPI_File fh, MPI_Info info));
MPI_CALL(XBT_PUBLIC int, MPI_File_get_info, (MPI_File fh, MPI_Info* info_used));
MPI_CALL(XBT_PUBLIC int, MPI_File_read_at,
         (MPI_File fh, MPI_Offset offset, void* buf, int count, MPI_Datatype datatype, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_File_read_at_all,
         (MPI_File fh, MPI_Offset offset, void* buf, int count, MPI_Datatype datatype, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_File_write_at,
         (MPI_File fh, MPI_Offset offset, const void* buf, int count, MPI_Datatype datatype, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_File_write_at_all,
         (MPI_File fh, MPI_Offset offset, const void* buf, int count, MPI_Datatype datatype, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_File_read, (MPI_File fh, void* buf, int count, MPI_Datatype datatype, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_File_read_all,
         (MPI_File fh, void* buf, int count, MPI_Datatype datatype, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_File_write,
         (MPI_File fh, const void* buf, int count, MPI_Datatype datatype, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_File_write_all,
         (MPI_File fh, const void* buf, int count, MPI_Datatype datatype, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_File_seek, (MPI_File fh, MPI_Offset offset, int whenace));
MPI_CALL(XBT_PUBLIC int, MPI_File_get_position, (MPI_File fh, MPI_Offset* offset));
MPI_CALL(XBT_PUBLIC int, MPI_File_read_shared,
         (MPI_File fh, void* buf, int count, MPI_Datatype datatype, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_File_write_shared,
         (MPI_File fh, const void* buf, int count, MPI_Datatype datatype, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_File_read_ordered,
         (MPI_File fh, void* buf, int count, MPI_Datatype datatype, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_File_write_ordered,
         (MPI_File fh, const void* buf, int count, MPI_Datatype datatype, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_File_seek_shared, (MPI_File fh, MPI_Offset offset, int whence));
MPI_CALL(XBT_PUBLIC int, MPI_File_get_position_shared, (MPI_File fh, MPI_Offset* offset));
MPI_CALL(XBT_PUBLIC int, MPI_File_sync, (MPI_File fh));
MPI_CALL(XBT_PUBLIC int, MPI_File_set_view,
         (MPI_File fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype, const char* datarep, MPI_Info info));
MPI_CALL(XBT_PUBLIC int, MPI_File_get_view,
         (MPI_File fh, MPI_Offset* disp, MPI_Datatype* etype, MPI_Datatype* filetype, char* datarep));


MPI_CALL(XBT_PUBLIC int, MPI_Errhandler_set, (MPI_Comm comm, MPI_Errhandler errhandler));
MPI_CALL(XBT_PUBLIC int, MPI_Errhandler_create, (MPI_Handler_function * function, MPI_Errhandler* errhandler));
MPI_CALL(XBT_PUBLIC int, MPI_Errhandler_free, (MPI_Errhandler * errhandler));
MPI_CALL(XBT_PUBLIC int, MPI_Errhandler_get, (MPI_Comm comm, MPI_Errhandler* errhandler));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_set_errhandler, (MPI_Comm comm, MPI_Errhandler errhandler));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_get_errhandler, (MPI_Comm comm, MPI_Errhandler* errhandler));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_create_errhandler, (MPI_Comm_errhandler_fn * function, MPI_Errhandler* errhandler));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_call_errhandler, (MPI_Comm comm, int errorcode));
MPI_CALL(XBT_PUBLIC int, MPI_Win_set_errhandler, (MPI_Win win, MPI_Errhandler errhandler));
MPI_CALL(XBT_PUBLIC int, MPI_Win_get_errhandler, (MPI_Win win, MPI_Errhandler* errhandler));
MPI_CALL(XBT_PUBLIC int, MPI_Win_create_errhandler, (MPI_Win_errhandler_fn * function, MPI_Errhandler* errhandler));
MPI_CALL(XBT_PUBLIC int, MPI_Win_call_errhandler, (MPI_Win win, int errorcode));
MPI_CALL(XBT_PUBLIC MPI_Errhandler, MPI_Errhandler_f2c, (MPI_Fint errhandler));
MPI_CALL(XBT_PUBLIC MPI_Fint, MPI_Errhandler_c2f, (MPI_Errhandler errhandler));

MPI_CALL(XBT_PUBLIC int, MPI_Type_create_f90_integer, (int count, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC int, MPI_Type_create_f90_real, (int prec, int exp, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC int, MPI_Type_create_f90_complex, (int prec, int exp, MPI_Datatype* newtype));

MPI_CALL(XBT_PUBLIC int, MPI_Type_get_contents,
         (MPI_Datatype datatype, int max_integers, int max_addresses, int max_datatypes, int* array_of_integers,
          MPI_Aint* array_of_addresses, MPI_Datatype* array_of_datatypes));
MPI_CALL(XBT_PUBLIC int, MPI_Type_get_envelope,
         (MPI_Datatype datatype, int* num_integers, int* num_addresses, int* num_datatypes, int* combiner));
MPI_CALL(XBT_PUBLIC int, MPI_File_call_errhandler, (MPI_File fh, int errorcode));
MPI_CALL(XBT_PUBLIC int, MPI_File_create_errhandler,
         (MPI_File_errhandler_function * function, MPI_Errhandler* errhandler));
MPI_CALL(XBT_PUBLIC int, MPI_File_set_errhandler, (MPI_File file, MPI_Errhandler errhandler));
MPI_CALL(XBT_PUBLIC int, MPI_File_get_errhandler, (MPI_File file, MPI_Errhandler* errhandler));
//FIXME: these are not yet implemented


typedef void* MPI_Message;
#define MPI_MESSAGE_NULL NULL
#define MPI_MESSAGE_NO_PROC NULL

#define MPI_DUP_FN ((void*) 1)
#define MPI_CONVERSION_FN_NULL NULL

#define MPI_F_STATUS_IGNORE 0
#define MPI_F_STATUSES_IGNORE NULL

#define MPI_WIN_DUP_FN ((MPI_Win_copy_attr_function*)MPI_DUP_FN)
#define MPI_TYPE_DUP_FN ((MPI_Type_copy_attr_function*)MPI_DUP_FN)
#define MPI_COMM_DUP_FN  ((MPI_Comm_copy_attr_function *)MPI_DUP_FN)
#define MPI_INFO_ENV smpi_process_info_env()
extern SMPI_Datatype smpi_MPI_PACKED;
#define MPI_PACKED SMPI_PREDEFINED_POINTER(MPI_Datatype, smpi_MPI_PACKED)


MPI_CALL(XBT_PUBLIC int, MPI_Cart_map, (MPI_Comm comm_old, int ndims, const int* dims, const int* periods, int* newrank));
MPI_CALL(XBT_PUBLIC int, MPI_Graph_create,
         (MPI_Comm comm_old, int nnodes, const int* index, const int* edges, int reorder, MPI_Comm* comm_graph));
MPI_CALL(XBT_PUBLIC int, MPI_Graph_get, (MPI_Comm comm, int maxindex, int maxedges, int* index, int* edges));
MPI_CALL(XBT_PUBLIC int, MPI_Graph_map, (MPI_Comm comm_old, int nnodes, const int* index, const int* edges, int* newrank));
MPI_CALL(XBT_PUBLIC int, MPI_Graph_neighbors, (MPI_Comm comm, int rank, int maxneighbors, int* neighbors));
MPI_CALL(XBT_PUBLIC int, MPI_Graph_neighbors_count, (MPI_Comm comm, int rank, int* nneighbors));
MPI_CALL(XBT_PUBLIC int, MPI_Graphdims_get, (MPI_Comm comm, int* nnodes, int* nedges));
MPI_CALL(XBT_PUBLIC int, MPI_Topo_test, (MPI_Comm comm, int* top_type));
MPI_CALL(XBT_PUBLIC int, MPI_Add_error_class, (int* errorclass));
MPI_CALL(XBT_PUBLIC int, MPI_Add_error_code, (int errorclass, int* errorcode));
MPI_CALL(XBT_PUBLIC int, MPI_Add_error_string, (int errorcode, char* string));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_test_inter, (MPI_Comm comm, int* flag));
MPI_CALL(XBT_PUBLIC int, MPI_Intercomm_create,
         (MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm, int remote_leader, int tag, MPI_Comm* comm_out));
MPI_CALL(XBT_PUBLIC int, MPI_Intercomm_merge, (MPI_Comm comm, int high, MPI_Comm* comm_out));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_remote_group, (MPI_Comm comm, MPI_Group* group));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_remote_size, (MPI_Comm comm, int* size));
MPI_CALL(XBT_PUBLIC int, MPI_Get_elements, (MPI_Status * status, MPI_Datatype datatype, int* elements));
MPI_CALL(XBT_PUBLIC int, MPI_Get_elements_x, (MPI_Status * status, MPI_Datatype datatype, MPI_Count* elements));
MPI_CALL(XBT_PUBLIC int, MPI_Pcontrol, (const int level, ...));
MPI_CALL(XBT_PUBLIC int, MPI_Type_create_darray,
         (int size, int rank, int ndims, int* array_of_gsizes, int* array_of_distribs, int* array_of_dargs,
          int* array_of_psizes, int order, MPI_Datatype oldtype, MPI_Datatype* newtype));
MPI_CALL(XBT_PUBLIC int, MPI_Pack_external_size, (char* datarep, int incount, MPI_Datatype datatype, MPI_Aint* size));
MPI_CALL(XBT_PUBLIC int, MPI_Pack_external, (char* datarep, void* inbuf, int incount, MPI_Datatype datatype,
                                             void* outbuf, MPI_Aint outcount, MPI_Aint* position));
MPI_CALL(XBT_PUBLIC int, MPI_Unpack_external, (char* datarep, void* inbuf, MPI_Aint insize, MPI_Aint* position,
                                               void* outbuf, int outcount, MPI_Datatype datatype));
MPI_CALL(XBT_PUBLIC int, MPI_Type_match_size, (int typeclass, int size, MPI_Datatype* datatype));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_connect,
         (const char* port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm* newcomm));
MPI_CALL(XBT_PUBLIC int, MPI_Unpublish_name, (char* service_name, MPI_Info info, char* port_name));
MPI_CALL(XBT_PUBLIC int, MPI_Publish_name, (char* service_name, MPI_Info info, char* port_name));
MPI_CALL(XBT_PUBLIC int, MPI_Lookup_name, (char* service_name, MPI_Info info, char* port_name));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_idup, (MPI_Comm comm, MPI_Comm* newcomm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_join, (int fd, MPI_Comm* intercomm));
MPI_CALL(XBT_PUBLIC int, MPI_Open_port, (MPI_Info info, char* port_name));
MPI_CALL(XBT_PUBLIC int, MPI_Close_port, (const char* port_name));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_accept, (const char* port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm* newcomm));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_spawn, (const char* command, char** argv, int maxprocs, MPI_Info info, int root,
                                          MPI_Comm comm, MPI_Comm* intercomm, int* array_of_errcodes));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_spawn_multiple,
         (int count, char** array_of_commands, char*** array_of_argv, int* array_of_maxprocs, MPI_Info* array_of_info,
          int root, MPI_Comm comm, MPI_Comm* intercomm, int* array_of_errcodes));
MPI_CALL(XBT_PUBLIC int, MPI_Comm_get_parent, (MPI_Comm * parent));
MPI_CALL(XBT_PUBLIC int, MPI_Dist_graph_create, (MPI_Comm comm_old, int n, const int* sources, const int* degrees, const int* destinations,
                          const int* weights, MPI_Info info, int reorder, MPI_Comm* comm_dist_graph));
MPI_CALL(XBT_PUBLIC int, MPI_Dist_graph_create_adjacent, (MPI_Comm comm_old, int indegree, const int* sources, const int* sourceweights,
                          int outdegree, const int* destinations, const int* destweights, MPI_Info info, int reorder, MPI_Comm* comm_dist_graph));
MPI_CALL(XBT_PUBLIC int, MPI_Dist_graph_neighbors,
         (MPI_Comm comm, int maxindegree, int* sources, int* sourceweights, int maxoutdegree, int* destinations,
          int* destweights));
MPI_CALL(XBT_PUBLIC int, MPI_Dist_graph_neighbors_count, (MPI_Comm comm, int *indegree, int *outdegree, int *weighted));

MPI_CALL(XBT_PUBLIC int, MPI_Win_test, (MPI_Win win, int* flag));
MPI_CALL(XBT_PUBLIC MPI_Fint, MPI_File_c2f, (MPI_File file));
MPI_CALL(XBT_PUBLIC MPI_File, MPI_File_f2c, (MPI_Fint file));
MPI_CALL(XBT_PUBLIC int, MPI_Register_datarep, (char* datarep, MPI_Datarep_conversion_function* read_conversion_fn,
                                                MPI_Datarep_conversion_function* write_conversion_fn,
                                                MPI_Datarep_extent_function* dtype_file_extent_fn, void* extra_state));
MPI_CALL(XBT_PUBLIC int, MPI_File_set_size, (MPI_File fh, MPI_Offset size));
MPI_CALL(XBT_PUBLIC int, MPI_File_preallocate, (MPI_File fh, MPI_Offset size));
MPI_CALL(XBT_PUBLIC int, MPI_File_iread_at,
         (MPI_File fh, MPI_Offset offset, void* buf, int count, MPI_Datatype datatype, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_File_iwrite_at,
         (MPI_File fh, MPI_Offset offset, const void* buf, int count, MPI_Datatype datatype, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_File_iread_at_all,
         (MPI_File fh, MPI_Offset offset, void* buf, int count, MPI_Datatype datatype, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_File_iwrite_at_all,
         (MPI_File fh, MPI_Offset offset, const void* buf, int count, MPI_Datatype datatype, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_File_iread,
         (MPI_File fh, void* buf, int count, MPI_Datatype datatype, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_File_iwrite,
         (MPI_File fh, const void* buf, int count, MPI_Datatype datatype, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_File_iread_all,
         (MPI_File fh, void* buf, int count, MPI_Datatype datatype, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_File_iwrite_all,
         (MPI_File fh, const void* buf, int count, MPI_Datatype datatype, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_File_get_byte_offset, (MPI_File fh, MPI_Offset offset, MPI_Offset* disp));
MPI_CALL(XBT_PUBLIC int, MPI_File_iread_shared,
         (MPI_File fh, void* buf, int count, MPI_Datatype datatype, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_File_iwrite_shared,
         (MPI_File fh, const void* buf, int count, MPI_Datatype datatype, MPI_Request* request));
MPI_CALL(XBT_PUBLIC int, MPI_File_read_at_all_begin,
         (MPI_File fh, MPI_Offset offset, void* buf, int count, MPI_Datatype datatype));
MPI_CALL(XBT_PUBLIC int, MPI_File_read_at_all_end, (MPI_File fh, void* buf, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_File_write_at_all_begin,
         (MPI_File fh, MPI_Offset offset, const void* buf, int count, MPI_Datatype datatype));
MPI_CALL(XBT_PUBLIC int, MPI_File_write_at_all_end, (MPI_File fh, const void* buf, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_File_read_all_begin, (MPI_File fh, void* buf, int count, MPI_Datatype datatype));
MPI_CALL(XBT_PUBLIC int, MPI_File_read_all_end, (MPI_File fh, void* buf, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_File_write_all_begin, (MPI_File fh, const void* buf, int count, MPI_Datatype datatype));
MPI_CALL(XBT_PUBLIC int, MPI_File_write_all_end, (MPI_File fh, const void* buf, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_File_read_ordered_begin, (MPI_File fh, void* buf, int count, MPI_Datatype datatype));
MPI_CALL(XBT_PUBLIC int, MPI_File_read_ordered_end, (MPI_File fh, void* buf, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_File_write_ordered_begin, (MPI_File fh, const void* buf, int count, MPI_Datatype datatype));
MPI_CALL(XBT_PUBLIC int, MPI_File_write_ordered_end, (MPI_File fh, const void* buf, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_File_get_type_extent, (MPI_File fh, MPI_Datatype datatype, MPI_Aint* extent));
MPI_CALL(XBT_PUBLIC int, MPI_File_set_atomicity, (MPI_File fh, int flag));
MPI_CALL(XBT_PUBLIC int, MPI_File_get_atomicity, (MPI_File fh, int* flag));
MPI_CALL(XBT_PUBLIC MPI_Message, MPI_Message_f2c, (MPI_Fint message));
MPI_CALL(XBT_PUBLIC MPI_Fint, MPI_Message_c2f, (MPI_Message message));
MPI_CALL(XBT_PUBLIC int, MPI_Mrecv, (void* buf, int count, MPI_Datatype datatype, MPI_Message* message, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_Mprobe, (int source, int tag, MPI_Comm comm, MPI_Message* message, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_Imrecv, (void* buf, int count, MPI_Datatype datatype, MPI_Message* message, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Improbe, (int source, int tag, MPI_Comm comm, int *flag, MPI_Message* message, MPI_Status* status));
MPI_CALL(XBT_PUBLIC int, MPI_Neighbor_allgather, (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                         int recvcount, MPI_Datatype recvtype, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Neighbor_allgatherv, (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                          const int* recvcounts, const int* displs, MPI_Datatype recvtype, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Neighbor_alltoall, (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                        int recvcount, MPI_Datatype recvtype, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Neighbor_alltoallv,
         (const void* sendbuf, const int* sendcounts, const int* senddisps, MPI_Datatype sendtype, void* recvbuf, const int* recvcounts,
          const int* recvdisps, MPI_Datatype recvtype, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Neighbor_alltoallw,
         (const void* sendbuf, const int* sendcnts, const MPI_Aint* sdispls, const MPI_Datatype* sendtypes, void* recvbuf, const int* recvcnts,
          const MPI_Aint* rdispls, const MPI_Datatype* recvtypes, MPI_Comm comm));
MPI_CALL(XBT_PUBLIC int, MPI_Ineighbor_allgather, (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                         int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Ineighbor_allgatherv, (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                          const int* recvcounts, const int* displs, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Ineighbor_alltoall, (const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                        int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Ineighbor_alltoallv,
         (const void* sendbuf, const int* sendcounts, const int* senddisps, MPI_Datatype sendtype, void* recvbuf, const int* recvcounts,
          const int* recvdisps, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Ineighbor_alltoallw,
         (const void* sendbuf, const int* sendcounts, const MPI_Aint* senddisps, const MPI_Datatype* sendtypes, void* recvbuf, const int* recvcounts,
          const MPI_Aint* recvdisps, const MPI_Datatype* recvtypes, MPI_Comm comm, MPI_Request *request));
MPI_CALL(XBT_PUBLIC int, MPI_Status_f2c, (MPI_Fint *f_status, MPI_Status *c_status));
MPI_CALL(XBT_PUBLIC int, MPI_Status_c2f, (MPI_Status *c_status, MPI_Fint *f_status));



//FIXME: End of all the not yet implemented stuff

// smpi functions
XBT_PUBLIC int smpi_global_size();
XBT_PUBLIC MPI_Comm smpi_process_comm_self();
XBT_PUBLIC MPI_Info smpi_process_info_env();
XBT_PUBLIC void* smpi_process_get_user_data();
XBT_PUBLIC void smpi_process_set_user_data(void*);
XBT_PUBLIC void smpi_init_options();

XBT_PUBLIC void smpi_execute_flops(double flops);
XBT_PUBLIC void smpi_execute_flops_benched(double flops);
XBT_PUBLIC void smpi_execute(double duration);
XBT_PUBLIC void smpi_execute_benched(double duration);

XBT_PUBLIC void smpi_bench_begin();
XBT_PUBLIC void smpi_bench_end();

XBT_PUBLIC unsigned long long smpi_rastro_resolution();
XBT_PUBLIC unsigned long long smpi_rastro_timestamp();
XBT_PUBLIC void smpi_sample_1(int global, const char* file, const char* tag, int iters, double threshold);
XBT_PUBLIC int smpi_sample_2(int global, const char* file, const char* tag, int iter_count);
XBT_PUBLIC void smpi_sample_3(int global, const char* file, const char* tag);
XBT_PUBLIC int smpi_sample_exit(int global, const char* file, const char* tag, int iter_count);
/**
 * Need a public setter for SMPI copy_callback function, so users can define
 * their own while still using default copy callback for S4U copies.
 */
XBT_PUBLIC void smpi_comm_set_copy_data_callback(void (*callback)(smx_activity_t, void*, size_t));

/**
 * Functions for call location tracing. These functions will be
 * called from the user's application! (With the __FILE__ and __LINE__ values
 * passed as parameters.)
 */
XBT_PUBLIC void smpi_trace_set_call_location(const char* file, int line);
/** Fortran binding **/
XBT_PUBLIC void smpi_trace_set_call_location_(const char* file, const int* line);
/** Fortran binding + -fsecond-underscore **/
XBT_PUBLIC void smpi_trace_set_call_location__(const char* file, const int* line);

#define SMPI_ITER_NAME1(line) _XBT_CONCAT(iter_count, line)
#define SMPI_ITER_NAME(line) SMPI_ITER_NAME1(line)
#define SMPI_CTAG_NAME1(line) _XBT_CONCAT(ctag, line)
#define SMPI_CTAG_NAME(line) SMPI_CTAG_NAME1(line)

#define SMPI_SAMPLE_LOOP(loop_init, loop_end, loop_iter, global, iters, thres, tag)                                    \
  char SMPI_CTAG_NAME(__LINE__) [132];                                                                                 \
  snprintf( SMPI_CTAG_NAME(__LINE__), 132, "%s%d", tag, __LINE__);                                                           \
  int SMPI_ITER_NAME(__LINE__) = 0;                                                                                    \
  {                                                                                                                    \
    loop_init;                                                                                                         \
    while (loop_end) {                                                                                                 \
      SMPI_ITER_NAME(__LINE__)++;                                                                                      \
      (loop_iter);                                                                                                     \
    }                                                                                                                  \
  }                                                                                                                    \
  for ( loop_init;                                                                                                     \
         (loop_end) ? (smpi_sample_1((global), __FILE__, SMPI_CTAG_NAME(__LINE__), (iters), (thres))                   \
                        , (smpi_sample_2((global), __FILE__, SMPI_CTAG_NAME(__LINE__), SMPI_ITER_NAME(__LINE__))))     \
                    : smpi_sample_exit((global), __FILE__, SMPI_CTAG_NAME(__LINE__), SMPI_ITER_NAME(__LINE__));        \
         smpi_sample_3((global), __FILE__, SMPI_CTAG_NAME(__LINE__)), (loop_iter) )

#define SMPI_SAMPLE_LOCAL(loop_init, loop_end, loop_iter, iters, thres)                                                \
  SMPI_SAMPLE_LOOP(loop_init, (loop_end), (loop_iter), 0, (iters), (thres), "")
#define SMPI_SAMPLE_LOCAL_TAG(loop_init, loop_end, loop_iter, iters, thres, tag)                                       \
  SMPI_SAMPLE_LOOP(loop_init, (loop_end), (loop_iter), 0, (iters), (thres), tag)
#define SMPI_SAMPLE_GLOBAL(loop_init, loop_end, loop_iter, iters, thres)                                               \
  SMPI_SAMPLE_LOOP(loop_init, (loop_end), (loop_iter), 1, (iters), (thres), "")
#define SMPI_SAMPLE_GLOBAL_TAG(loop_init, loop_end, loop_iter, iters, thres, tag)                                      \
  SMPI_SAMPLE_LOOP(loop_init, (loop_end), (loop_iter), 1, (iters), (thres), tag)
#define SMPI_SAMPLE_DELAY(duration) for(smpi_execute(duration); 0; )
#define SMPI_SAMPLE_FLOPS(flops) for(smpi_execute_flops(flops); 0; )
XBT_PUBLIC void* smpi_shared_malloc(size_t size, const char* file, int line);
#define SMPI_SHARED_MALLOC(size) smpi_shared_malloc((size), __FILE__, __LINE__)
XBT_PUBLIC void* smpi_shared_malloc_partial(size_t size, const size_t* shared_block_offsets, int nb_shared_blocks);
#define SMPI_PARTIAL_SHARED_MALLOC(size, shared_block_offsets, nb_shared_blocks)                                       \
  smpi_shared_malloc_partial((size), (shared_block_offsets), (nb_shared_blocks))


#define SMPI_SHARED_FREE(data) smpi_shared_free(data)

XBT_PUBLIC int smpi_shared_known_call(const char* func, const char* input);
XBT_PUBLIC void* smpi_shared_get_call(const char* func, const char* input);
XBT_PUBLIC void* smpi_shared_set_call(const char* func, const char* input, void* data);
#define SMPI_SHARED_CALL(func, input, ...)                                                                             \
  (smpi_shared_known_call(_XBT_STRINGIFY(func), (input))                                                               \
       ? smpi_shared_get_call(_XBT_STRINGIFY(func), (input))                                                           \
       : smpi_shared_set_call(_XBT_STRINGIFY(func), (input), ((func)(__VA_ARGS__))))

/* Fortran specific stuff */

XBT_PUBLIC int smpi_main(const char* program, int argc, char* argv[]);

/* Trace replay specific stuff */
XBT_PUBLIC void smpi_replay_init(const char* instance_id, int rank, double start_delay_flops); // Only initialization
XBT_PUBLIC void smpi_replay_main(int rank, const char* private_trace_filename); // Launch the replay once init is done
XBT_PUBLIC void smpi_replay_run(const char* instance_id, int rank, double start_delay_flops,
                                const char* private_trace_filename); // Both init and start

XBT_PUBLIC void SMPI_app_instance_register(const char* name, xbt_main_func_t code, int num_processes);
XBT_PUBLIC void SMPI_init();
XBT_PUBLIC void SMPI_finalize();
XBT_PUBLIC void SMPI_thread_create();

SG_END_DECL

/* C++ declarations for shared_malloc and default copy buffer callback */
#ifdef __cplusplus
XBT_PUBLIC int smpi_is_shared(const void* ptr, std::vector<std::pair<size_t, size_t>>& private_blocks, size_t* offset);

std::vector<std::pair<size_t, size_t>> shift_and_frame_private_blocks(const std::vector<std::pair<size_t, size_t>>& vec,
                                                                      size_t offset, size_t buff_size);
std::vector<std::pair<size_t, size_t>> merge_private_blocks(const std::vector<std::pair<size_t, size_t>>& src,
                                                            const std::vector<std::pair<size_t, size_t>>& dst);

/* May be used by S4U simulations to manually initialize SMPI */
XBT_PUBLIC void smpi_comm_copy_buffer_callback(simgrid::kernel::activity::CommImpl* comm, void* buff,
                                                size_t buff_size);

/**
 * @brief Callback to set cost for SMPI operations (send, recv, isend)
 *
 * This callback replaces the configuration parameters smpi/or, smpi/os, smpi/ois.
 * It offers more flexibility for cost functions.
 *
 * @param size Size of message being received/sent
 * @param source Source host
 * @param dst Destination host
 */
using SmpiOpCostCb = std::function<double(size_t size, simgrid::s4u::Host* source, simgrid::s4u::Host* dst)>;
/** @brief SMPI functions that accept cost functions */
enum class SmpiOperation { RECV = 2, SEND = 1, ISEND = 0 };
/**
 * @brief Register a cost callback for some SMPI function (MPI_Send, MPI_ISend or MPI_Recv)
 *
 * @param op SMPI function
 * @param cb User's callback
 */
XBT_PUBLIC void smpi_register_op_cost_callback(SmpiOperation op, const SmpiOpCostCb& cb);
#endif

#endif

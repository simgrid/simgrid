#include "colls_private.h"
#ifndef REDUCE_STUFF
#define REDUCE_STUFF
/*****************************************************************************

Copyright (c) 2006, Ahmad Faraj & Xin Yuan,
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

  * Neither the name of the Florida State University nor the names of its
    contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  *************************************************************************
  *     Any results obtained from executing this software require the     *
  *     acknowledgment and citation of the software and its owners.       *
  *     The full citation is given below:                                 *
  *                                                                       *
  *     A. Faraj and X. Yuan. "Automatic Generation and Tuning of MPI     *
  *     Collective Communication Routines." The 19th ACM International    *
  *     Conference on Supercomputing (ICS), Cambridge, Massachusetts,     *
  *     June 20-22, 2005.                                                 *
  *************************************************************************

*****************************************************************************/

extern MPI_User_function *MPIR_Op_table[];


/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: mpich-stuff.h,v 1.1 2005/08/22 19:50:21 faraj Exp $
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef _MPICH_STUFF_H
#define _MPICH_STUFF_H

/*TOpaqOverview.tex
  MPI Opaque Objects:

  MPI Opaque objects such as 'MPI_Comm' or 'MPI_Datatype' are specified by 
  integers (in the MPICH2 implementation); the MPI standard calls these
  handles.  
  Out of range values are invalid; the value 0 is reserved.
  For most (with the possible exception of 
  'MPI_Request' for performance reasons) MPI Opaque objects, the integer
  encodes both the kind of object (allowing runtime tests to detect a datatype
  passed where a communicator is expected) and important properties of the 
  object.  Even the 'MPI_xxx_NULL' values should be encoded so that 
  different null handles can be distinguished.  The details of the encoding
  of the handles is covered in more detail in the MPICH2 Design Document.
  For the most part, the ADI uses pointers to the underlying structures
  rather than the handles themselves.  However, each structure contains an 
  'handle' field that is the corresponding integer handle for the MPI object.

  MPID objects (objects used within the implementation of MPI) are not opaque.

  T*/

/* Known MPI object types.  These are used for both the error handlers 
   and for the handles.  This is a 4 bit value.  0 is reserved for so 
   that all-zero handles can be flagged as an error. */
/*E
  MPID_Object_kind - Object kind (communicator, window, or file)

  Notes:
  This enum is used by keyvals and errhandlers to indicate the type of
  object for which MPI opaque types the data is valid.  These are defined
  as bits to allow future expansion to the case where an object is value for
  multiple types (for example, we may want a universal error handler for 
  errors return).  This is also used to indicate the type of MPI object a 
  MPI handle represents.  It is an enum because only this applies only the
  the MPI objects.

  Module:
  Attribute-DS
  E*/
typedef enum MPID_Object_kind {
  MPID_COMM = 0x1,
  MPID_GROUP = 0x2,
  MPID_DATATYPE = 0x3,
  MPID_FILE = 0x4,
  MPID_ERRHANDLER = 0x5,
  MPID_OP = 0x6,
  MPID_INFO = 0x7,
  MPID_WIN = 0x8,
  MPID_KEYVAL = 0x9,
  MPID_ATTR = 0xa,
  MPID_REQUEST = 0xb
} MPID_Object_kind;
/* The above objects should correspond to MPI objects only. */
#define HANDLE_MPI_KIND_SHIFT 26
#define HANDLE_GET_MPI_KIND(a) ( ((a)&0x3c000000) >> HANDLE_MPI_KIND_SHIFT )

/* Handle types.  These are really 2 bits */
#define HANDLE_KIND_INVALID  0x0
#define HANDLE_KIND_BUILTIN  0x1
#define HANDLE_KIND_DIRECT   0x2
#define HANDLE_KIND_INDIRECT 0x3
/* Mask assumes that ints are at least 4 bytes */
#define HANDLE_KIND_MASK 0xc0000000
#define HANDLE_KIND_SHIFT 30
#define HANDLE_GET_KIND(a) (((a)&HANDLE_KIND_MASK)>>HANDLE_KIND_SHIFT)
#define HANDLE_SET_KIND(a,kind) ((a)|((kind)<<HANDLE_KIND_SHIFT))

/* For indirect, the remainder of the handle has a block and index */
#define HANDLE_INDIRECT_SHIFT 16
#define HANDLE_BLOCK(a) (((a)& 0x03FF0000) >> HANDLE_INDIRECT_SHIFT)
#define HANDLE_BLOCK_INDEX(a) ((a) & 0x0000FFFF)

/* Handle block is between 1 and 1024 *elements* */
#define HANDLE_BLOCK_SIZE 256
/* Index size is bewtween 1 and 65536 *elements* */
#define HANDLE_BLOCK_INDEX_SIZE 1024

/* For direct, the remainder of the handle is the index into a predefined 
   block */
#define HANDLE_MASK 0x03FFFFFF
#define HANDLE_INDEX(a) ((a)& HANDLE_MASK)

/* ALL objects have the handle as the first value. */
/* Inactive (unused and stored on the appropriate avail list) objects 
   have MPIU_Handle_common as the head */
typedef struct MPIU_Handle_common {
  int handle;
  volatile int ref_count;       /* This field is used to indicate that the
                                   object is not in use (see, e.g., 
                                   MPID_Comm_valid_ptr) */
  void *next;                   /* Free handles use this field to point to the next
                                   free object */
} MPIU_Handle_common;

/* All *active* (in use) objects have the handle as the first value; objects
   with referene counts have the reference count as the second value.
   See MPIU_Object_add_ref and MPIU_Object_release_ref. */
typedef struct MPIU_Handle_head {
  int handle;
  volatile int ref_count;
} MPIU_Handle_head;

/* This type contains all of the data, except for the direct array,
   used by the object allocators. */
typedef struct MPIU_Object_alloc_t {
  MPIU_Handle_common *avail;    /* Next available object */
  int initialized;              /* */
  void *(*indirect)[];          /* Pointer to indirect object blocks */
  int indirect_size;            /* Number of allocated indirect blocks */
  MPID_Object_kind kind;        /* Kind of object this is for */
  int size;                     /* Size of an individual object */
  void *direct;                 /* Pointer to direct block, used 
                                   for allocation */
  int direct_size;              /* Size of direct block */
} MPIU_Object_alloc_t;
extern void *MPIU_Handle_obj_alloc(MPIU_Object_alloc_t *);
extern void MPIU_Handle_obj_alloc_start(MPIU_Object_alloc_t *);
extern void MPIU_Handle_obj_alloc_complete(MPIU_Object_alloc_t *, int init);
extern void MPIU_Handle_obj_free(MPIU_Object_alloc_t *, void *);
void *MPIU_Handle_get_ptr_indirect(int, MPIU_Object_alloc_t *);
extern void *MPIU_Handle_direct_init(void *direct, int direct_size,
                                     int obj_size, int handle_type);
#endif
#define MPID_Getb_ptr(kind,a,bmsk,ptr)                                  \
{                                                                       \
   switch (HANDLE_GET_KIND(a)) {                                        \
      case HANDLE_KIND_BUILTIN:                                         \
          ptr=MPID_##kind##_builtin+((a)&(bmsk));                       \
          break;                                                        \
      case HANDLE_KIND_DIRECT:                                          \
          ptr=MPID_##kind##_direct+HANDLE_INDEX(a);                     \
          break;                                                        \
      case HANDLE_KIND_INDIRECT:                                        \
          ptr=((MPID_##kind*)                                           \
               MPIU_Handle_get_ptr_indirect(a,&MPID_##kind##_mem));     \
          break;                                                        \
      case HANDLE_KIND_INVALID:                                         \
      default:								\
          ptr=0;							\
          break;							\
    }                                                                   \
}



#define MPID_Op_get_ptr(a,ptr)         MPID_Getb_ptr(Op,a,0x000000ff,ptr)
typedef enum MPID_Lang_t { MPID_LANG_C
#ifdef HAVE_FORTRAN_BINDING
      , MPID_LANG_FORTRAN, MPID_LANG_FORTRAN90
#endif
#ifdef HAVE_CXX_BINDING
      , MPID_LANG_CXX
#endif
} MPID_Lang_t;
/* Reduction and accumulate operations */
/*E
  MPID_Op_kind - Enumerates types of MPI_Op types

  Notes:
  These are needed for implementing 'MPI_Accumulate', since only predefined
  operations are allowed for that operation.  

  A gap in the enum values was made allow additional predefined operations
  to be inserted.  This might include future additions to MPI or experimental
  extensions (such as a Read-Modify-Write operation).

  Module:
  Collective-DS
  E*/
typedef enum MPID_Op_kind { MPID_OP_MAX = 1, MPID_OP_MIN = 2,
  MPID_OP_SUM = 3, MPID_OP_PROD = 4,
  MPID_OP_LAND = 5, MPID_OP_BAND = 6, MPID_OP_LOR = 7, MPID_OP_BOR = 8,
  MPID_OP_LXOR = 9, MPID_OP_BXOR = 10, MPID_OP_MAXLOC = 11,
  MPID_OP_MINLOC = 12, MPID_OP_REPLACE = 13,
  MPID_OP_USER_NONCOMMUTE = 32, MPID_OP_USER = 33
} MPID_Op_kind;

/*S
  MPID_User_function - Definition of a user function for MPI_Op types.

  Notes:
  This includes a 'const' to make clear which is the 'in' argument and 
  which the 'inout' argument, and to indicate that the 'count' and 'datatype'
  arguments are unchanged (they are addresses in an attempt to allow 
  interoperation with Fortran).  It includes 'restrict' to emphasize that 
  no overlapping operations are allowed.

  We need to include a Fortran version, since those arguments will
  have type 'MPI_Fint *' instead.  We also need to add a test to the
  test suite for this case; in fact, we need tests for each of the handle
  types to ensure that the transfered handle works correctly.

  This is part of the collective module because user-defined operations
  are valid only for the collective computation routines and not for 
  RMA accumulate.

  Yes, the 'restrict' is in the correct location.  C compilers that 
  support 'restrict' should be able to generate code that is as good as a
  Fortran compiler would for these functions.

  We should note on the manual pages for user-defined operations that
  'restrict' should be used when available, and that a cast may be 
  required when passing such a function to 'MPI_Op_create'.

  Question:
  Should each of these function types have an associated typedef?

  Should there be a C++ function here?

  Module:
  Collective-DS
  S*/
typedef union MPID_User_function {
  void (*c_function) (const void *, void *, const int *, const MPI_Datatype *);
  void (*f77_function) (const void *, void *,
                        const MPI_Fint *, const MPI_Fint *);
} MPID_User_function;
/* FIXME: Should there be "restrict" in the definitions above, e.g., 
   (*c_function)( const void restrict * , void restrict *, ... )? */

/*S
  MPID_Op - MPI_Op structure

  Notes:
  All of the predefined functions are commutative.  Only user functions may 
  be noncummutative, so there are two separate op types for commutative and
  non-commutative user-defined operations.

  Operations do not require reference counts because there are no nonblocking
  operations that accept user-defined operations.  Thus, there is no way that
  a valid program can free an 'MPI_Op' while it is in use.

  Module:
  Collective-DS
  S*/
typedef struct MPID_Op {
  int handle;                   /* value of MPI_Op for this structure */
  volatile int ref_count;
  MPID_Op_kind kind;
  MPID_Lang_t language;
  MPID_User_function function;
} MPID_Op;
#define MPID_OP_N_BUILTIN 14
extern MPID_Op MPID_Op_builtin[MPID_OP_N_BUILTIN];
extern MPID_Op MPID_Op_direct[];
extern MPIU_Object_alloc_t MPID_Op_mem;

/*****************************************************************************

* Function: get_op_func

* return: Pointer to MPI_User_function

* inputs:
   op: operator (max, min, etc)

   * Descrp: Function returns the function associated with current operator
   * op.

   * Auther: AHMAD FARAJ

****************************************************************************/
MPI_User_function *get_op_func(MPI_Op op)
{

  if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN)
    return MPIR_Op_table[op % 16 - 1];
  return NULL;
}

#endif


int smpi_coll_tuned_allreduce_rab_reduce_scatter(void *sbuff, void *rbuff,
                                                 int count, MPI_Datatype dtype,
                                                 MPI_Op op, MPI_Comm comm)
{
  int nprocs, rank, type_size, tag = 543;
  int mask, dst, pof2, newrank, rem, newdst, i,
      send_idx, recv_idx, last_idx, send_cnt, recv_cnt, *cnts, *disps;
  MPI_Aint lb, extent;
  MPI_Status status;
  void *tmp_buf = NULL;
  MPI_User_function *func = get_op_func(op);
  nprocs = smpi_comm_size(comm);
  rank = smpi_comm_rank(comm);

  extent = smpi_datatype_get_extent(dtype);
  tmp_buf = (void *) xbt_malloc(count * extent);

  MPIR_Localcopy(sbuff, count, dtype, rbuff, count, dtype);

  type_size = smpi_datatype_size(dtype);

  // find nearest power-of-two less than or equal to comm_size
  pof2 = 1;
  while (pof2 <= nprocs)
    pof2 <<= 1;
  pof2 >>= 1;

  rem = nprocs - pof2;

  // In the non-power-of-two case, all even-numbered
  // processes of rank < 2*rem send their data to
  // (rank+1). These even-numbered processes no longer
  // participate in the algorithm until the very end. The
  // remaining processes form a nice power-of-two. 

  if (rank < 2 * rem) {
    // even       
    if (rank % 2 == 0) {

      MPIC_Send(rbuff, count, dtype, rank + 1, tag, comm);

      // temporarily set the rank to -1 so that this
      // process does not pariticipate in recursive
      // doubling
      newrank = -1;
    } else                      // odd
    {
      MPIC_Recv(tmp_buf, count, dtype, rank - 1, tag, comm, &status);
      // do the reduction on received data. since the
      // ordering is right, it doesn't matter whether
      // the operation is commutative or not.
      (*func) (tmp_buf, rbuff, &count, &dtype);

      // change the rank 
      newrank = rank / 2;
    }
  }

  else                          // rank >= 2 * rem 
    newrank = rank - rem;

  // If op is user-defined or count is less than pof2, use
  // recursive doubling algorithm. Otherwise do a reduce-scatter
  // followed by allgather. (If op is user-defined,
  // derived datatypes are allowed and the user could pass basic
  // datatypes on one process and derived on another as long as
  // the type maps are the same. Breaking up derived
  // datatypes to do the reduce-scatter is tricky, therefore
  // using recursive doubling in that case.) 

  if (newrank != -1) {
    // do a reduce-scatter followed by allgather. for the
    // reduce-scatter, calculate the count that each process receives
    // and the displacement within the buffer 

    cnts = (int *) xbt_malloc(pof2 * sizeof(int));
    disps = (int *) xbt_malloc(pof2 * sizeof(int));

    for (i = 0; i < (pof2 - 1); i++)
      cnts[i] = count / pof2;
    cnts[pof2 - 1] = count - (count / pof2) * (pof2 - 1);

    disps[0] = 0;
    for (i = 1; i < pof2; i++)
      disps[i] = disps[i - 1] + cnts[i - 1];

    mask = 0x1;
    send_idx = recv_idx = 0;
    last_idx = pof2;
    while (mask < pof2) {
      newdst = newrank ^ mask;
      // find real rank of dest 
      dst = (newdst < rem) ? newdst * 2 + 1 : newdst + rem;

      send_cnt = recv_cnt = 0;
      if (newrank < newdst) {
        send_idx = recv_idx + pof2 / (mask * 2);
        for (i = send_idx; i < last_idx; i++)
          send_cnt += cnts[i];
        for (i = recv_idx; i < send_idx; i++)
          recv_cnt += cnts[i];
      } else {
        recv_idx = send_idx + pof2 / (mask * 2);
        for (i = send_idx; i < recv_idx; i++)
          send_cnt += cnts[i];
        for (i = recv_idx; i < last_idx; i++)
          recv_cnt += cnts[i];
      }

      // Send data from recvbuf. Recv into tmp_buf 
      MPIC_Sendrecv((char *) rbuff + disps[send_idx] * extent, send_cnt,
                    dtype, dst, tag,
                    (char *) tmp_buf + disps[recv_idx] * extent, recv_cnt,
                    dtype, dst, tag, comm, &status);

      // tmp_buf contains data received in this step.
      // recvbuf contains data accumulated so far 

      // This algorithm is used only for predefined ops
      // and predefined ops are always commutative.
      (*func) ((char *) tmp_buf + disps[recv_idx] * extent,
               (char *) rbuff + disps[recv_idx] * extent, &recv_cnt, &dtype);

      // update send_idx for next iteration 
      send_idx = recv_idx;
      mask <<= 1;

      // update last_idx, but not in last iteration because the value
      // is needed in the allgather step below. 
      if (mask < pof2)
        last_idx = recv_idx + pof2 / mask;
    }

    // now do the allgather 

    mask >>= 1;
    while (mask > 0) {
      newdst = newrank ^ mask;
      // find real rank of dest
      dst = (newdst < rem) ? newdst * 2 + 1 : newdst + rem;

      send_cnt = recv_cnt = 0;
      if (newrank < newdst) {
        // update last_idx except on first iteration 
        if (mask != pof2 / 2)
          last_idx = last_idx + pof2 / (mask * 2);

        recv_idx = send_idx + pof2 / (mask * 2);
        for (i = send_idx; i < recv_idx; i++)
          send_cnt += cnts[i];
        for (i = recv_idx; i < last_idx; i++)
          recv_cnt += cnts[i];
      } else {
        recv_idx = send_idx - pof2 / (mask * 2);
        for (i = send_idx; i < last_idx; i++)
          send_cnt += cnts[i];
        for (i = recv_idx; i < send_idx; i++)
          recv_cnt += cnts[i];
      }

      MPIC_Sendrecv((char *) rbuff + disps[send_idx] * extent, send_cnt,
                    dtype, dst, tag,
                    (char *) rbuff + disps[recv_idx] * extent, recv_cnt,
                    dtype, dst, tag, comm, &status);

      if (newrank > newdst)
        send_idx = recv_idx;

      mask >>= 1;
    }

    free(cnts);
    free(disps);

  }
  // In the non-power-of-two case, all odd-numbered processes of
  // rank < 2 * rem send the result to (rank-1), the ranks who didn't
  // participate above.

  if (rank < 2 * rem) {
    if (rank % 2)               // odd 
      MPIC_Send(rbuff, count, dtype, rank - 1, tag, comm);
    else                        // even 
      MPIC_Recv(rbuff, count, dtype, rank + 1, tag, comm, &status);
  }

  free(tmp_buf);
  return MPI_SUCCESS;
}

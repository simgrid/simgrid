#include "colls.h"

/*
 * created by Pitch Patarasuk
 * Modified by Xin Yuan
 * 
 * realize a subset of MPI predefine operators:
 * MPI_LAND, MPI_BAND: C integer, Fortran integer, Byte
 * MPI_LOR, MPI_BOR: C integer, Fortran integer, Byte
 * MPI_LXOR, MPI_BXOR: C integer, Fortran integer, Byte
 * MPI_SUM, MPI_PROD: C integer, Fortran integer, Floating point
 * MPI_MIN, MPI_MAX: C integer, Fortran integer, Floating point, Byte
 *
 * Types not implemented: MPI_LONG_DOUBLE, MPI_LOGICAL, MPI_COMPLEX
 */

#ifndef STAR_REDUCTION
#define STAR_REDUCTION


#ifdef MPICH2_REDUCTION
extern MPI_User_function * MPIR_Op_table[];
#elif defined MVAPICH_REDUCETION
extern void *MPIR_ToPointer();
struct MPIR_OP
{
  MPI_User_function *op;
  int               commute;
  int               permanent;
};
#endif

static void star_generic_reduction(MPI_Op op, void *src, void *target, int *count, MPI_Datatype *dtype){
  int i;
  if ((op == MPI_BOR) || (op == MPI_LOR)) {
    if ((*dtype == MPI_BYTE) || (*dtype == MPI_CHAR)) {
      for (i=0;i<*count;i++) {
          ((char *)target)[i] |= ((char *)src)[i];
      }
    }
    else if ((*dtype == MPI_INT) 
	     || (*dtype == MPI_LONG) 
	     || (*dtype == MPI_INT) 
             || (*dtype == MPI_UNSIGNED) 
	     || (*dtype == MPI_UNSIGNED_LONG)) {
      for (i=0;i<*count;i++) {
          ((int *)target)[i] |= ((int *)src)[i];
      }
    } 
    else if ((*dtype == MPI_SHORT) 
	     || (*dtype == MPI_UNSIGNED_SHORT)) { 
      for (i=0;i<*count;i++) {
          ((short *)target)[i] |= ((short *)src)[i];
      }
    } 
    else {
      printf("reduction operation not supported\n");
    }
  }

  else if ((op == MPI_BAND) || (op == MPI_LAND)) {
    if ((*dtype == MPI_BYTE) || (*dtype == MPI_CHAR)) {
      for (i=0;i<*count;i++) {
          ((char *)target)[i] &= ((char *)src)[i];
      }
    }
    else if ((*dtype == MPI_INT) 
	     || (*dtype == MPI_LONG) 
             || (*dtype == MPI_UNSIGNED) 
	     || (*dtype == MPI_UNSIGNED_LONG)) {
      for (i=0;i<*count;i++) {
          ((int *)target)[i] &= ((int *)src)[i];
      }
    } 
    else if ((*dtype == MPI_SHORT) 
	     || (*dtype == MPI_UNSIGNED_SHORT)) { 
      for (i=0;i<*count;i++) {
          ((short *)target)[i] &= ((short *)src)[i];
      }
    } 
    else {
      printf("reduction operation not supported\n");
    }
  }


  else if ((op == MPI_BXOR) || (op == MPI_LXOR)) {
    if ((*dtype == MPI_BYTE) || (*dtype == MPI_CHAR)) {
      for (i=0;i<*count;i++) {
          ((char *)target)[i] ^= ((char *)src)[i];
      }
    }
    else if ((*dtype == MPI_INT) 
	     || (*dtype == MPI_LONG) 
             || (*dtype == MPI_UNSIGNED) 
	     || (*dtype == MPI_UNSIGNED_LONG)) {
      for (i=0;i<*count;i++) {
          ((int *)target)[i] ^= ((int *)src)[i];
      }
    } 
    else if ((*dtype == MPI_SHORT) 
	     || (*dtype == MPI_UNSIGNED_SHORT)) { 
      for (i=0;i<*count;i++) {
          ((short *)target)[i] ^= ((short *)src)[i];
      }
    } 
    else {
      printf("reduction operation not supported\n");
    }
  }

  else if (op == MPI_MAX) {
    if ((*dtype == MPI_INT) 
	|| (*dtype == MPI_LONG)) { 
      for (i=0;i<*count;i++) {
        if (((int *)src)[i] > ((int *)target)[i]) {
          ((int *)target)[i] = ((int *)src)[i];
        }
      }
    }
    else if ((*dtype == MPI_UNSIGNED) 
	|| (*dtype == MPI_UNSIGNED_LONG)) {
      for (i=0;i<*count;i++) {
        if (((unsigned int *)src)[i] > ((unsigned int *)target)[i]) {
          ((unsigned int *)target)[i] = ((unsigned int *)src)[i];
        }
      }
    }
    else if (*dtype == MPI_SHORT) {
      for (i=0;i<*count;i++) {
        if (((short *)src)[i] > ((short *)target)[i]) {
          ((short *)target)[i] = ((short *)src)[i];
        }
      }
    }
    else if (*dtype == MPI_UNSIGNED_SHORT) { 
      for (i=0;i<*count;i++) {
        if (((unsigned short *)src)[i] > ((unsigned short *)target)[i]) {
          ((unsigned short *)target)[i] = ((unsigned short *)src)[i];
        }
      }
    }

    else if (*dtype == MPI_DOUBLE) {
      for (i=0;i<*count;i++) {
        if (((double *)src)[i] > ((double *)target)[i]) {
          ((double *)target)[i] = ((double *)src)[i];
        }
      }
    }
    else if (*dtype == MPI_FLOAT) {
      for (i=0;i<*count;i++) {
        if (((float *)src)[i] > ((float *)target)[i]) {
          ((float *)target)[i] = ((float *)src)[i];
        }
      }
    }
    else if ((*dtype == MPI_CHAR) || (*dtype == MPI_BYTE)) {
      for (i=0;i<*count;i++) {
        if (((char *)src)[i] > ((char *)target)[i]) {
          ((char *)target)[i] = ((char *)src)[i];
        }
      }
    }
    else {
      printf("reduction operation not supported\n");
    }
  }



  else if (op == MPI_MIN) {
    if ((*dtype == MPI_INT) 
	|| (*dtype == MPI_LONG)) { 
      for (i=0;i<*count;i++) {
        if (((int *)src)[i] < ((int *)target)[i]) {
          ((int *)target)[i] = ((int *)src)[i];
        }
      }
    }
    else if ((*dtype == MPI_UNSIGNED) 
	|| (*dtype == MPI_UNSIGNED_LONG)) {
      for (i=0;i<*count;i++) {
        if (((unsigned int *)src)[i] < ((unsigned int *)target)[i]) {
          ((unsigned int *)target)[i] = ((unsigned int *)src)[i];
        }
      }
    }
    else if (*dtype == MPI_SHORT) {
      for (i=0;i<*count;i++) {
        if (((short *)src)[i] < ((short *)target)[i]) {
          ((short *)target)[i] = ((short *)src)[i];
        }
      }
    }
    else if (*dtype == MPI_UNSIGNED_SHORT) { 
      for (i=0;i<*count;i++) {
        if (((unsigned short *)src)[i] < ((unsigned short *)target)[i]) {
          ((unsigned short *)target)[i] = ((unsigned short *)src)[i];
        }
      }
    }

    else if (*dtype == MPI_DOUBLE) {
      for (i=0;i<*count;i++) {
        if (((double *)src)[i] < ((double *)target)[i]) {
          ((double *)target)[i] = ((double *)src)[i];
        }
      }
    }
    else if (*dtype == MPI_FLOAT) {
      for (i=0;i<*count;i++) {
        if (((float *)src)[i] < ((float *)target)[i]) {
          ((float *)target)[i] = ((float *)src)[i];
        }
      }
    }
    else if ((*dtype == MPI_CHAR) || (*dtype == MPI_BYTE)) {
      for (i=0;i<*count;i++) {
        if (((char *)src)[i] < ((char *)target)[i]) {
          ((char *)target)[i] = ((char *)src)[i];
        }
      }
    }
    else {
      printf("reduction operation not supported\n");
    }
  }


  else if (op == MPI_SUM) {
    if ((*dtype == MPI_INT) 
	|| (*dtype == MPI_LONG)) { 
      for (i=0;i<*count;i++) {
          ((int *)target)[i] += ((int *)src)[i];
      }
    }
    else if ((*dtype == MPI_UNSIGNED) 
	|| (*dtype == MPI_UNSIGNED_LONG)) {
      for (i=0;i<*count;i++) {
          ((unsigned int *)target)[i] += ((unsigned int *)src)[i];
      }
    }
    else if (*dtype == MPI_SHORT) {
      for (i=0;i<*count;i++) {
          ((short *)target)[i] += ((short *)src)[i];
      }
    }
    else if (*dtype == MPI_UNSIGNED_SHORT) { 
      for (i=0;i<*count;i++) {
          ((unsigned short *)target)[i] += ((unsigned short *)src)[i];
      }
    }

    else if (*dtype == MPI_DOUBLE) {
      for (i=0;i<*count;i++) {
          ((double *)target)[i] += ((double *)src)[i];
      }
    }
    else if (*dtype == MPI_FLOAT) {
      for (i=0;i<*count;i++) {
          ((float *)target)[i] += ((float *)src)[i];
      }
    }
    else {
      printf("reduction operation not supported\n");
    }
  }

  else if (op == MPI_PROD) {
    if ((*dtype == MPI_INT) 
	|| (*dtype == MPI_LONG)) { 
      for (i=0;i<*count;i++) {
          ((int *)target)[i] *= ((int *)src)[i];
      }
    }
    else if ((*dtype == MPI_UNSIGNED) 
	|| (*dtype == MPI_UNSIGNED_LONG)) {
      for (i=0;i<*count;i++) {
          ((unsigned int *)target)[i] *= ((unsigned int *)src)[i];
      }
    }
    else if (*dtype == MPI_SHORT) {
      for (i=0;i<*count;i++) {
          ((short *)target)[i] *= ((short *)src)[i];
      }
    }
    else if (*dtype == MPI_UNSIGNED_SHORT) { 
      for (i=0;i<*count;i++) {
          ((unsigned short *)target)[i] *= ((unsigned short *)src)[i];
      }
    }

    else if (*dtype == MPI_DOUBLE) {
      for (i=0;i<*count;i++) {
          ((double *)target)[i] *= ((double *)src)[i];
      }
    }
    else if (*dtype == MPI_FLOAT) {
      for (i=0;i<*count;i++) {
          ((float *)target)[i] *= ((float *)src)[i];
      }
    }
    else {
      printf("reduction operation not supported\n");
    }
  }

  else {
    printf("reduction operation not supported\n");
  }
}

void star_reduction(MPI_Op op, void *src, void *target, int *count, MPI_Datatype *dtype){

#ifdef MPICH2_REDUCTION
MPI_User_function * uop = MPIR_Op_table[op % 16 - 1];
 return (*uop) (src,target,count,dtype);
#elif defined MVAPICH_REDUCTION
MPI_User_function *uop;
struct MPIR_OP *op_ptr;
op_ptr = MPIR_ToPointer(op);
uop  = op_ptr->op;
 return (*uop) (src,target,count,dtype);
#else
 return star_generic_reduction(op,src,target,count,dtype);
#endif



}


#endif

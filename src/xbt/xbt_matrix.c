/* $Id$ */

/* xbt_matrix_t management functions                                        */

/* Copyright (c) 2006 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/matrix.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(matrix,xbt,"2D data storage");

/** \brief constructor */
xbt_matrix_t xbt_matrix_new(int lines, int rows, 
			    const unsigned long elmsize,
			    void_f_pvoid_t * const free_f)  {
   xbt_matrix_t res=xbt_new(s_xbt_matrix_t, 1);
   res->lines   = lines;
   res->rows    = rows;
   res->elmsize = elmsize;
   res->free_f  = free_f;
   res->data    = xbt_malloc(elmsize*lines*rows);
   return res;
}

/** \brief destructor */
void xbt_matrix_free(xbt_matrix_t mat) {
  int i;
  if (mat) {
    if (mat->free_f) {
      for (i=0; i < (mat->lines * mat->rows) ; i++) {
	(*(mat->free_f))( (void*)& (mat->data[i*mat->elmsize]) );
      }
    }
    free(mat->data);
    free(mat);
  }
}

/** \brief Freeing function for containers of xbt_matrix_t */
void xbt_matrix_free_voidp(void *d) {
   xbt_matrix_free( (xbt_matrix_t) *(void**)d );
}


/** \brief Display the content of a matrix (debugging purpose) 
 *  \param coords: boolean indicating whether we should add the coords of each cell to the output*/
void xbt_matrix_dump(xbt_matrix_t matrix, const char*name, int coords,
		     void_f_pvoid_t display_fun) {
  int i,j;

  fprintf(stderr,">>> Matrix %s dump (%d x %d)\n",name,matrix->lines,matrix->rows);
  for (i=0; i<matrix->lines; i++) {
    fprintf(stderr,"  ");
    for (j=0; j<matrix->rows; j++) {
       if (coords)
	 fprintf(stderr," (%d,%d)=",i,j);
       else 
	 fprintf(stderr," ");
       display_fun(xbt_matrix_get_ptr(matrix,i,j));
    }
    fprintf(stderr,"\n");
  }
  fprintf(stderr,"<<< end_of_matrix %s dump\n",name);
}

void xbt_matrix_dump_display_double(void*d) {
  fprintf(stderr,"%.2f",*(double*)d);
}

/** \brief Creates a new matrix of double filled with zeros */
xbt_matrix_t xbt_matrix_double_new_zeros(int lines, int rows) {
  xbt_matrix_t res = xbt_matrix_new(lines, rows,sizeof(double),NULL);
  int i,j;
   
  for (i=0; i<lines; i++) 
    for (j=0; j<rows; j++)
      xbt_matrix_get_as(res,i,j,double) = 0;
  return res;
}

/** \brief Creates a new matrix of double being the identity matrix */
xbt_matrix_t xbt_matrix_double_new_id(int lines, int rows) {
  xbt_matrix_t res = xbt_matrix_double_new_zeros(lines, rows);
  int i;
   
  for (i=0; i<lines; i++) 
    xbt_matrix_get_as(res,i,i,double) = 1;
  return res;
}

/** \brief Creates a new matrix of double randomly filled */
xbt_matrix_t xbt_matrix_double_new_rand(int lines, int rows) {
  xbt_matrix_t res = xbt_matrix_new(lines, rows,sizeof(double),NULL);
  int i,j;
   
  for (i=0; i<lines; i++) 
    for (j=0; j<rows; j++)
      xbt_matrix_get_as(res,i,j,double) = (double)rand();
  return res;
}

/** \brief Creates a new matrix being the multiplication of two others */
xbt_matrix_t xbt_matrix_double_new_mult(xbt_matrix_t A,xbt_matrix_t B) {
  xbt_matrix_t result = xbt_matrix_double_new_zeros(A->lines, B->rows);
  int i,j,k;

  for (i=0; i<result->lines; i++) 
    for (j=0; j<result->rows; j++) 
       for (k=0; k<B->lines; k++) 
	 xbt_matrix_get_as(result,i,j,double) +=
            xbt_matrix_get_as(A,i,k,double) * xbt_matrix_get_as(B,k,j,double);
   
  return result;
}



/* xbt_matrix_t management functions                                        */

/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/matrix.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_matrix, xbt, "2D data storage");

/** \brief constructor */
xbt_matrix_t xbt_matrix_new(int lines, int rows,
                            const unsigned long elmsize,
                            void_f_pvoid_t const free_f)
{
  xbt_matrix_t res = xbt_new(s_xbt_matrix_t, 1);
  res->lines = lines;
  res->rows = rows;
  res->elmsize = elmsize;
  res->free_f = free_f;
  res->data = xbt_malloc(elmsize * lines * rows);
  return res;
}

/** \brief Creates a matrix being a submatrix of another one */
xbt_matrix_t xbt_matrix_new_sub(xbt_matrix_t from,
                                int lsize, int rsize,
                                int lpos, int rpos,
                                pvoid_f_pvoid_t const cpy_f)
{

  xbt_matrix_t res = xbt_matrix_new(lsize, rsize,
                                    from->elmsize, from->free_f);
  xbt_matrix_copy_values(res, from, lsize, rsize, 0, 0, lpos, rpos, cpy_f);
  return res;
}

/** \brief destructor */
void xbt_matrix_free(xbt_matrix_t mat)
{
  unsigned int i;
  if (mat) {
    if (mat->free_f) {
      for (i = 0; i < (mat->lines * mat->rows); i++) {
        (*(mat->free_f)) ((void *) &(mat->data[i * mat->elmsize]));
      }
    }
    free(mat->data);
    free(mat);
  }
}

/** \brief Freeing function for containers of xbt_matrix_t */
void xbt_matrix_free_voidp(void *d)
{
  xbt_matrix_free((xbt_matrix_t) * (void **) d);
}


/** \brief Display the content of a matrix (debugging purpose)
 *  \param coords: boolean indicating whether we should add the coords of each cell to the output*/
void xbt_matrix_dump(xbt_matrix_t matrix, const char *name, int coords,
                     void_f_pvoid_t display_fun)
{
  unsigned int i, j;

  fprintf(stderr, ">>> Matrix %s dump (%d x %d)\n", name, matrix->lines,
          matrix->rows);
  for (i = 0; i < matrix->lines; i++) {
    fprintf(stderr, "  ");
    for (j = 0; j < matrix->rows; j++) {
      if (coords)
        fprintf(stderr, " (%d,%d)=", i, j);
      else
        fprintf(stderr, " ");
      (*display_fun) (xbt_matrix_get_ptr(matrix, i, j));
    }
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "<<< end_of_matrix %s dump\n", name);
}

void xbt_matrix_dump_display_double(void *d)
{
  fprintf(stderr, "%.2f", *(double *) d);
}

/** \brief Copy the values from the matrix src into the matrix dst
 * \param dest: destination
 * \param src: source
 * \param lsize: number of lines to copy
 * \param rsize: number of rows to copy
 * \param lpos_dst: line offset on destination matrix
 * \param rpos_dst: row offset on destination matrix
 * \param lpos_src: line offset on destination matrix
 * \param rpos_src: row offset on destination matrix
 */
void xbt_matrix_copy_values(xbt_matrix_t dst, xbt_matrix_t src,
                            unsigned int lsize, unsigned int rsize,
                            unsigned int lpos_dst, unsigned int rpos_dst,
                            unsigned int lpos_src, unsigned int rpos_src,
                            pvoid_f_pvoid_t const cpy_f)
{
  unsigned int i, j;

  DEBUG10
      ("Copy a %dx%d submatrix from %dx%d(of %dx%d) to %dx%d (of %dx%d)",
       lsize, rsize, lpos_src, rpos_src, src->lines, src->rows, lpos_dst,
       rpos_dst, dst->lines, dst->rows);

  /* everybody knows that issue is between the chair and the screen (particulary in my office) */
  xbt_assert(src->elmsize == dst->elmsize);
  /* don't check free_f since the user may play weird games with this */

  xbt_assert(lpos_src + lsize <= src->lines);
  xbt_assert(rpos_src + rsize <= src->rows);

  xbt_assert(lpos_dst + lsize <= dst->lines);
  xbt_assert(rpos_dst + rsize <= dst->rows);

  /* Lets get serious here */
  for (i = 0; i < rsize; i++) {
    if (cpy_f) {
      for (j = 0; j < lsize; j++)
        xbt_matrix_get_as(dst, j + lpos_dst, i + rpos_dst, void *) =
            (*cpy_f) (xbt_matrix_get_ptr(src, j + rpos_src, i + lpos_src));
    } else {
      memcpy(xbt_matrix_get_ptr(dst, lpos_dst, i + rpos_dst),
             xbt_matrix_get_ptr(src, lpos_src, i + rpos_src),
             dst->elmsize * lsize);
    }
  }

}

/** \brief Creates a new matrix of double filled with zeros */
xbt_matrix_t xbt_matrix_double_new_zeros(int lines, int rows)
{
  xbt_matrix_t res = xbt_matrix_new(lines, rows, sizeof(double), NULL);

  memset(res->data, 0, res->elmsize * res->lines * res->rows);
  return res;
}

/** \brief Creates a new matrix of double being the identity matrix */
xbt_matrix_t xbt_matrix_double_new_id(int lines, int rows)
{
  xbt_matrix_t res = xbt_matrix_double_new_zeros(lines, rows);
  int i;

  for (i = 0; i < lines; i++)
    xbt_matrix_get_as(res, i, i, double) = 1;
  return res;
}

/** \brief Creates a new matrix of double randomly filled */
xbt_matrix_t xbt_matrix_double_new_rand(int lines, int rows)
{
  xbt_matrix_t res = xbt_matrix_new(lines, rows, sizeof(double), NULL);
  int i, j;

  for (i = 0; i < lines; i++)
    for (j = 0; j < rows; j++)
      xbt_matrix_get_as(res, i, j, double) = (double) rand();
  return res;
}

/** \brief Creates a new matrix of double containing the sequence of numbers in order */
xbt_matrix_t xbt_matrix_double_new_seq(int lines, int rows)
{
  xbt_matrix_t res = xbt_matrix_new(lines, rows, sizeof(double), NULL);
  int i;

  for (i = 0; i < lines * rows; i++)
    *(double *) &res->data[i * res->elmsize] = i;

  return res;
}

/** \brief Checks whether the matrix contains the sequence of numbers */
int xbt_matrix_double_is_seq(xbt_matrix_t mat)
{
  int i;

  for (i = 0; i < mat->lines * mat->rows; i++) {
    double val = xbt_matrix_get_as(mat, i, 0, double);
    if (val != i)
      return 0;
  }

  return 1;
}

/** \brief Creates a new matrix being the multiplication of two others */
xbt_matrix_t xbt_matrix_double_new_mult(xbt_matrix_t A, xbt_matrix_t B)
{
  xbt_matrix_t result = xbt_matrix_double_new_zeros(A->lines, B->rows);

  xbt_matrix_double_addmult(A, B, result);
  return result;
}

/** \brief add to C the result of A*B */
void xbt_matrix_double_addmult(xbt_matrix_t A, xbt_matrix_t B,
                               /*OUT*/ xbt_matrix_t C)
{
  unsigned int i, j, k;

  xbt_assert2(A->lines == C->lines,
              "A->lines != C->lines (%d vs %d)", A->lines, C->lines);
  xbt_assert(B->rows == C->rows);

  for (i = 0; i < C->lines; i++)
    for (j = 0; j < C->rows; j++)
      for (k = 0; k < B->lines; k++)
        xbt_matrix_get_as(C, i, j, double) +=
            xbt_matrix_get_as(A, i, k, double) * xbt_matrix_get_as(B, k, j,
                                                                   double);
}

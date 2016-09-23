/* xbt_matrix_t management functions                                        */

/* Copyright (c) 2006-2007, 2009-2010, 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_MATRIX_H
#define XBT_MATRIX_H

#include "xbt/misc.h"
#include "xbt/function_types.h"

SG_BEGIN_DECL()

typedef struct {
  unsigned int lines, rows;
  unsigned long elmsize;

  char *data;
  void_f_pvoid_t free_f;
} s_xbt_matrix_t, *xbt_matrix_t;


  /** @brief Retrieve the address of a cell (not its content)
   *  @hideinitializer */
#define xbt_matrix_get_ptr(mat,l,c) \
  ((void*)&(mat)->data[(c)*(mat)->lines*(mat)->elmsize + (l)*(mat)->elmsize])

  /** @brief Quick retrieval of scalar content
   *  @hideinitializer */
#define xbt_matrix_get_as(mat,l,c,type) *(type*)xbt_matrix_get_ptr(mat,l,c)

XBT_PUBLIC(xbt_matrix_t) xbt_matrix_new(int lines, int rows, const unsigned long elmsize, void_f_pvoid_t const free_f);
XBT_PUBLIC(xbt_matrix_t) xbt_matrix_new_sub(xbt_matrix_t from, int lsize, int rsize, int lpos, int rpos,
                                            pvoid_f_pvoid_t const cpy_f);
XBT_PUBLIC(void) xbt_matrix_free(xbt_matrix_t matrix);

XBT_PUBLIC(void) xbt_matrix_copy_values(xbt_matrix_t dest, xbt_matrix_t src, unsigned int lsize, unsigned int rsize,
                                        unsigned int lpos_dst, unsigned int rpos_dst, unsigned int lpos_src,
                                        unsigned int rpos_src, pvoid_f_pvoid_t const cpy_f);

XBT_PUBLIC(void) xbt_matrix_dump(xbt_matrix_t matrix, const char *name, int coords, void_f_pvoid_t display_fun);

XBT_PUBLIC(xbt_matrix_t) xbt_matrix_double_new_zeros(int lines, int rows);
XBT_PUBLIC(xbt_matrix_t) xbt_matrix_double_new_id(int lines, int rows);
XBT_PUBLIC(xbt_matrix_t) xbt_matrix_double_new_seq(int lines, int rows);
XBT_PUBLIC(void) xbt_matrix_double_addmult(xbt_matrix_t A, xbt_matrix_t B, /*OUT*/ xbt_matrix_t C);
SG_END_DECL()
#endif                          /* XBT_MATRIX_H */

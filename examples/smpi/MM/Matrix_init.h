/* Copyright (c) 2012, 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
//#undef CYCLIC
#define CYCLIC
#undef SIMPLE_MATRIX
//#define SIMPLE_MATRIX

void matrices_initialisation(double ** p_a, double ** p_b, double ** p_c,
                             size_t m, size_t k_a, size_t k_b, size_t n,
                             size_t row, size_t col);
void matrices_allocation(double ** p_a, double ** p_b, double ** p_c,
                         size_t m, size_t k_a, size_t k_b, size_t n);
void blocks_initialisation(double ** p_a_local, double ** p_b_local,
                           size_t m, size_t B_k, size_t n);


void check_result(double *c, double *a, double *b,
                  size_t m, size_t n, size_t k_a, size_t k_b,
                  size_t row, size_t col,
                  size_t size_row, size_t size_col);

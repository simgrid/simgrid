/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf/maxmin.h"
#include "../src/xbt/fifo_private.h" /* Yeah! I know. It is very dirty. */

typedef struct lmm_mat_element {
  s_xbt_fifo_item_t row;
  FLOAT value;
} s_mat_element_t;

typedef struct lmm_cnst_element {
  s_xbt_fifo_t row; /* in fact a list of lmm_mat_element_t */
  void *id;
  FLOAT bound;
  FLOAT usage;
} s_lmm_cnst_element_t;

typedef struct lmm_var_element {
  void *id;
  s_mat_element_t *cnsts;
  int cnsts_size;
  FLOAT value;
  FLOAT bound;
} s_lmm_var_element_t;

typedef struct lmm_system {
  s_xbt_fifo_item_t var_set; /* in fact a list of lmm_var_element_t */
  s_xbt_fifo_item_t cnst_set; /* in fact a list of lmm_cnst_element_t */
};

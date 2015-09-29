/* Copyright (c) 2010, 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/base.h"
#include "xbt/dict.h"
#include "xbt/dynar.h"
#include "xbt/setset.h"
#include "xbt/fifo.h"

#define BITS_INT (8 * sizeof(int))

typedef struct s_xbt_setset_elm {
  XBT_SETSET_HEADERS;
} s_xbt_setset_elm_t, *xbt_setset_elm_t;

typedef union u_xbt_setset_elm_entry {
  /* Information when the entry is being used */
  struct {
    xbt_setset_elm_t obj;
  } info;
  /* Information when the entry is free */
  struct {
    unsigned long next;
  } free;
} u_xbt_setset_elm_entry_t, *xbt_setset_elm_entry_t;

typedef struct s_xbt_setset_set {
  xbt_setset_t setset;          /* setset that contains this set */
  unsigned int size;            /* in integers */
  unsigned int *bitmap;         /* the bit array */
} s_xbt_setset_set_t;

typedef struct s_xbt_setset {
  xbt_dynar_t elm_array;        /* of s_xbt_setset_elm_entry_t, to find elements by index */
  xbt_fifo_t sets;              /* of s_xbt_setset_set_t, memberships in actual sets of setset */
} s_xbt_setset_t;

typedef struct s_xbt_setset_cursor {
  int idx;                      /* Actual postition of the cursor (bit number) */
  xbt_setset_set_t set;         /* The set associated to the cursor */
} s_xbt_setset_cursor_t;

/* Some internal functions */

XBT_PRIVATE int bitcount(int);

/* Get the object associated to a given index */
XBT_PRIVATE void *_xbt_setset_idx_to_obj(xbt_setset_t setset, unsigned long idx);

/* Check if the nth bit of an integer is set or not*/
XBT_PRIVATE unsigned int _is_bit_set(unsigned int bit, unsigned int integer);

/* Set the nth bit of an array of integers */
XBT_PRIVATE void _set_bit(unsigned int bit, unsigned int *bitmap);

/* Unset the nth bit of an array of integers */
XBT_PRIVATE void _unset_bit(unsigned int bit, unsigned int *bitmap);

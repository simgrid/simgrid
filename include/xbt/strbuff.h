/* $Id: buff.h 3483 2007-05-07 11:18:56Z mquinson $ */

/* strbuff -- string buffers                                                */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_STRBUFF_H
#define XBT_STRBUFF_H

#include "xbt/sysdep.h"
#include "xbt/function_types.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/dict.h"

/**
 ** Buffer code
 **/
typedef struct {
  char *data;
  int used, size;
} s_xbt_strbuff_t, *xbt_strbuff_t;


XBT_PUBLIC(void) xbt_strbuff_empty(xbt_strbuff_t b);
XBT_PUBLIC(xbt_strbuff_t) xbt_strbuff_new(void);
XBT_INLINE XBT_PUBLIC(xbt_strbuff_t) xbt_strbuff_new_from(char *s);
XBT_PUBLIC(void) xbt_strbuff_free(xbt_strbuff_t b);
XBT_INLINE XBT_PUBLIC(void) xbt_strbuff_free_container(xbt_strbuff_t b);
XBT_PUBLIC(void) xbt_strbuff_append(xbt_strbuff_t b, const char *toadd);
XBT_PUBLIC(void) xbt_strbuff_chomp(xbt_strbuff_t b);
XBT_PUBLIC(void) xbt_strbuff_trim(xbt_strbuff_t b);
XBT_PUBLIC(void) xbt_strbuff_varsubst(xbt_strbuff_t b,
                                      xbt_dict_t patterns);

#endif

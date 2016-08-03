/* strbuff -- string buffers                                                */

/* Copyright (c) 2007-2011, 2013-2014. The SimGrid Team.
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

SG_BEGIN_DECL()

/** @defgroup xbt_strbuff String buffers 
 *  @ingroup XBT_adt
 * 
 *  This data container is very similar to the Java StringBuffer: 
 *  that's a string to which you can add content with a lesser performance 
 *  penalty than if you recreate a new string from scratch. Once done building 
 *  your string, you must retrieve the content and free its container.
 * 
 *  @{
 */

/** @brief Buffer data container **/
struct xbt_strbuff {
  char *data;
  int used, size;
};
typedef struct xbt_strbuff  s_xbt_strbuff_t;
typedef struct xbt_strbuff* xbt_strbuff_t;

XBT_PUBLIC(void) xbt_strbuff_clear(xbt_strbuff_t b);
XBT_PUBLIC(xbt_strbuff_t) xbt_strbuff_new(void);
XBT_PUBLIC(xbt_strbuff_t) xbt_strbuff_new_from(const char *s);
XBT_PUBLIC(void) xbt_strbuff_free(xbt_strbuff_t b);
XBT_PUBLIC(void) xbt_strbuff_free_container(xbt_strbuff_t b);
XBT_PUBLIC(void) xbt_strbuff_append(xbt_strbuff_t b, const char *toadd);
XBT_PUBLIC(void) xbt_strbuff_printf(xbt_strbuff_t b, const char *fmt, ...);
XBT_PUBLIC(void) xbt_strbuff_chomp(xbt_strbuff_t b);
XBT_PUBLIC(void) xbt_strbuff_trim(xbt_strbuff_t b);
XBT_PUBLIC(void) xbt_strbuff_varsubst(xbt_strbuff_t b, xbt_dict_t patterns);

/** @} */
SG_END_DECL()
#endif

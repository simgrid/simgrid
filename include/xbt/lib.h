/* xbt/lib.h - api to a generic library                                     */

/* Copyright (c) 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_LIB_H
#define _XBT_LIB_H

#include <xbt/dict.h>

SG_BEGIN_DECL()

typedef struct s_xbt_lib {
  xbt_dict_t dict;
  int levels;
  void_f_pvoid_t *free_f;       /* This is actually a table */
} s_xbt_lib_t, *xbt_lib_t;

#define xbt_lib_cursor_t xbt_dict_cursor_t

XBT_PUBLIC(xbt_lib_t) xbt_lib_new(void);
XBT_PUBLIC(void) xbt_lib_free(xbt_lib_t * lib);
XBT_PUBLIC(int) xbt_lib_add_level(xbt_lib_t lib, void_f_pvoid_t free_f);
XBT_PUBLIC(void) xbt_lib_set(xbt_lib_t lib, const char *name, int level,
                             void *obj);
XBT_PUBLIC(void *) xbt_lib_get_or_null(xbt_lib_t lib, const char *name,
                                       int level);

#define xbt_lib_length(lib) xbt_dict_length((lib)->dict)

/** @def xbt_lib_foreach
    @hideinitializer */
#define xbt_lib_foreach(lib, cursor, key, data)         \
  xbt_dict_foreach((lib)->dict, cursor, key, data)

SG_END_DECL()
#endif                          /* _XBT_LIB_H */

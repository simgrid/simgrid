/* xbt/lib.h - api to a generic library                                     */

/* Copyright (c) 2011, 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_LIB_H
#define _XBT_LIB_H

#include <xbt/dict.h>

SG_BEGIN_DECL()

/** Container for all the objects of a given type
 *
 *  * each item is identified by a string name/identifier;
 *
 *  * the lib itself is a dictionary from the element id to the element;
 *
 *  * the element itself is represented aby the xbt_dictelm_t;
 *
 *  * the element can store any number of associated facets/data structures (corresponding to the different layers of
 *    SimGrid or its extensions) in ((void**)dictelt->content)[level];
 *
 *  * each level is allocated in the lib with `xbt_lib_add_level`.
 *
 *  <pre>
 *  // Define a collection for the foo objects and two associated facets:
 *  typedef xbt_dictelm_t foo_t;
 *  xbt_lib_t foo_lib = xbt_lib_new();
 *  int BAR_FOO_LEVEL  = xbt_lib_add_level(foo_lib, free_bar);
 *  int AUTH_FOO_LEVEL = xbt_lib_add_level(foo_lib, free_auth);
 *
 *  // Store a bar:
 *  bar_t bar = bar_new();
 *  char* id = bar_name(bar);
 *  xbt_lib_set(id, id, BAR_FOO_LEVEL, bar);
 *
 *  // Find the corresponding foo and the facet again:
 *  foo_t foo = xbt_lib_get_elm_or_null(foo_lib, id);
 *  bar_t bar2 = (bar_t) xbt_lib_get_level(foo, BAR_FOO_LEVEL);
 *  assert(bar == bar2);
 *
 *  // Add authentication facet for the previous object:
 *  auth_t auth = auth_new();
 *  xbt_lib_set(foo_lib, id, AUTH_FOO_LEVEL, auth);
 *  </pre>
 */
struct s_xbt_lib {
  xbt_dict_t dict;
  int levels;
  void_f_pvoid_t *free_f;       /* This is actually a table */
};
typedef struct s_xbt_lib  s_xbt_lib_t;
typedef struct s_xbt_lib* xbt_lib_t;

#define xbt_lib_cursor_t xbt_dict_cursor_t

XBT_PUBLIC(xbt_lib_t) xbt_lib_new(void);
XBT_PUBLIC(void) xbt_lib_free(xbt_lib_t * lib);
XBT_PUBLIC(int) xbt_lib_add_level(xbt_lib_t lib, void_f_pvoid_t free_f);
XBT_PUBLIC(void) xbt_lib_set(xbt_lib_t lib, const char *name, int level, void *obj);
XBT_PUBLIC(void) xbt_lib_unset(xbt_lib_t lib, const char *key, int level, int invoke_callback);
XBT_PUBLIC(void *) xbt_lib_get_or_null(xbt_lib_t lib, const char *name, int level);
XBT_PUBLIC(xbt_dictelm_t) xbt_lib_get_elm_or_null(xbt_lib_t lib, const char *key);
XBT_PUBLIC(void *) xbt_lib_get_level(xbt_dictelm_t elm, int level);
XBT_PUBLIC(void) xbt_lib_remove(xbt_lib_t lib, const char *key);

#define xbt_lib_length(lib) xbt_dict_length((lib)->dict)

/** @def xbt_lib_foreach
    @hideinitializer */
#define xbt_lib_foreach(lib, cursor, key, data)         \
  xbt_dict_foreach((lib)->dict, cursor, key, data)

SG_END_DECL()

#endif                          /* _XBT_LIB_H */

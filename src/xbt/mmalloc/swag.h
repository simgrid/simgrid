/* Copyright (c) 2004-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Warning, this module is done to be efficient and performs tons of cast and dirty things. So avoid using it unless
 * you really know what you are doing. */

#ifndef XBT_SWAG_H
#define XBT_SWAG_H

#include "xbt/sysdep.h" /* size_t */

/*
 * XBT_swag: a O(1) set based on linked lists
 *
 *  Warning, this module is done to be efficient and performs tons of cast and dirty things. So make sure you know what
 *  you are doing while using it.
 *  It is basically a fifo but with restrictions so that it can be used as a set. Any operation (add, remove, belongs)
 *  is O(1) and no call to malloc/free is done.
 *
 *  @deprecated If you are using C++, you might want to use `boost::intrusive::set` instead.
 */

/* Swag types
 *
 *  Specific set.
 *
 *  These typedefs are public so that the compiler can do his job but believe me, you don't want to try to play with
 *  those structs directly. Use them as an abstract datatype.
 */

typedef struct xbt_swag_hookup {
  void *next;
  void *prev;
} s_xbt_swag_hookup_t;

/* This type should be added to a type that is to be used in a swag.
 *
 *  Whenever a new object with this struct is created, all fields have to be set to NULL
 *
 * Here is an example like that :

\code
typedef struct foo {
  s_xbt_swag_hookup_t set1_hookup;
  s_xbt_swag_hookup_t set2_hookup;

  double value;
} s_foo_t, *foo_t;
...
{
  s_foo_t elem;
  xbt_swag_t set1=NULL;
  xbt_swag_t set2=NULL;

  set1 = xbt_swag_new(xbt_swag_offset(elem, set1_hookup));
  set2 = xbt_swag_new(xbt_swag_offset(elem, set2_hookup));

}
\endcode
*/

struct xbt_swag {
  void *head;
  void *tail;
  size_t offset;
  int count;
};
typedef struct xbt_swag s_xbt_swag_t;

#endif /* XBT_SWAG_H */

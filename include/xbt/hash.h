/* $Id: str.h,v 1.5 2007/05/02 10:08:55 mquinson Exp $ */

/* hash.h - Various hashing functions.                                      */

/* Copyright (c) 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_HASH_H
#define XBT_HASH_H
#include "xbt/str.h"

/* Chord needs a SHA1 algorithm. Let's drop it in there */
typedef struct s_xbt_sha_ s_xbt_sha_t, *xbt_sha_t;

XBT_PUBLIC(xbt_sha_t) xbt_sha_new(void);
XBT_PUBLIC(void) xbt_sha_free(xbt_sha_t sha);

XBT_PUBLIC(void) xbt_sha_feed(xbt_sha_t sha, const unsigned char *data,
                              size_t len);
XBT_PUBLIC(void) xbt_sha_reset(xbt_sha_t sha);

XBT_PUBLIC(void) xbt_sha_print(xbt_sha_t sha, char *hash);
XBT_PUBLIC(char *) xbt_sha_read(xbt_sha_t sha);

XBT_PUBLIC(void) xbt_sha(const char *data, char *hash);


#endif                          /* XBT_HASH_H */

/* hash.h - Various hashing functions.                                      */

/* Copyright (c) 2008-2011, 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_HASH_H
#define XBT_HASH_H

#include "xbt/misc.h"

SG_BEGIN_DECL()

/* The classical SHA1 algorithm */
typedef struct s_xbt_sha_ s_xbt_sha_t, *xbt_sha_t;

XBT_PUBLIC(xbt_sha_t) xbt_sha_new(void);
XBT_PUBLIC(void) xbt_sha_free(xbt_sha_t sha);

XBT_PUBLIC(void) xbt_sha_feed(xbt_sha_t sha, const unsigned char *data,
                              size_t len);
XBT_PUBLIC(void) xbt_sha_reset(xbt_sha_t sha);

XBT_PUBLIC(void) xbt_sha_print(xbt_sha_t sha, char *hash);
XBT_PUBLIC(char *) xbt_sha_read(xbt_sha_t sha);

XBT_PUBLIC(void) xbt_sha(const char *data, char *hash);

SG_END_DECL()

#endif                          /* XBT_HASH_H */

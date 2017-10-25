/* Copyright (c) 2007-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_FILE_H
#define XBT_FILE_H

#include <xbt/base.h>

SG_BEGIN_DECL()

/** @defgroup XBT_file File manipulation functions
 *  @ingroup XBT_misc
 *
 * This module redefine some quite classical functions such as xbt_dirname() or xbt_basename() for the platforms
 * lacking them.
 * @{
 */
/* Our own implementation of dirname, that does not exist on windows */
XBT_PUBLIC(char *) xbt_dirname(const char *path);
XBT_PUBLIC(char *) xbt_basename(const char *path);

/**@}*/

SG_END_DECL()
#endif                          /* XBT_FILE_H */

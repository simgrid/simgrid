/* xbt/synchro.h -- Synchronization tools                                   */
/* Usable in simulator, (or in real life when mixing with GRAS)             */

/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* synchro_core.h is splited away since it is used by dynar.h, and we use dynar here */

#ifndef SYNCHRO_H_
#define SYNCHRO_H_
#include "xbt/misc.h"           /* SG_BEGIN_DECL */
#include "xbt/dynar.h"
SG_BEGIN_DECL()

XBT_PUBLIC(void) xbt_dynar_dopar(xbt_dynar_t datas,
                                 void_f_int_pvoid_t function);

SG_END_DECL()
#endif                          /* SYNCHRO_H_ */

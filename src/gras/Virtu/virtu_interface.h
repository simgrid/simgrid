/* $Id$ */

/* virtu[alization] - speciafic parts for each OS and for SG                */

/* module's public interface exported within GRAS, but not to end user.     */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_VIRTU_INTERFACE_H
#define GRAS_VIRTU_INTERFACE_H

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/error.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"
#include "gras/virtu.h"
#include "gras/process.h"

/* declare a new process specific data 
   (used by gras_<module>_register to make sure that gras_process_init will create it) */

typedef void* (pvoid_f_void_t)(void); /* FIXME: find a better place for it */

void gras_procdata_add(const char *name, pvoid_f_void_t creator,void_f_pvoid_t destructor);
void *gras_libdata_get(const char *name);

#endif  /* GRAS_VIRTU_INTERFACE_H */

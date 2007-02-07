/* $Id$ */

/* virtu[alization] - speciafic parts for each OS and for SG                */

/* module's public interface exported within GRAS, but not to end user.     */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_VIRTU_INTERFACE_H
#define GRAS_VIRTU_INTERFACE_H

#include "xbt/function_types.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/set.h"
#include "gras/virtu.h"
#include "gras/process.h"

/* shutdown the module mechanism (world-wide cleanups) */
XBT_PUBLIC(void) gras_moddata_exit(void);
/* shutdown this process wrt module mecanism (process-wide cleanups) */
XBT_PUBLIC(void) gras_moddata_leave(void);

/* This is the old interface (deprecated) */


/* declare a new process specific data 
   (used by gras_<module>_register to make sure that gras_process_init will create it) */
XBT_PUBLIC(int) gras_procdata_add(const char *name, pvoid_f_void_t creator,void_f_pvoid_t destructor);

XBT_PUBLIC(void*) gras_libdata_by_name(const char *name);
XBT_PUBLIC(void*) gras_libdata_by_id(int id);

#endif  /* GRAS_VIRTU_INTERFACE_H */

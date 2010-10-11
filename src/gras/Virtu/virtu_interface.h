/* virtu[alization] - speciafic parts for each OS and for SG                */

/* module's public interface exported within GRAS, but not to end user.     */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

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

SG_BEGIN_DECL()

/* shutdown the module mechanism (world-wide cleanups) */
void gras_moddata_exit(void);
/* shutdown this process wrt module mecanism (process-wide cleanups) */
void gras_moddata_leave(void);


/* Perform the various intialisations needed by gras. Each process must run it */
XBT_PUBLIC(void) gras_process_init(void);

/* Frees the memory allocated by gras. Processes should run it */
XBT_PUBLIC(void) gras_process_exit(void);


/* This is the old interface (deprecated) */

/* declare a new process specific data 
   (used by gras_<module>_register to make sure that gras_process_init will create it) */
int gras_procdata_add(const char *name, pvoid_f_void_t creator,
                      void_f_pvoid_t destructor);

void *gras_libdata_by_name(const char *name);
void *gras_libdata_by_id(int id);

SG_END_DECL()
#endif                          /* GRAS_VIRTU_INTERFACE_H */

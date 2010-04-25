/* gras/virtu.h - public interface to virtualization (cross-OS portability) */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_VIRTU_H
#define GRAS_VIRTU_H

#include "xbt/misc.h"           /* SG_BEGIN_DECL */
#include "xbt/time.h"
#include "xbt/dict.h"

SG_BEGIN_DECL()

/* Initialization of the simulation world. Do not call them in RL. 
   Indeed, do not call them at all. Let gras_stub_generator do it for you. */
     void gras_global_init(int *argc, char **argv);
     void gras_create_environment(const char *file);
     void gras_function_register(const char *name, xbt_main_func_t code);
     void gras_launch_application(const char *file);
     void gras_clean(void);
     void gras_main(void);


/** @addtogroup GRAS_virtu  
 *  @brief System call abstraction layer.
 *
 *
 *  @{
 */

/** @brief Get the current time
 *  @return number of second since the Epoch.
 *  (00:00:00 UTC, January 1, 1970 in Real Life, and begining of simulation in SG)
 */
#define gras_os_time() xbt_time()
/** @brief sleeps for the given amount of time.
 *  @param sec: number of seconds to sleep
 */
#define gras_os_sleep(sec) xbt_sleep(sec)
/** @brief get the fully-qualified name of the current host
 *
 * Returns the fully-qualified name of the host machine, or "localhost" if the name
 * cannot be determined.  Always returns the same value, so multiple calls
 * cause no problems.
 */
XBT_PUBLIC(const char *) gras_os_myname(void);

/** @brief returns the number on which this process is listening for incoming messages */
XBT_PUBLIC(int) gras_os_myport(void);

/** @brief get the uri of the current process
 *
 * Returns the concatenation of gras_os_myname():gras_os_myport(). Please do not free the result.
 */
XBT_PUBLIC(const char *) gras_os_hostport(void);

/** @brief get process identification
 *
 * Returns the process ID of the current process.  (This is often used
   by routines that generate unique temporary file names.)
 */
XBT_PUBLIC(int) gras_os_getpid(void);


/* Properties related */
XBT_PUBLIC(xbt_dict_t) gras_process_properties(void);
XBT_PUBLIC(const char *) gras_process_property_value(const char *name);

XBT_PUBLIC(xbt_dict_t) gras_os_host_properties(void);
XBT_PUBLIC(const char *) gras_os_host_property_value(const char *name);

/** @} */
SG_END_DECL()
#endif /* GRAS_VIRTU_H */

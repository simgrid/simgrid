/* Copyright (c) 2023. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_ENVIRON_H
#define MC_ENVIRON_H

/* Define macros for the name of environment variables used by the MC.  Keep them in a separate file without any other
 * includes, since it's also loaded from mmalloc.
 */

/** Environment variable name used to pass the communication socket.
 *
 * It is set by `simgrid-mc` to enable MC support in the children processes.
 */
#define MC_ENV_SOCKET_FD "SIMGRID_MC_SOCKET_FD"

/** Environment variable used to request additional system statistics.
 */
#define MC_ENV_SYSTEM_STATISTICS "SIMGRID_MC_SYSTEM_STATISTICS"

#endif

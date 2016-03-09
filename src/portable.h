/* portable -- header loading to write portable code within SimGrid         */

/* Copyright (c) 2004, 2016. The SimGrid Team. All rights reserved.         */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PORTABLE_H
#define SIMGRID_PORTABLE_H

#include "src/internal_config.h"  /* some information about the environment */

#ifdef _WIN32
# include <windows.h>
#endif

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
# include <sys/sysctl.h>
#endif

/* File handling */
#ifdef _WIN32
  #ifndef S_IRGRP
    #define S_IRGRP 0
  #endif
#endif
#endif                          /* SIMGRID_PORTABLE_H */
